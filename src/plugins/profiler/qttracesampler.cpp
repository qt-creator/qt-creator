// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttracesampler.h"

#include "profilertr.h"
#include "sampletrace.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QRegularExpression>

#include <QtTaskTree/QBarrier>

using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

// The variable Qt's CTF backend reads, at the moment the traced process writes
// its first event. It has to be in the environment the target is launched with.
static const char traceLocationVariable[] = "QTRACE_LOCATION";

// Qt writes the trace into a subdirectory of this name when a session file
// selects what to record.
static const QLatin1StringView sessionSubdirectory("ust");

static const QLatin1StringView sessionFileName("session.json");

// Names every provider, as Qt's session file spells it.
static const QLatin1StringView allProviders("all");

QtTraceSamplerSettings::QtTraceSamplerSettings()
{
    setSettingsGroup("QtTraceSampler");

    providers.setSettingsKey("Providers");
    providers.setLabelText(Tr::tr("Providers:"));
    providers.setDisplayStyle(StringAspect::LineEditDisplay);
    providers.setPlaceHolderText(Tr::tr("All providers"));
    providers.setToolTip(Tr::tr("Tracepoint providers to record, separated by commas, for "
                                "example \"qtcore, qtquick\". Leave empty to record every "
                                "provider the application has."));

    traceDirectory.setSettingsKey("TraceDirectory");
    traceDirectory.setLabelText(Tr::tr("Trace directory:"));
    traceDirectory.setExpectedKind(PathChooserKind::Directory);
    traceDirectory.setPlaceHolderText(Tr::tr("Temporary directory"));
    traceDirectory.setToolTip(Tr::tr("Where to write the traces. Every recording gets a new "
                                     "directory of its own here, so nothing already in this one "
                                     "is written over. Leave empty to use the temporary "
                                     "location."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            executable,
            arguments,
            workingDirectory,
            providers,
            traceDirectory,
        };
    });
}

QStringList QtTraceSamplerSettings::providerList() const
{
    static const QRegularExpression separator(u"[,\\s]+"_s);
    QStringList names = providers().split(separator, Qt::SkipEmptyParts);
    for (QString &name : names)
        name = name.trimmed();
    return names;
}

Result<> QtTraceSamplerSettings::checkTraceDirectory() const
{
    // The temporary location is not the user's to get wrong.
    const FilePath chosen = traceDirectory();
    if (chosen.isEmpty())
        return ResultOk;
    if (!chosen.ensureWritableDir())
        return ResultError(Tr::tr("Cannot create the trace directory %1.")
                               .arg(chosen.toUserOutput()));
    return ResultOk;
}

Result<std::shared_ptr<RecordingSession>> QtTraceSamplerSettings::createSession() const
{
    auto session = std::make_shared<RecordingSession>();
    fillOptions(*session);
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    // A directory that cannot be written to is the one thing about this backend
    // that is wrong before anything has run, and prepareLaunch(), which writes
    // into it, has no way to fail a launch. Say so here instead, where the user
    // is told before the target is started rather than after it has run.
    if (Result<> directory = checkTraceDirectory(); !directory)
        return ResultError(directory.error());
    return session;
}

Result<> writeTraceSession(const FilePath &location, const QString &target,
                           const QStringList &providers)
{
    if (!location.ensureWritableDir()) {
        return ResultError(Tr::tr("Cannot create the trace directory %1.")
                               .arg(location.toUserOutput()));
    }

    // The session file is written even when everything is recorded: its key
    // names the session, which is what the trace carries as its name and what
    // the timeline shows for the recorded process.
    QJsonArray selected;
    for (const QString &provider : providers.isEmpty() ? QStringList{QString(allProviders)}
                                                       : providers) {
        selected.append(provider);
    }
    const QJsonObject session{{target.isEmpty() ? u"trace"_s : target, selected}};
    const FilePath sessionFile = location / sessionFileName;
    if (Result<qint64> written = sessionFile.writeFileContents(
            QJsonDocument(session).toJson(QJsonDocument::Compact));
        !written) {
        return ResultError(written.error());
    }
    return ResultOk;
}

Result<FilePath> collectTrace(const FilePath &location)
{
    const FilePath subdirectory = location / sessionSubdirectory;
    const FilePath traceDirectory = (subdirectory / "metadata").exists() ? subdirectory : location;

    // The metadata is written as soon as the traced process loads the tracing
    // plugin, so its absence means nothing traced at all -- which for a Qt
    // application means a Qt that was not configured for it.
    if (!(traceDirectory / "metadata").exists()) {
        return ResultError(Tr::tr("No trace was written to %1. Applications record Qt "
                                  "tracepoints only when their Qt was configured with "
                                  "\"-trace ctf\".")
                               .arg(location.toUserOutput()));
    }

    // Each traced thread writes a channel of its own. A trace without any can be
    // one where no tracepoint was reached -- or one stopped too early, since Qt
    // writes a thread's events out only once they fill a packet, or when the
    // application exits by itself.
    if (traceDirectory.dirEntries(FileFilter({u"channel_*"_s}, DirFilterFlag::Files)).isEmpty()) {
        return ResultError(Tr::tr("No tracepoints were recorded. The application reached none "
                                  "of them, or it was stopped before Qt wrote the first "
                                  "events out."));
    }

    return traceDirectory;
}

QtTraceSampler::QtTraceSampler()
    : m_settings(std::make_unique<QtTraceSamplerSettings>())
{}

QtTraceSampler::~QtTraceSampler() = default;

QString QtTraceSampler::displayName() const
{
    return Tr::tr("Qt Tracepoints");
}

// Whether the target's Qt carries tracepoints cannot be seen from here -- it is
// a property of the Qt the target was built against, and shows only once it has
// run. collectTrace() explains it then.
bool QtTraceSampler::isAvailable(QString *error) const
{
    Q_UNUSED(error)
    return true;
}

void QtTraceSampler::prepareLaunch(const std::shared_ptr<RecordingSession> &session) const
{
    // The location is prepared for every start, including one whose process Qt
    // Creator's run machinery launches: the target is pointed at the directory
    // through its environment either way.
    //
    // Every start writes into a directory of its own, whether that is the
    // temporary location or one the user chose. A recording must neither delete
    // nor write over what is already in the directory it was pointed at -- Qt
    // clears the location it writes to, but only once it writes at all, so a
    // target whose Qt has no tracing would otherwise leave the earlier trace
    // there for collectTrace() to hand over as this recording's own.
    const FilePath chosen = m_settings->traceDirectory();
    const FilePath location = chosen.isEmpty()
                                  ? uniqueTracePath("qtprofiler-qttrace"_L1)
                                  : uniqueTracePathIn(chosen, "qtprofiler-qttrace"_L1);

    // Names the session, hence the trace, hence what the timeline shows for the
    // recorded process: the executable that is about to run, whoever runs it.
    QString target = session->launchCommand ? session->launchCommand->executable().baseName()
                                            : session->launchExecutable.baseName();
    if (target.isEmpty())
        target = session->processName;

    if (Result<> prepared = writeTraceSession(location, target, m_settings->providerList());
        !prepared) {
        // Reported when the recording ends: there is no way to fail a launch
        // from here, and an unwritable location produces no trace anyway.
        session->result.emplace(ResultError(prepared.error()));
        return;
    }

    // A start prepared twice -- a launch that is retried -- gets a location of
    // its own, and the target must not be pointed at the earlier one.
    Utils::eraseOne(session->launchEnvironmentChanges, [](const EnvironmentItem &item) {
        return item.name == QLatin1StringView(traceLocationVariable);
    });
    session->launchEnvironmentChanges.append(
        {QString::fromLatin1(traceLocationVariable), location.toUserOutput()});
}

ExecutableItem QtTraceSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    const auto onSetup = [session](QBarrier &barrier) {
        QBarrier *b = &barrier;

        // Nothing is captured at this end: the target writes the trace itself,
        // from its first tracepoint to its last. The capture is live as soon as
        // the target is, and lasts until it exits or the user stops.
        session->markStarted();
        session->onStopRequested(b, [b] { b->advance(); });
    };

    return QBarrierTask(onSetup);
}

// Nothing is collected when the capture ends: the target is still running then,
// and is stopped only once the capture is done with it (see launchThenCapture()).
// The events Qt writes on the way out -- a thread's are written when a packet
// fills, or when the application exits -- are part of the recording, and a trace
// judged empty before the target had been asked to exit would be judged wrongly.
void QtTraceSampler::completeRecording(const std::shared_ptr<RecordingSession> &session) const
{
    // Set once. A location that could not be prepared already failed the
    // recording, and there is nothing to collect.
    if (session->result)
        return;

    const EnvironmentItem location = Utils::findOrDefault(
        session->launchEnvironmentChanges, [](const EnvironmentItem &item) {
            return item.name == QLatin1StringView(traceLocationVariable);
        });
    // No location means the launch was never prepared -- a recording torn down
    // before its target was set up. There is no trace, and nothing to explain
    // beyond that: whoever reads an unset result says so.
    if (location.name.isEmpty())
        return;
    session->result.emplace(collectTrace(FilePath::fromUserInput(location.value)));
}

SamplerSettings *QtTraceSampler::settings() const
{
    return m_settings.get();
}

} // namespace Profiler::Internal
