// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"

#include <projectexplorer/runcontrol.h>

#include <utils/qtcprocess.h>

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
    const AndroidConfig &m_androidConfig;
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
