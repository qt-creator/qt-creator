// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewruncontrol.h"

#include "qmlpreviewconnectionmanager.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/buildsystem/qmlbuildsystem.h>
#include <qmlprojectmanager/qmlmainfileaspect.h>
#include <qmlprojectmanager/qmlmultilanguageaspect.h>

#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/url.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlPreview {

class QmlPreviewRunner : public ProjectExplorer::RunWorker
{
public:
    QmlPreviewRunner(ProjectExplorer::RunControl *runControl, QmlPreviewPlugin *plugin,
                     const QmlPreviewRunnerSetting &settings);

private:
    void start() override;
    void stop() override;

    QmlPreviewConnectionManager m_connectionManager;
    std::unique_ptr<Utils::Async<void>> m_translationUpdater;
};

QmlPreviewRunner::QmlPreviewRunner(RunControl *runControl, QmlPreviewPlugin *plugin,
                                   const QmlPreviewRunnerSetting &settings)
    : RunWorker(runControl)
{
    setId("QmlPreviewRunner");
    m_connectionManager.setFileLoader(settings.fileLoader);
    m_connectionManager.setFileClassifier(settings.fileClassifier);
    m_connectionManager.setFpsHandler(settings.fpsHandler);
    m_connectionManager.setQmlDebugTranslationClientCreator(
        settings.createDebugTranslationClientMethod);

    connect(plugin, &QmlPreviewPlugin::updatePreviews,
            &m_connectionManager, &QmlPreviewConnectionManager::loadFile);
    connect(plugin, &QmlPreviewPlugin::rerunPreviews,
            &m_connectionManager, &QmlPreviewConnectionManager::rerun);
    connect(plugin, &QmlPreviewPlugin::zoomFactorChanged,
            &m_connectionManager, &QmlPreviewConnectionManager::zoom);
    connect(plugin, &QmlPreviewPlugin::localeIsoCodeChanged,
            &m_connectionManager, &QmlPreviewConnectionManager::language);

    connect(&m_connectionManager, &QmlPreviewConnectionManager::connectionOpened,
            this, [this, plugin, settings] {
        if (settings.zoomFactor > 0)
            m_connectionManager.zoom(settings.zoomFactor);
        if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current()) {
            if (!multiLanguageAspect->currentLocale().isEmpty())
                m_connectionManager.language(multiLanguageAspect->currentLocale());
        }
        plugin->previewCurrentFile();
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

    if (settings.refreshTranslationsFunction) {
        m_translationUpdater.reset(new Async<void>);
        m_translationUpdater->setParent(this);
        m_translationUpdater->setConcurrentCallData(settings.refreshTranslationsFunction);
        // Cancel and blocking wait for finished when deleting m_translationUpdater.
        m_translationUpdater->setFutureSynchronizer(nullptr);
    }
}

void QmlPreviewRunner::start()
{
    if (m_translationUpdater)
        m_translationUpdater->start();
    m_connectionManager.setBuildConfiguration(runControl()->buildConfiguration());
    m_connectionManager.setServer(runControl()->qmlChannel());
    m_connectionManager.connectToServer();
    reportStarted();
}

void QmlPreviewRunner::stop()
{
    m_translationUpdater.reset();
    m_connectionManager.disconnectFromServer();
    reportStopped();
}

QmlPreviewRunWorkerFactory::QmlPreviewRunWorkerFactory(QmlPreviewPlugin *plugin,
                                                       const QmlPreviewRunnerSetting *runnerSettings)
{
    setProducer([plugin, runnerSettings](RunControl *runControl) {
        auto runner = new QmlPreviewRunner(runControl, plugin, *runnerSettings);
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
                    runControl->buildConfiguration()->buildSystem());
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
            worker->setCommandLine(cmd.toLocal());
        });
        return worker;
    });
    addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addSupportForLocalRunConfigs();
}

} // QmlPreview
