/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "androidbuildapkstep.h"
#include "androidconfigurations.h"

#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

namespace Utils { class QtcProcess; }

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

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product) override;
};

class AndroidDeployQtStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidDeployQtStepFactory;

    enum DeployErrorCode
    {
        NoError = 0,
        InconsistentCertificates = 0x0001,
        UpdateIncompatible = 0x0002,
        PermissionModelDowngrade = 0x0004,
        VersionDowngrade = 0x0008,
        Failure = 0x0010
    };

public:
    enum UninstallType {
        Keep,
        Uninstall,
        ForceUnintall
    };
public:
    explicit AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool runInGuiThread() const override;

    UninstallType uninstallPreviousPackage();

    AndroidDeviceInfo deviceInfo() const;

    void setUninstallPreviousPackage(bool uninstall);

signals:
    void askForUninstall(DeployErrorCode errorCode);
    void setSerialNumber(const QString &serialNumber);

private:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc, AndroidDeployQtStep *other);
    void ctor();
    void runCommand(const QString &program, const QStringList &arguments);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    DeployErrorCode runDeploy(QFutureInterface<bool> &fi);
    void slotAskForUninstall(DeployErrorCode errorCode);
    void slotSetSerialNumber(const QString &serialNumber);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override { return true; }

    void processReadyReadStdOutput(DeployErrorCode &errorCode);
    void stdOutput(const QString &line);
    void processReadyReadStdError(DeployErrorCode &errorCode);
    void stdError(const QString &line);
    DeployErrorCode parseDeployErrors(QString &deployOutputLine) const;

    void slotProcessFinished(int, QProcess::ExitStatus);
    void processFinished(int exitCode, QProcess::ExitStatus status);

    friend void operator|=(DeployErrorCode &e1, const DeployErrorCode &e2) { e1 = static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2); }
    friend DeployErrorCode operator|(const DeployErrorCode &e1, const DeployErrorCode &e2) { return static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2); }

    Utils::FileName m_manifestName;
    QString m_serialNumber;
    QString m_buildDirectory;
    QString m_avdName;
    QString m_apkPath;
    QStringList m_appProcessBinaries;
    QString m_libdir;

    QString m_targetArch;
    bool m_uninstallPreviousPackage;
    bool m_uninstallPreviousPackageRun;
    bool m_useAndroiddeployqt;
    bool m_askForUinstall;
    static const Core::Id Id;
    QString m_androiddeployqtArgs;
    QString m_adbPath;
    QString m_command;
    QString m_workingDirectory;
    Utils::Environment m_environment;
    Utils::QtcProcess *m_process;
    AndroidDeviceInfo m_deviceInfo;
};

}
} // namespace Android
