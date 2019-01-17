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

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/port.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

namespace QmlPreview {

static const QString QmlServerUrl = "QmlServerUrl";

QmlPreviewRunner::QmlPreviewRunner(ProjectExplorer::RunControl *runControl,
                                   QmlPreviewFileLoader fileLoader,
                                   QmlPreviewFileClassifier fileClassifier,
                                   QmlPreviewFpsHandler fpsHandler,
                                   float initialZoom,
                                   const QString &initialLocale)
    : RunWorker(runControl)
{
    setId("QmlPreviewRunner");
    m_connectionManager.reset(new Internal::QmlPreviewConnectionManager(this));
    m_connectionManager->setFileLoader(fileLoader);
    m_connectionManager->setFileClassifier(fileClassifier);
    m_connectionManager->setFpsHandler(fpsHandler);

    connect(this, &QmlPreviewRunner::loadFile,
            m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::loadFile);
    connect(this, &QmlPreviewRunner::rerun,
            m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::rerun);

    connect(this, &QmlPreviewRunner::zoom,
            m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::zoom);
    connect(this, &QmlPreviewRunner::language,
            m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::language);
    connect(m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::connectionOpened,
            this, [this, initialZoom, initialLocale]() {
        if (initialZoom > 0)
            emit zoom(initialZoom);
        if (!initialLocale.isEmpty())
            emit language(initialLocale);
        emit ready();
    });

    connect(m_connectionManager.data(), &Internal::QmlPreviewConnectionManager::restart,
            runControl, [runControl, this]() {
        if (!runControl->isRunning())
            return;

        ProjectExplorer::RunConfiguration *runConfig = runControl->runConfiguration();
        connect(runControl, &ProjectExplorer::RunControl::stopped, runConfig, [runConfig](){
            ProjectExplorer::ProjectExplorerPlugin::runRunConfiguration(
                        runConfig, ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE, true);
        });

        runControl->initiateStop();
    });
}

void QmlPreviewRunner::start()
{
    ProjectExplorer::Target *target = nullptr;
    if (ProjectExplorer::RunConfiguration *config = runControl()->runConfiguration())
        target = config->target();
    m_connectionManager->setTarget(target);
    m_connectionManager->connectToServer(serverUrl());
    reportStarted();
}

void QmlPreviewRunner::stop()
{
    m_connectionManager->disconnectFromServer();
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
                runControl->createWorker(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE));
    preview->setServerUrl(serverUrl);

    addStopDependency(preview);
    addStartDependency(preview);

    ProjectExplorer::Runnable run = runnable();

    Utils::QtcProcess::addArg(&run.commandLineArguments,
                              QmlDebug::qmlDebugLocalArguments(QmlDebug::QmlPreviewServices,
                                                               serverUrl.path()));
    setRunnable(run);
}

} // namespace QmlPreview
