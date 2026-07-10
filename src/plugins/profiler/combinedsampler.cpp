// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "combinedsampler.h"

#include "callstacksampler.h"
#include "perfsampler.h"
#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersampler.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QtTaskTree/QBarrier>

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

using namespace Profiler;
using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

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
        for (BoolAspect *aspect : featureAspects)
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

Result<std::shared_ptr<RecordingSession>> CombinedSamplerSettings::createSession() const
{
    // v1 always launches: both captures must target one process, and launching it
    // ourselves is the only way to guarantee that (and to inject -qmljsdebugger).
    auto session = std::make_shared<RecordingSession>();
    session->intervalUs = int(intervalUs());
    session->requestedFeatures = requestedFeatures();
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

CombinedSampler::CombinedSampler()
    : m_settings(std::make_unique<CombinedSamplerSettings>())
    , m_qml(std::make_unique<QmlProfilerSampler>())
{
    if (HostOsInfo::isMacHost())
        m_native = std::make_unique<CallStackSampler>();
    else if (HostOsInfo::isLinuxHost())
        m_native = std::make_unique<PerfSampler>();
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

    const QString dirName = u"qtprofiler-combined-%1"_s.arg(QDateTime::currentMSecsSinceEpoch());
    const QString bundlePath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(dirName);
    if (!QDir().mkpath(bundlePath)) {
        fail(Tr::tr("Cannot create the combined trace directory %1.").arg(bundlePath));
        return;
    }

    const QDir bundle(bundlePath);
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

    parent->result.emplace(FilePath::fromString(bundlePath));
}

ExecutableItem CombinedSampler::captureRecipe(const std::shared_ptr<RecordingSession> &parent) const
{
    // Each sub-capture writes its own trace into its own result; they share the
    // parent's target and stop flag, forwarded below. The QML child connects to
    // the debug server prepareLaunch() allocated on the parent.
    auto qmlChild = std::make_shared<RecordingSession>();
    auto nativeChild = std::make_shared<RecordingSession>();
    qmlChild->serverUrl = parent->serverUrl;
    qmlChild->requestedFeatures = parent->requestedFeatures; // QML feature toggles
    nativeChild->intervalUs = parent->intervalUs;            // native sampler cadence

    // recordRecipe() runs captureRecipe() only after the target has started (see
    // launchThenCapture), so parent->pid is already set; copy it to both children
    // before either sub-capture reads it.
    const auto prime = [parent, qmlChild, nativeChild] {
        const qint64 pid = parent->pid.load();
        qmlChild->pid.store(pid);
        nativeChild->pid.store(pid);
        qmlChild->processName = parent->processName;
        nativeChild->processName = parent->processName;
    };

    // Runs in parallel with the two captures: forwards the GUI's stop request (and
    // stops the other side once either finishes), mirrors combined progress and
    // "started" state onto the parent, and completes once both have a result.
    const auto onForwardSetup = [parent, qmlChild, nativeChild](QBarrier &barrier) {
        QBarrier *b = &barrier;
        auto *poll = new QTimer(b);
        poll->setInterval(50);
        QObject::connect(poll, &QTimer::timeout, b, [b, parent, qmlChild, nativeChild, poll] {
            const bool eitherDone = qmlChild->result.has_value() || nativeChild->result.has_value();
            if (parent->stop.load() || eitherDone) {
                qmlChild->stop.store(true);
                nativeChild->stop.store(true);
            }
            if (qmlChild->started.load() && nativeChild->started.load())
                parent->started.store(true);
            parent->progress.store(qMax(qmlChild->progress.load(), nativeChild->progress.load()),
                                   std::memory_order_relaxed);
            if (qmlChild->result.has_value() && nativeChild->result.has_value()) {
                poll->stop();
                b->advance();
            }
        });
        poll->start();
    };

    const auto assemble = [parent, qmlChild, nativeChild] {
        assembleBundle(parent, qmlChild, nativeChild);
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
            m_qml->captureRecipe(qmlChild),
            m_native->captureRecipe(nativeChild),
            QBarrierTask(onForwardSetup),
        },
        QSyncTask(assemble),
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

} // namespace QmlProfiler::Internal
