// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilersampler.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"
#include "sampletrace.h"

#include "profilertr.h"

#include <projectexplorer/qmldebugcommandlinearguments.h>

#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/qtdesignwidgets.h>
#include <utils/url.h>

#include <QtTaskTree/QBarrier>

#include <QDebug>
#include <QTimer>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

using namespace Qt::StringLiterals;

namespace Profiler::Internal {

QmlProfilerSamplerSettings::QmlProfilerSamplerSettings()
{
    setSettingsGroup("QmlProfilerSampler");

    connectToServer.setSettingsKey("ConnectToServer");
    connectToServer.setLabel(Tr::tr("Connect to a running QML debug server"),
                             BoolAspect::LabelPlacement::AtCheckBox);

    host.setSettingsKey("Host");
    host.setLabelText(Tr::tr("Host:"));
    host.setDisplayStyle(StringAspect::LineEditDisplay);
    host.setDefaultValue("localhost");
    host.setEnabler(&connectToServer);

    port.setSettingsKey("Port");
    port.setLabelText(Tr::tr("Port:"));
    port.setRange(1, 65535);
    port.setDefaultValue(3768); // QML debug server's conventional default port.
    port.setEnabler(&connectToServer);

    updateTargetEnabled();
    connect(&connectToServer, &BoolAspect::changed, this, [this] { updateTargetEnabled(); });

    // One toggle per profiler feature, defaulting to enabled (record everything).
    for (int feature = 0; feature < QmlDebug::MaximumProfileFeature; ++feature) {
        auto *aspect = new BoolAspect(this);
        const QByteArray key = QByteArray("Feature") + QByteArray::number(feature);
        aspect->setSettingsKey(key);
        aspect->setLabel(QString::fromLatin1(QmlProfilerModelManager::featureName(
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
            connectToServer,
            Row { host, port, st },
            executable,
            arguments,
            workingDirectory,
            Layouting::Group { title(Tr::tr("Record")), Column { features } },
        };
    });
}

void QmlProfilerSamplerSettings::fillOptions(RecordingSession &session) const
{
    session.requestedFeatures = requestedFeatures();
}

// The launch settings are irrelevant while connecting to a running server, and
// both are while the target comes from a run configuration.
void QmlProfilerSamplerSettings::updateTargetEnabled()
{
    const bool own = !targetChosenElsewhere();
    connectToServer.setEnabled(own);
    const bool connecting = own && connectToServer();
    host.setEnabled(connecting);
    port.setEnabled(connecting);
    const bool launching = own && !connectToServer();
    executable.setEnabled(launching);
    arguments.setEnabled(launching);
    workingDirectory.setEnabled(launching);
}

Result<std::shared_ptr<RecordingSession>> QmlProfilerSamplerSettings::createSession() const
{
    auto session = std::make_shared<RecordingSession>();
    fillOptions(*session);
    if (connectToServer()) {
        QUrl url;
        url.setScheme(urlTcpScheme());
        url.setHost(host());
        url.setPort(int(port()));
        session->serverUrl = url;
        return session;
    }
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

quint64 QmlProfilerSamplerSettings::requestedFeatures() const
{
    quint64 features = 0;
    for (int feature = 0; feature < featureAspects.size(); ++feature) {
        if (featureAspects.at(feature)->value())
            features |= (1ULL << feature);
    }
    return features;
}

QmlProfilerSampler::QmlProfilerSampler()
    : m_settings(std::make_unique<QmlProfilerSamplerSettings>())
    , m_modelManager(std::make_unique<QmlProfilerModelManager>())
    , m_stateManager(std::make_unique<QmlProfilerStateManager>())
    , m_clientManager(std::make_unique<QmlProfilerClientManager>())
{
    m_clientManager->setModelManager(m_modelManager.get());
    m_clientManager->setProfilerStateManager(m_stateManager.get());

    // The default logger flashes the Creator message pane, which needs an
    // initialized Core (ActionManager etc.). The standalone viewer has none, so
    // route connection-state messages to qDebug() instead.
    m_clientManager->setLogger([](const QString &message) { qDebug().noquote() << message; });

    // The features to record are taken from the settings at record time (see
    // recordRecipe); without requested features the trace client records nothing.

    // QmlProfilerTool drives the model's initialize()/finalize() from the state
    // manager's recording transitions; replicate that here, ungated by app state.
    QObject::connect(m_stateManager.get(), &QmlProfilerStateManager::serverRecordingChanged,
                     m_modelManager.get(), [this](bool recording) {
        if (recording)
            m_modelManager->initialize();
        else
            m_modelManager->finalize();
    });
}

QmlProfilerSampler::~QmlProfilerSampler() = default;

QString QmlProfilerSampler::displayName() const
{
    return Tr::tr("QML Profiler");
}

bool QmlProfilerSampler::isAvailable(QString *error) const
{
    Q_UNUSED(error)
    return true;
}

SamplerSettings *QmlProfilerSampler::settings() const
{
    return m_settings.get();
}

static FilePath tempQtdPath()
{
    return uniqueTracePath("qtprofiler-qml"_L1, QLatin1StringView(Constants::QtdFileExtension));
}

void QmlProfilerSampler::prepareLaunch(const std::shared_ptr<RecordingSession> &session) const
{
    if (!session->launchCommand)
        return;
    // Launch: capture on a freshly allocated local port and tell the target to
    // open a matching QML debug server, blocking until we connect.
    session->serverUrl = urlFromLocalHostAndFreePort();
    const QString args = ProcessArgs::quoteArg(qmlDebugCommandLineArguments(
        QmlProfilerServices, u"port:%1"_s.arg(session->serverUrl.port()), /*block*/ true));
    session->launchCommand->prependArgs(args, CommandLine::Raw);
}

ExecutableItem QmlProfilerSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    const auto onSetup = [this, session](QBarrier &barrier) {
        QBarrier *b = &barrier;

        // Connecting to nothing should fail in a few seconds, not the default ~50s.
        if (!session->launchCommand) {
            m_clientManager->setMaximumRetries(5);
            m_clientManager->setRetryInterval(1000);
        }

        QObject::connect(m_clientManager.get(), &QmlProfilerClientManager::connectionFailed,
                         b, [b, session] {
            session->result.emplace(ResultError(
                Tr::tr("Could not connect to the QML debug server.")));
            b->stopWithResult(DoneResult::Error);
        });

        // Writes what the model holds and finishes the capture from saveFinished,
        // when the file is complete. Guarded because both ways of getting here --
        // the server reporting that it stopped, and the connection dropping --
        // can happen for one recording.
        auto saving = std::make_shared<bool>(false);
        const auto saveAndFinish = [this, b, session, saving] {
            if (*saving)
                return;
            *saving = true;
            const FilePath out = tempQtdPath();
            QObject::connect(m_modelManager.get(), &QmlProfilerModelManager::saveFinished,
                             b, [b, session, out] {
                session->result.emplace(out);
                b->advance();
            }, Qt::SingleShotConnection);
            QObject::connect(m_modelManager.get(), &QmlProfilerModelManager::error,
                             b, [b, session](const QString &message) {
                session->result.emplace(ResultError(message));
                b->stopWithResult(DoneResult::Error);
            }, Qt::SingleShotConnection);
            m_modelManager->save(out.toFSPathString());
        };

        // Once the server stops recording (we asked it to, or the target finished
        // the trace), the model has been finalized and can be written out.
        QObject::connect(m_stateManager.get(), &QmlProfilerStateManager::serverRecordingChanged,
                         b, [session, saveAndFinish](bool recording) {
            if (recording) {
                session->markStarted(); // capture is live; the duration clock can start
                return;
            }
            saveAndFinish();
        });

        // The target can also just go away, in which case the server will never
        // report that it stopped and nothing above would ever end the capture.
        // Finalize whatever arrived and write that; a recording that never got
        // anything has nothing to show and says so.
        QObject::connect(m_clientManager.get(), &QmlProfilerClientManager::connectionClosed,
                         b, [this, b, session, saving, saveAndFinish] {
            if (*saving || session->result.has_value())
                return;
            m_modelManager->finalize();
            if (m_modelManager->isEmpty()) {
                session->result.emplace(ResultError(
                    Tr::tr("The application finished before it sent any profiling data.")));
                b->stopWithResult(DoneResult::Error);
                return;
            }
            saveAndFinish();
        });

        // Start from a clean slate so repeated recordings don't accumulate.
        m_modelManager->clearAll();

        // Record exactly the features requested on the session (set by
        // createSession, or by a composite backend driving this capture); fall
        // back to the backend's own settings if the session left it unset.
        m_stateManager->setRequestedFeatures(
            session->requestedFeatures ? session->requestedFeatures : m_settings->requestedFeatures());

        m_clientManager->setServer(session->serverUrl);
        m_clientManager->connectToServer();
        m_stateManager->setClientRecording(true);

        // The GUI thread owns session->stop; poll it and translate a stop request
        // into "stop recording", which makes the server send its final trace.
        auto *poll = new QTimer(b);
        poll->setInterval(50);
        QObject::connect(poll, &QTimer::timeout, b, [this, session, poll] {
            if (!session->stop.load())
                return;
            poll->stop();
            m_stateManager->setClientRecording(false);
            m_clientManager->stopRecording();
        });
        poll->start();
    };

    const auto onDone = [this] {
        // Drop the connection so no QML-debug socket lingers across recordings.
        m_clientManager->disconnectFromServer();
    };

    return QBarrierTask(onSetup, onDone);
}

} // namespace Profiler::Internal
