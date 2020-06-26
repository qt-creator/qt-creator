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

#include <utils/environment.h>
#include <utils/qtcprocess.h>

namespace Android {
namespace Internal {

class AndroidDeployQtStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    AndroidDeployQtStepFactory();
};

class AndroidDeployQtStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

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

    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

    static Utils::Id stepId();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    UninstallType uninstallPreviousPackage();
    void setUninstallPreviousPackage(bool uninstall);

signals:
    void askForUninstall(DeployErrorCode errorCode);
    void setSerialNumber(const QString &serialNumber);

private:
    void runCommand(const Utils::CommandLine &command);

    bool init() override;
    void doRun() override;
    void gatherFilesToPull();
    DeployErrorCode runDeploy();
    void slotAskForUninstall(DeployErrorCode errorCode);
    void slotSetSerialNumber(const QString &serialNumber);

    bool runImpl();

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    void processReadyReadStdOutput(DeployErrorCode &errorCode);
    void stdOutput(const QString &line);
    void processReadyReadStdError(DeployErrorCode &errorCode);
    void stdError(const QString &line);
    DeployErrorCode parseDeployErrors(QString &deployOutputLine) const;

    void slotProcessFinished(int, QProcess::ExitStatus);
    void processFinished(int exitCode, QProcess::ExitStatus status);

    friend void operator|=(DeployErrorCode &e1, const DeployErrorCode &e2) { e1 = static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2); }
    friend DeployErrorCode operator|(const DeployErrorCode &e1, const DeployErrorCode &e2) { return static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2); }

    Utils::FilePath m_manifestName;
    QString m_serialNumber;
    QString m_avdName;
    Utils::FilePath m_apkPath;
    QMap<QString, QString> m_filesToPull;

    QStringList m_androidABIs;
    bool m_uninstallPreviousPackage = false;
    bool m_uninstallPreviousPackageRun = false;
    bool m_useAndroiddeployqt = false;
    bool m_askForUninstall = false;
    static const Utils::Id Id;
    Utils::CommandLine m_androiddeployqtArgs;
    Utils::FilePath m_adbPath;
    Utils::FilePath m_command;
    Utils::FilePath m_workingDirectory;
    Utils::Environment m_environment;
    Utils::QtcProcess *m_process = nullptr;
    AndroidDeviceInfo m_deviceInfo;
};

}
} // namespace Android
