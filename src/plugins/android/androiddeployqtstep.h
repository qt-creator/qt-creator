/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDDEPLOYQTSTEP_H
#define ANDROIDDEPLOYQTSTEP_H

#include "androidbuildapkstep.h"
#include "androidconfigurations.h"

#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

namespace Utils {
class QtcProcess;
}

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidDeployQtStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit AndroidDeployQtStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);

    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product);
};

class AndroidDeployQtStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidDeployQtStepFactory;
public:
    enum UninstallType {
        Keep,
        Uninstall,
        ForceUnintall
    };
public:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc);

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    bool runInGuiThread() const;

    UninstallType uninstallPreviousPackage();

public slots:
    void setUninstallPreviousPackage(bool uninstall);

signals:
    void askForUninstall();
    void setSerialNumber(const QString &serialNumber);

private:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc,
        AndroidDeployQtStep *other);
    void ctor();
    void runCommand(const QString &program, const QStringList &arguments);
    QString systemAppProcessFilePath() const;

    bool init();
    void run(QFutureInterface<bool> &fi);
    enum DeployResult { Success, Failure, AskUinstall };
    DeployResult runDeploy(QFutureInterface<bool> &fi);
    void slotAskForUninstall();
    void slotSetSerialNumber(const QString &serialNumber);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const { return true; }

    void processReadyReadStdOutput();
    void stdOutput(const QString &line);
    void processReadyReadStdError();
    void stdError(const QString &line);

    void slotProcessFinished(int, QProcess::ExitStatus);
    void processFinished(int exitCode, QProcess::ExitStatus status);

    Utils::FileName m_manifestName;
    QString m_serialNumber;
    QString m_buildDirectory;
    QString m_avdName;
    QString m_apkPath;
    QString m_appProcess;
    QString m_libdir;

    QString m_targetArch;
    bool m_uninstallPreviousPackage;
    bool m_uninstallPreviousPackageRun;
    static const Core::Id Id;
    bool m_installOk;
    bool m_useAndroiddeployqt;
    QString m_androiddeployqtArgs;
    QString m_adbPath;
    QString m_command;
    QString m_workingDirectory;
    Utils::Environment m_environment;
    Utils::QtcProcess *m_process;
    bool m_askForUinstall;
};

}
} // namespace Android

#endif // ANDROIDDEPLOYQTSTEP_H
