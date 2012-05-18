/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QEMURUNTIMEMANAGER_H
#define QEMURUNTIMEMANAGER_H

#include "maemoconstants.h"
#include "maemoqemuruntime.h"

#include <QMap>
#include <QObject>
#include <QProcess>

#include <QIcon>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QStringList)

namespace ProjectExplorer {
class BuildConfiguration;
class Project;
class RunConfiguration;
class Target;
}

namespace QtSupport { class BaseQtVersion; }
namespace RemoteLinux { class RemoteLinuxRunConfiguration; }
namespace Utils { class FileSystemWatcher; }

namespace Madde {
namespace Internal {

class MaemoQemuManager : public QObject
{
    Q_OBJECT

public:
    static MaemoQemuManager& instance(QObject *parent = 0);

    bool runtimeForQtVersion(int uniqueId, MaemoQemuRuntime *rt) const;
    bool qemuIsRunning() const;
    Q_SLOT void startRuntime();

signals:
    void qemuProcessStatus(QemuStatus, const QString &error = QString());

private slots:
    void qtVersionsChanged(const QList<int> &addedIds, const QList<int> &removedIds, const QList<int> &changed);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
    void projectChanged(ProjectExplorer::Project *project);

    void targetAdded(ProjectExplorer::Target *target);
    void targetRemoved(ProjectExplorer::Target *target);
    void targetChanged(ProjectExplorer::Target *target);

    void runConfigurationAdded(ProjectExplorer::RunConfiguration *rc);
    void runConfigurationRemoved(ProjectExplorer::RunConfiguration *rc);
    void runConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void buildConfigurationAdded(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationRemoved(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);

    void environmentChanged();  // needed to check for qt version
    void deviceConfigurationChanged(ProjectExplorer::Target *target);

    void terminateRuntime();

    void qemuProcessFinished();
    void qemuProcessError(QProcess::ProcessError error);
    void qemuStatusChanged(QemuStatus status, const QString &error);
    void qemuOutput();

    void runtimeRootChanged(const QString &directory);
    void runtimeFolderChanged(const QString &directory);

private:
    MaemoQemuManager(QObject *parent);
    ~MaemoQemuManager();

    bool sessionHasMaemoTarget() const;

    void updateStarterIcon(bool running);
    void toggleStarterButton(ProjectExplorer::Target *target);
    bool targetUsesMatchingRuntimeConfig(ProjectExplorer::Target *target,
        QtSupport::BaseQtVersion **qtVersion = 0);

    void notify(const QList<int> uniqueIds);
    void toggleDeviceConnections(RemoteLinux::RemoteLinuxRunConfiguration *mrc, bool connect);
    void showOrHideQemuButton();

private:
    QAction *m_qemuAction;
    QProcess *m_qemuProcess;
    Utils::FileSystemWatcher *runtimeRootWatcher();
    Utils::FileSystemWatcher *runtimeFolderWatcher();

    int m_runningQtId;
    bool m_userTerminated;
    QIcon m_qemuStarterIcon;
    QMap<int, MaemoQemuRuntime> m_runtimes;
    static MaemoQemuManager *m_instance;
    Utils::FileSystemWatcher *m_runtimeRootWatcher;
    Utils::FileSystemWatcher *m_runtimeFolderWatcher;
};

}   // namespace Internal
}   // namespace Madde

#endif  // QEMURUNTIMEMANAGER_H
