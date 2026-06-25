// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilersampler.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"
#include "samplerrecipe.h"

#include "profilertr.h"

#include <projectexplorer/qmldebugcommandlinearguments.h>

#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/qtdesignwidgets.h>
#include <utils/url.h>

#include <QtTaskTree/QBarrier>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

using namespace Profiler;
using namespace Profiler::Internal;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

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

    // The launch settings are irrelevant while connecting to a running server.
    const auto updateLaunchEnabled = [this] {
        const bool launching = !connectToServer();
        executable.setEnabled(launching);
        arguments.setEnabled(launching);
        workingDirectory.setEnabled(launching);
    };
    updateLaunchEnabled();
    connect(&connectToServer, &BoolAspect::changed, this, updateLaunchEnabled);

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
        for (BoolAspect *aspect : featureAspects)
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

Result<std::shared_ptr<RecordingSession>> QmlProfilerSamplerSettings::createSession() const
{
    auto session = std::make_shared<RecordingSession>();
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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString name = u"qtprofiler-qml-%1%2"_s
                             .arg(QDateTime::currentMSecsSinceEpoch())
                             .arg(QLatin1String(Constants::QtdFileExtension));
    return FilePath::fromString(QDir(dir).filePath(name));
}

ExecutableItem QmlProfilerSampler::recordRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    if (session->launchCommand) {
        // Launch: capture on a freshly allocated local port and tell the target to
        // open a matching QML debug server, blocking until we connect.
        session->serverUrl = urlFromLocalHostAndFreePort();
        const QString args = ProcessArgs::quoteArg(qmlDebugCommandLineArguments(
            QmlProfilerServices, u"port:%1"_s.arg(session->serverUrl.port()), /*block*/ true));
        session->launchCommand->prependArgs(args, CommandLine::Raw);
    }

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

        // Once the server stops recording (we asked it to, or the target finished
        // the trace), the model has been finalized; save it to a .qtd and only
        // advance the barrier from saveFinished, when the file is complete.
        QObject::connect(m_stateManager.get(), &QmlProfilerStateManager::serverRecordingChanged,
                         b, [this, b, session](bool recording) {
            if (recording) {
                session->started.store(true); // capture is live; the duration clock can start
                return;
            }
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
        });

        // Start from a clean slate so repeated recordings don't accumulate.
        m_modelManager->clearAll();

        // Record exactly the features the user selected.
        m_stateManager->setRequestedFeatures(m_settings->requestedFeatures());

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

    return launchThenCapture(session, QBarrierTask(onSetup, onDone));
}

} // namespace QmlProfiler::Internal
