// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewruncontrol.h"

#include "projectexplorer/runcontrol.h"
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
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace QmlPreview {

static Group qmlPreviewRecipe(RunControl *runControl)
{
    const auto onTranslationSetup = [](Async<void> &task) {
        if (!QmlPreviewPlugin::settings().refreshTranslationsFunction)
            return SetupResult::StopWithSuccess;

        task.setConcurrentCallData(QmlPreviewPlugin::settings().refreshTranslationsFunction);
        // Cancel and blocking wait for finished when deleting the translation task.
        task.setFutureSynchronizer(nullptr);
        return SetupResult::Continue;
    };

    const auto onPreviewSetup = [runControl](QmlPreviewConnectionManager &task) {
        QmlPreviewPlugin *plugin = QmlPreviewPlugin::instance();
        const QmlPreviewRunnerSetting &settings = QmlPreviewPlugin::settings();
        task.setFileLoader(settings.fileLoader);
        task.setFileClassifier(settings.fileClassifier);
        task.setFpsHandler(settings.fpsHandler);
        task.setQmlDebugTranslationClientCreator(settings.createDebugTranslationClientMethod);

        QObject::connect(plugin, &QmlPreviewPlugin::updatePreviews,
                         &task, &QmlPreviewConnectionManager::loadFile);
        QObject::connect(plugin, &QmlPreviewPlugin::rerunPreviews,
                         &task, &QmlPreviewConnectionManager::rerun);
        QObject::connect(plugin, &QmlPreviewPlugin::zoomFactorChanged,
                         &task, &QmlPreviewConnectionManager::zoom);
        QObject::connect(plugin, &QmlPreviewPlugin::localeIsoCodeChanged,
                         &task, &QmlPreviewConnectionManager::language);

        QObject::connect(&task, &QmlPreviewConnectionManager::connectionOpened,
                         &task, [task = &task, plugin, settings] {
            if (settings.zoomFactor > 0)
                task->zoom(settings.zoomFactor);
            if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current()) {
                if (!multiLanguageAspect->currentLocale().isEmpty())
                    task->language(multiLanguageAspect->currentLocale());
            }
            plugin->previewCurrentFile();
        });

        QObject::connect(&task, &QmlPreviewConnectionManager::restart, runControl, [runControl] {
            if (!runControl->isRunning())
                return;

            QObject::connect(runControl, &RunControl::stopped,
                             ProjectExplorerPlugin::instance(), [runControl] {
                auto rc = new RunControl(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
                rc->copyDataFromRunControl(runControl);
                rc->start();
            });

            runControl->initiateStop();
        });

        task.setBuildConfiguration(runControl->buildConfiguration());
        task.setServer(runControl->qmlChannel());
        plugin->addPreview(runControl);
    };

    const auto onDone = [runControl] {
        QmlPreviewPlugin::instance()->removePreview(runControl);
    };

    return Group {
        parallel,
        onGroupSetup([] { emit runStorage()->started(); }),
        AsyncTask<void>(onTranslationSetup),
        QmlPreviewConnectionManagerTask(onPreviewSetup),
        onGroupDone(onDone)
    }.withCancel(canceler());
}

QmlPreviewRunWorkerFactory::QmlPreviewRunWorkerFactory()
{
    setRecipeProducer(qmlPreviewRecipe);
    addSupportedRunMode(Constants::QML_PREVIEW_RUNNER);
}

LocalQmlPreviewSupportFactory::LocalQmlPreviewSupportFactory()
{
    setId(ProjectExplorer::Constants::QML_PREVIEW_RUN_FACTORY);
    setProducer([](RunControl *runControl) {
        runControl->setQmlChannel(Utils::urlFromLocalSocket());

        // Create QmlPreviewRunner
        RunWorker *preview = runControl->createWorker(ProjectExplorer::Constants::QML_PREVIEW_RUNNER);

        const auto modifier = [runControl](Process &process) {
            CommandLine cmd = runControl->commandLine();

            if (const auto aspect = runControl->aspectData<QmlProjectManager::QmlMainFileAspect>()) {
                const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
                    runControl->buildConfiguration()->buildSystem());
                QTC_ASSERT(qmlBuildSystem, return SetupResult::StopWithError);

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
            process.setCommand(cmd.toLocal());
            return SetupResult::Continue;
        };
        auto worker = createProcessWorker(runControl, modifier);
        worker->addStopDependency(preview);
        worker->addStartDependency(preview);
        return worker;
    });
    addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addSupportForLocalRunConfigs();
}

} // QmlPreview
