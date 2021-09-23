/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once
#include "androidconfigurations.h"

#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>

#include <QFutureWatcher>

namespace Android {
class SdkToolResult;

namespace Internal {

class UploadInfo
{
public:
    Utils::FilePath uploadPackage;
    Utils::FilePath projectFolder;
};

class AndroidQmlPreviewWorker : public ProjectExplorer::RunWorker
{
    Q_OBJECT
public:
    AndroidQmlPreviewWorker(ProjectExplorer::RunControl *runControl);
    ~AndroidQmlPreviewWorker();

signals:
    void previewPidChanged();

private:
    void start() override;
    void stop() override;

    bool ensureAvdIsRunning();
    bool checkAndInstallPreviewApp();
    bool preparePreviewArtefacts();
    bool uploadPreviewArtefacts();

    SdkToolResult runAdbCommand(const QStringList &arguments) const;
    SdkToolResult runAdbShellCommand(const QStringList &arguments) const;
    int pidofPreview() const;
    bool isPreviewRunning(int lastKnownPid = -1) const;

    void startPidWatcher();
    void startLogcat();
    void filterLogcatAndAppendMessage(const QString &stdOut);

    bool startPreviewApp();
    bool stopPreviewApp();

    Utils::FilePath designViewerApkPath(const QString &abi) const;
    Utils::FilePath createQmlrcFile(const Utils::FilePath &workFolder, const QString &basename);

    ProjectExplorer::RunControl *m_rc = nullptr;
    AndroidConfig m_androidConfig;
    QString m_serialNumber;
    QStringList m_avdAbis;
    int m_viewerPid = -1;
    QFutureWatcher<void> m_pidFutureWatcher;
    Utils::QtcProcess m_logcatProcess;
    QString m_logcatStartTimeStamp;
    UploadInfo m_uploadInfo;
};

} // namespace Internal
} // namespace Android
