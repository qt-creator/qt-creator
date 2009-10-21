/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace trk {

struct TrkResult;
struct TrkMessage;
struct LauncherPrivate;

class Launcher : public QObject
{
    Q_OBJECT
public:
    typedef void (Launcher::*TrkCallBack)(const TrkResult &);

    enum Actions {
        ActionPingOnly = 0x0,
        ActionCopy = 0x1,
        ActionInstall = 0x2,
        ActionCopyInstall = ActionCopy | ActionInstall,
        ActionRun = 0x4,
        ActionCopyRun = ActionCopy | ActionRun,
        ActionInstallRun = ActionInstall | ActionRun,
        ActionCopyInstallRun = ActionCopy | ActionInstall | ActionRun
    };

    explicit Launcher(trk::Launcher::Actions startupActions = trk::Launcher::ActionPingOnly,
                      QObject *parent = 0);
    ~Launcher();
    void addStartupActions(trk::Launcher::Actions startupActions);
    void setTrkServerName(const QString &name);
    void setFileName(const QString &name);
    void setCopyFileName(const QString &srcName, const QString &dstName);
    void setInstallFileName(const QString &name);
    bool startServer(QString *errorMessage);
    void setVerbose(int v);    
    void setSerialFrame(bool b);
    bool serialFrame() const;

    // becomes valid after successful execution of ActionPingOnly
    QString deviceDescription(unsigned verbose = 0u) const;

signals:
    void copyingStarted();
    void canNotConnect(const QString &errorMessage);
    void canNotCreateFile(const QString &filename, const QString &errorMessage);
    void canNotWriteFile(const QString &filename, const QString &errorMessage);
    void canNotCloseFile(const QString &filename, const QString &errorMessage);
    void installingStarted();
    void canNotInstall(const QString &packageFilename, const QString &errorMessage);
    void startingApplication();
    void applicationRunning(uint pid);
    void canNotRun(const QString &errorMessage);
    void finished();
    void applicationOutputReceived(const QString &output);
    void copyProgress(int percent);

public slots:
    void terminate();

private slots:
    void handleResult(const trk::TrkResult &data);

private:
    // kill process and breakpoints
    void cleanUp();
    void disconnectTrk();

    void handleRemoteProcessKilled(const TrkResult &result);
    void handleConnect(const TrkResult &result);
    void handleFileCreation(const TrkResult &result);
    void handleCopy(const TrkResult &result);
    void continueCopying(uint lastCopiedBlockSize = 0);
    void closeRemoteFile(bool failed = false);
    void handleFileCopied(const TrkResult &result);
    void handleInstallPackageFinished(const TrkResult &result);
    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleWaitForFinished(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);
    void handleTrkVersion(const TrkResult &result);

    void copyFileToRemote();
    void installRemotePackageSilently();
    void startInferiorIfNeeded();

    void logMessage(const QString &msg);

    LauncherPrivate *d;
};

} // namespace Trk

#endif // LAUNCHER_H
