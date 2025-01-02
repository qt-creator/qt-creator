// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewconnectionmanager.h"
#include "qmlpreviewruncontrol.h"

#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlmainfileaspect.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlmultilanguageaspect.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/port.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

#include <QFutureWatcher>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlPreview {

class RefreshTranslationWorker final : public RunWorker
{
public:
    explicit RefreshTranslationWorker(ProjectExplorer::RunControl *runControl,
                                      const QmlPreviewRunnerSetting &runnerSettings)
        : ProjectExplorer::RunWorker(runControl), m_runnerSettings(runnerSettings)
    {
        setId("RefreshTranslationWorker");
        connect(this, &RunWorker::started, this, &RefreshTranslationWorker::startRefreshTranslationsAsync);
        connect(this, &RunWorker::stopped, &m_futureWatcher, &QFutureWatcher<void>::cancel);
        connect(&m_futureWatcher, &QFutureWatcherBase::finished, this, &RefreshTranslationWorker::stop);
    }
    ~RefreshTranslationWorker()
    {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }

private:
    void startRefreshTranslationsAsync()
    {
        m_futureWatcher.setFuture(Utils::asyncRun([this] {
            m_runnerSettings.refreshTranslationsFunction();
        }));
    }
    QmlPreviewRunnerSetting m_runnerSettings;
    QFutureWatcher<void> m_futureWatcher;
};

class QmlPreviewRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlPreviewRunner(ProjectExplorer::RunControl *runControl,
                     const QmlPreviewRunnerSetting &settings);

signals:
    void loadFile(const QString &previewedFile, const QString &changedFile,
                  const QByteArray &contents);
    void language(const QString &locale);
    void zoom(float zoomFactor);
    void rerun();
    void ready();

private:
    void start() override;
    void stop() override;
    QUrl serverUrl() const;

    QmlPreviewConnectionManager m_connectionManager;
};

QmlPreviewRunner::QmlPreviewRunner(RunControl *runControl, const QmlPreviewRunnerSetting &settings)
    : RunWorker(runControl)
{
    setId("QmlPreviewRunner");
    m_connectionManager.setFileLoader(settings.fileLoader);
    m_connectionManager.setFileClassifier(settings.fileClassifier);
    m_connectionManager.setFpsHandler(settings.fpsHandler);
    m_connectionManager.setQmlDebugTranslationClientCreator(
        settings.createDebugTranslationClientMethod);

    connect(this, &QmlPreviewRunner::loadFile,
            &m_connectionManager, &QmlPreviewConnectionManager::loadFile);
    connect(this, &QmlPreviewRunner::rerun,
            &m_connectionManager, &QmlPreviewConnectionManager::rerun);

    connect(this, &QmlPreviewRunner::zoom,
            &m_connectionManager, &QmlPreviewConnectionManager::zoom);
    connect(this, &QmlPreviewRunner::language,
            &m_connectionManager, &QmlPreviewConnectionManager::language);

    connect(&m_connectionManager, &QmlPreviewConnectionManager::connectionOpened,
            this, [this, settings]() {
        if (settings.zoomFactor > 0)
            emit zoom(settings.zoomFactor);
        if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current()) {
            if (!multiLanguageAspect->currentLocale().isEmpty())
                emit language(multiLanguageAspect->currentLocale());
        }
        emit ready();
    });

    connect(&m_connectionManager, &QmlPreviewConnectionManager::restart, runControl, [this, runControl] {
        if (!runControl->isRunning())
            return;

        this->connect(runControl,
                      &RunControl::stopped,
                      ProjectExplorerPlugin::instance(),
                      [runControl] {
                          auto rc = new RunControl(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
                          rc->copyDataFromRunControl(runControl);
                          rc->start();
                      });

        runControl->initiateStop();
    });

    addStartDependency(new RefreshTranslationWorker(runControl, settings));
}

void QmlPreviewRunner::start()
{
    m_connectionManager.setTarget(runControl()->target());
    m_connectionManager.connectToServer(runControl()->qmlChannel());
    reportStarted();
}

void QmlPreviewRunner::stop()
{
    m_connectionManager.disconnectFromServer();
    reportStopped();
}

QmlPreviewRunWorkerFactory::QmlPreviewRunWorkerFactory(QmlPreviewPlugin *plugin,
                                                       const QmlPreviewRunnerSetting *runnerSettings)
{
    setProducer([plugin, runnerSettings](RunControl *runControl) {
        auto runner = new QmlPreviewRunner(runControl, *runnerSettings);
        QObject::connect(plugin, &QmlPreviewPlugin::updatePreviews,
                         runner, &QmlPreviewRunner::loadFile);
        QObject::connect(plugin, &QmlPreviewPlugin::rerunPreviews,
                         runner, &QmlPreviewRunner::rerun);
        QObject::connect(runner, &QmlPreviewRunner::ready,
                         plugin, &QmlPreviewPlugin::previewCurrentFile);
        QObject::connect(plugin, &QmlPreviewPlugin::zoomFactorChanged,
                         runner, &QmlPreviewRunner::zoom);
        QObject::connect(plugin, &QmlPreviewPlugin::localeIsoCodeChanged,
                         runner, &QmlPreviewRunner::language);

        QObject::connect(runner, &RunWorker::started, plugin, [plugin, runControl] {
            plugin->addPreview(runControl);
        });
        QObject::connect(runner, &RunWorker::stopped, plugin, [plugin, runControl] {
            plugin->removePreview(runControl);
        });

        return runner;
    });
    addSupportedRunMode(Constants::QML_PREVIEW_RUNNER);
}

LocalQmlPreviewSupportFactory::LocalQmlPreviewSupportFactory()
{
    setId(ProjectExplorer::Constants::QML_PREVIEW_RUN_FACTORY);
    setProducer([](RunControl *runControl) {
        auto worker = new ProcessRunner(runControl);
        worker->setId("LocalQmlPreviewSupport");

        runControl->setQmlChannel(Utils::urlFromLocalSocket());

        // Create QmlPreviewRunner
        RunWorker *preview =
            runControl->createWorker(ProjectExplorer::Constants::QML_PREVIEW_RUNNER);

        worker->addStopDependency(preview);
        worker->addStartDependency(preview);

        worker->setStartModifier([worker, runControl] {
            CommandLine cmd = worker->commandLine();

            if (const auto aspect = runControl->aspectData<QmlProjectManager::QmlMainFileAspect>()) {
                const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
                    runControl->target()->buildSystem());
                QTC_ASSERT(qmlBuildSystem, return);

                const FilePath mainScript = aspect->mainScript;
                const FilePath currentFile = aspect->currentFile;

                const QString mainScriptFromProject = qmlBuildSystem->targetFile(mainScript).path();

                QStringList qmlProjectRunConfigurationArguments = cmd.splitArguments();

                if (!currentFile.isEmpty() && qmlProjectRunConfigurationArguments.last().contains(mainScriptFromProject)) {
                    qmlProjectRunConfigurationArguments.removeLast();
                    cmd = CommandLine(cmd.executable(), qmlProjectRunConfigurationArguments);
                    cmd.addArg(currentFile.path());
                }
            }

            cmd.addArg(qmlDebugLocalArguments(QmlPreviewServices, runControl->qmlChannel().path()));
            worker->setCommandLine(cmd);
            worker->forceRunOnHost();
        });
        return worker;
    });
    addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addSupportForLocalRunConfigs();
}

} // QmlPreview

#include "qmlpreviewruncontrol.moc"
