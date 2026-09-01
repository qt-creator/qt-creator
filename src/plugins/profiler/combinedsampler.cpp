// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "combinedsampler.h"

#include "callstacksampler.h"
#include "combinedtraceloader.h"
#include "perfsampler.h"
#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersampler.h"
#include "sampletrace.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

CombinedSamplerSettings::CombinedSamplerSettings()
{
    setSettingsGroup("CombinedSampler");

    intervalUs.setSettingsKey("IntervalUs");
    intervalUs.setLabelText(Tr::tr("Sample interval (µs):"));
    intervalUs.setRange(0, 1000000); // 0 = as fast as possible.
    intervalUs.setDefaultValue(200);

    // One toggle per profiler feature, defaulting to enabled (record everything).
    for (int feature = 0; feature < QmlDebug::MaximumProfileFeature; ++feature) {
        auto *aspect = new BoolAspect(this);
        const QByteArray key = QByteArray("Feature") + QByteArray::number(feature);
        aspect->setSettingsKey(key);
        aspect->setLabel(QString::fromLatin1(Profiler::Internal::QmlProfilerModelManager::featureName(
                             QmlDebug::ProfileFeature(feature))),
                         BoolAspect::LabelPlacement::AtCheckBox);
        aspect->setDefaultValue(true);
        featureAspects.append(aspect);
    }

    setLayouter([this] {
        using namespace Layouting;
        Flow features;
        for (BoolAspect *aspect : std::as_const(featureAspects))
            features.addItem(*aspect);

        return Column {
            executable,
            arguments,
            workingDirectory,
            Layouting::Group { title(Tr::tr("CPU Sampler")), Column { Row { intervalUs, st } } },
            Layouting::Group { title(Tr::tr("QML Profiler")), Column { features } },
        };
    });
}

quint64 CombinedSamplerSettings::requestedFeatures() const
{
    quint64 features = 0;
    for (int feature = 0; feature < featureAspects.size(); ++feature) {
        if (featureAspects.at(feature)->value())
            features |= (1ULL << feature);
    }
    return features;
}

void CombinedSamplerSettings::fillOptions(RecordingSession &session) const
{
    session.intervalUs = int(intervalUs());
    session.requestedFeatures = requestedFeatures();
}

Result<std::shared_ptr<RecordingSession>> CombinedSamplerSettings::createSession() const
{
    // v1 always launches: both captures must target one process, and launching it
    // ourselves is the only way to guarantee that (and to inject -qmljsdebugger).
    auto session = std::make_shared<RecordingSession>();
    fillOptions(*session);
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

CombinedSampler::CombinedSampler()
    : m_settings(std::make_unique<CombinedSamplerSettings>())
    , m_qml(std::make_unique<QmlProfilerSampler>())
{
    if (HostOsInfo::isMacHost() || HostOsInfo::isWindowsHost())
        m_native = std::make_unique<CallStackSampler>();
    else if (HostOsInfo::isLinuxHost())
        m_native = std::make_unique<PerfSampler>();

    // Only the top-level backends' settings are loaded (see the qtprofiler
    // window), so whatever a sub-capture reads from its own settings rather
    // than from the session -- the perf record arguments, the debuginfod
    // toggle -- would stay at its default here.
    if (m_native) {
        if (SamplerSettings *nativeSettings = m_native->settings())
            nativeSettings->readSettings();
    }
}

CombinedSampler::~CombinedSampler() = default;

QString CombinedSampler::displayName() const
{
    return Tr::tr("CPU Sampler + QML Profiler");
}

bool CombinedSampler::isAvailable(QString *error) const
{
    if (!m_native) {
        if (error)
            *error = Tr::tr("No CPU sampler is available on this platform.");
        return false;
    }
    if (!m_native->isAvailable(error))
        return false;
    return m_qml->isAvailable(error);
}

SamplerSettings *CombinedSampler::settings() const
{
    return m_settings.get();
}

std::optional<SamplerFix> CombinedSampler::availableFix() const
{
    // The native half does the sampling that a system setting can block; the QML
    // half talks to the target over a debug socket and has nothing to offer.
    return m_native ? m_native->availableFix() : std::nullopt;
}

void CombinedSampler::prepareLaunch(const std::shared_ptr<RecordingSession> &session) const
{
    // Only the QML side rewrites the launch command (it injects -qmljsdebugger and
    // allocates session->serverUrl); the native sampler attaches by pid.
    m_qml->prepareLaunch(session);
}

// Moves `finished`'s two child traces into a fresh bundle directory and writes a
// manifest, storing the bundle path (or an error) into parent->result.
static void assembleBundle(const std::shared_ptr<RecordingSession> &parent,
                           const std::shared_ptr<RecordingSession> &qmlChild,
                           const std::shared_ptr<RecordingSession> &nativeChild)
{
    const auto fail = [&parent](const QString &message) {
        if (!parent->result)
            parent->result.emplace(ResultError(message));
    };

    if (!qmlChild->result || !nativeChild->result) {
        fail(Tr::tr("A capture did not produce a result."));
        return;
    }
    const Result<FilePath> &qmlResult = *qmlChild->result;
    const Result<FilePath> &nativeResult = *nativeChild->result;
    if (!qmlResult) {
        fail(qmlResult.error());
        return;
    }
    if (!nativeResult) {
        fail(nativeResult.error());
        return;
    }

    const FilePath bundlePath = uniqueTracePath("qtprofiler-combined"_L1);
    if (!bundlePath.createDir()) {
        fail(Tr::tr("Cannot create the combined trace directory %1.")
                 .arg(bundlePath.toUserOutput()));
        return;
    }

    const QDir bundle(bundlePath.toFSPathString());
    if (!QDir().rename(nativeResult->toFSPathString(), bundle.filePath(combinedSamplerSubdir))) {
        fail(Tr::tr("Cannot move the sampler trace into the bundle."));
        return;
    }
    if (!QFile::rename(qmlResult->toFSPathString(), bundle.filePath(combinedQmlFileName))) {
        fail(Tr::tr("Cannot move the QML trace into the bundle."));
        return;
    }

    // The offset (microseconds) to add to a native sample's timestamp to place it
    // on the QML profiler's (trace-start-relative) timeline. Both captures stamp
    // their go-live instant on the same process-wide steady_clock, so the
    // difference of those anchors correlates the two zero-based clocks. This is an
    // approximation (each anchor lags the trace's true zero by connect/attach
    // latency); exact per-sample alignment needs a common engine clock (M5).
    qint64 qmlClockOffsetUs = 0;
    const qint64 nativeMono = nativeChild->startedMonotonicUs.load();
    const qint64 qmlMono = qmlChild->startedMonotonicUs.load();
    if (nativeMono >= 0 && qmlMono >= 0)
        qmlClockOffsetUs = nativeMono - qmlMono;

    QJsonObject manifest;
    manifest.insert("sampler"_L1, combinedSamplerSubdir);
    manifest.insert("qml"_L1, combinedQmlFileName);
    manifest.insert("pid"_L1, double(parent->pid.load()));
    manifest.insert("qmlClockOffsetUs"_L1, double(qmlClockOffsetUs));

    QFile manifestFile(bundle.filePath(combinedManifestName));
    if (!manifestFile.open(QIODevice::WriteOnly)) {
        fail(Tr::tr("Cannot write the combined trace manifest."));
        return;
    }
    manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    manifestFile.close();

    parent->result.emplace(bundlePath);
}

ExecutableItem CombinedSampler::captureRecipe(const std::shared_ptr<RecordingSession> &parent) const
{
    // Each sub-capture writes its own trace into its own result; they share the
    // parent's target and stop flag, forwarded below. The QML child connects to
    // the debug server the target came up on, which the parent carries.
    auto qmlChild = std::make_shared<RecordingSession>();
    auto nativeChild = std::make_shared<RecordingSession>();
    qmlChild->requestedFeatures = parent->requestedFeatures; // QML feature toggles
    nativeChild->intervalUs = parent->intervalUs;            // native sampler cadence

    // With the two registered, a stop request reaches both, and the parent
    // answers for them when the GUI asks whether capture is live or how far
    // post-processing has come.
    parent->addSubSession(qmlChild);
    parent->addSubSession(nativeChild);

    // Post-processing has two phases -- the captures' own symbolicating and
    // writing, then the merge below -- and they share one bar, so each drives
    // half of it rather than both running 0..100. The merge reports into the
    // parent's own counter once it starts; until then the captures own the bar.
    // The parent's own counter is taken by pointer, not through the session:
    // this lambda lives on that session, so capturing it would be a cycle.
    parent->progressProvider = [qmlChild, nativeChild, mergeProgress = &parent->progress] {
        const int merge = mergeProgress->load(std::memory_order_relaxed);
        if (merge > 0)
            return merge;
        return qMax(qmlChild->progress.load(std::memory_order_relaxed),
                    nativeChild->progress.load(std::memory_order_relaxed)) / 2;
    };

    // The capture only runs once the target has started (see launchThenCapture),
    // so by now the parent knows its pid and which debug server it came up on.
    // Neither is known when this recipe is built, so both are copied to the
    // children here rather than above.
    const auto prime = [parent, qmlChild, nativeChild] {
        const qint64 pid = parent->pid.load();
        qmlChild->pid.store(pid);
        nativeChild->pid.store(pid);
        qmlChild->processName = parent->processName;
        nativeChild->processName = parent->processName;
        qmlChild->serverUrl = parent->serverUrl;
    };

    const auto assemble = [parent, qmlChild, nativeChild] {
        assembleBundle(parent, qmlChild, nativeChild);
    };

    // Merging the two sides into one native-mixed trace is the last, and by far
    // the longest, part of post-processing. Doing it here rather than leaving it
    // to the load that follows keeps it on the recording page's progress bar --
    // and out of the GUI thread, where it used to freeze the window for minutes.
    //
    // Nested, because the bundle path only exists once assemble has stored it:
    // the recipe is built when this task starts rather than when the tree is.
    const auto onMergeSetup = [parent](QTaskTree &taskTree) {
        if (!parent->result || !*parent->result) {
            taskTree.setRecipe({}); // Assembly failed: nothing to merge.
            return;
        }
        // The captures took the bar to 50; the merge takes it the rest of the way.
        const auto reportProgress = [parent](int percent) {
            parent->setProgress(50 + percent / 2);
        };
        taskTree.setRecipe({mergeCombinedBundleRecipe(**parent->result, reportProgress)});
    };

    return Group {
        QSyncTask(prime),
        // finishAllAndSuccess: run both captures to completion and report success
        // even if one fails, so the assemble step below always runs. It inspects
        // each child's result and surfaces the real per-side error (e.g. "Could
        // not connect to the QML debug server") instead of a generic failure.
        Group {
            parallel,
            finishAllAndSuccess,
            // Neither side is worth recording without the other, so whichever
            // ends first ends the recording -- the request cascades through the
            // parent to both. Not a cancel: a capture that is asked to stop
            // still writes what it has, which is the trace half we need.
            Group { m_qml->captureRecipe(qmlChild),
                    onGroupDone([parent] { parent->requestStop(); }) },
            Group { m_native->captureRecipe(nativeChild),
                    onGroupDone([parent] { parent->requestStop(); }) },
        },
        QSyncTask(assemble),
        QTaskTreeTask(onMergeSetup),
    };
}

bool CombinedSampler::isCombinedTrace(const FilePath &dir)
{
    const FilePath manifest = dir / combinedManifestName;
    if (!manifest.exists())
        return false;
    const Result<QByteArray> content = manifest.fileContents();
    if (!content)
        return false;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(*content, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonObject object = doc.object();
    return object.contains("sampler"_L1) && object.contains("qml"_L1);
}

} // namespace Profiler::Internal
