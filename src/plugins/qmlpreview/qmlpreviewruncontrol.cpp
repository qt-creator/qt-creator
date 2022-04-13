/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlpreviewruncontrol.h"

#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlmainfileaspect.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/port.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>
#include <utils/fileutils.h>

namespace QmlPreview {

static const QString QmlServerUrl = "QmlServerUrl";

QmlPreviewRunner::QmlPreviewRunner(const QmlPreviewRunnerSetting &settings)
    : RunWorker(settings.runControl)
{
    setId("QmlPreviewRunner");
    m_connectionManager.setFileLoader(settings.fileLoader);
    m_connectionManager.setFileClassifier(settings.fileClassifier);
    m_connectionManager.setFpsHandler(settings.fpsHandler);
    m_connectionManager.setQmlDebugTranslationClientCreator(
        settings.createDebugTranslationClientMethod);

    connect(this, &QmlPreviewRunner::loadFile,
            &m_connectionManager, &Internal::QmlPreviewConnectionManager::loadFile);
    connect(this, &QmlPreviewRunner::rerun,
            &m_connectionManager, &Internal::QmlPreviewConnectionManager::rerun);

    connect(this, &QmlPreviewRunner::zoom,
            &m_connectionManager, &Internal::QmlPreviewConnectionManager::zoom);
    connect(this, &QmlPreviewRunner::language,
            &m_connectionManager, &Internal::QmlPreviewConnectionManager::language);

    connect(&m_connectionManager, &Internal::QmlPreviewConnectionManager::connectionOpened,
            this, [this, settings]() {
        if (settings.zoom > 0)
            emit zoom(settings.zoom);
        if (!settings.language.isEmpty())
            emit language(settings.language);

        emit ready();
    });

    connect(&m_connectionManager, &Internal::QmlPreviewConnectionManager::restart,
            runControl(), [this]() {
        if (!runControl()->isRunning())
            return;

        this->connect(runControl(), &ProjectExplorer::RunControl::stopped, [this]() {
            auto rc = new ProjectExplorer::RunControl(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
            rc->copyDataFromRunControl(runControl());
            ProjectExplorer::ProjectExplorerPlugin::startRunControl(rc);
        });

        runControl()->initiateStop();
    });
}

void QmlPreviewRunner::start()
{
    m_connectionManager.setTarget(runControl()->target());
    m_connectionManager.connectToServer(serverUrl());
    reportStarted();
}

void QmlPreviewRunner::stop()
{
    m_connectionManager.disconnectFromServer();
    reportStopped();
}

void QmlPreviewRunner::setServerUrl(const QUrl &serverUrl)
{
    recordData(QmlServerUrl, serverUrl);
}

QUrl QmlPreviewRunner::serverUrl() const
{
    return recordedData(QmlServerUrl).toUrl();
}

LocalQmlPreviewSupport::LocalQmlPreviewSupport(ProjectExplorer::RunControl *runControl)
    : SimpleTargetRunner(runControl)
{
    setId("LocalQmlPreviewSupport");
    const QUrl serverUrl = Utils::urlFromLocalSocket();

    QmlPreviewRunner *preview = qobject_cast<QmlPreviewRunner *>(
                runControl->createWorker(ProjectExplorer::Constants::QML_PREVIEW_RUNNER));
    preview->setServerUrl(serverUrl);

    addStopDependency(preview);
    addStartDependency(preview);

    setStarter([this, runControl, serverUrl] {
        ProjectExplorer::Runnable runnable = runControl->runnable();
        QStringList qmlProjectRunConfigurationArguments = runnable.command.splitArguments();

        const auto currentTarget = runControl->target();
        const auto *qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(currentTarget->buildSystem());

        if (const auto aspect = runControl->aspect<QmlProjectManager::QmlMainFileAspect>()) {
            const QString mainScript = aspect->mainScript;
            const QString currentFile = aspect->currentFile;

            const QString mainScriptFromProject = qmlBuildSystem->targetFile(
                Utils::FilePath::fromString(mainScript)).toString();

            if (!currentFile.isEmpty() && qmlProjectRunConfigurationArguments.last().contains(mainScriptFromProject)) {
                qmlProjectRunConfigurationArguments.removeLast();
                runnable.command = Utils::CommandLine(runnable.command.executable(), qmlProjectRunConfigurationArguments);
                runnable.command.addArg(currentFile);
            }
        }

        runnable.command.addArg(QmlDebug::qmlDebugLocalArguments(QmlDebug::QmlPreviewServices,
                                                                 serverUrl.path()));
        runnable.device.reset();
        doStart(runnable);
    });
}

} // namespace QmlPreview
