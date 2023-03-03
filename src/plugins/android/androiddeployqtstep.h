// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androiddeviceinfo.h"

#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/futuresynchronizer.h>

namespace Utils { class QtcProcess; }

namespace Android::Internal {

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
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

signals:
    void askForUninstall(DeployErrorCode errorCode);

private:
    void runCommand(const Utils::CommandLine &command);

    bool init() override;
    void doRun() override;
    void doCancel() override;
    void gatherFilesToPull();
    DeployErrorCode runDeploy();
    void slotAskForUninstall(DeployErrorCode errorCode);

    void runImpl(QPromise<bool> &promise);

    QWidget *createConfigWidget() override;

    void processReadyReadStdOutput(DeployErrorCode &errorCode);
    void stdOutput(const QString &line);
    void processReadyReadStdError(DeployErrorCode &errorCode);
    void stdError(const QString &line);
    DeployErrorCode parseDeployErrors(const QString &deployOutputLine) const;

    friend void operator|=(DeployErrorCode &e1, const DeployErrorCode &e2) {
        e1 = static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2);
    }

    friend DeployErrorCode operator|(const DeployErrorCode &e1, const DeployErrorCode &e2) {
        return static_cast<AndroidDeployQtStep::DeployErrorCode>((int)e1 | (int)e2);
    }

    void reportWarningOrError(const QString &message, ProjectExplorer::Task::TaskType type);

    Utils::FilePath m_manifestName;
    QString m_serialNumber;
    QString m_avdName;
    Utils::FilePath m_apkPath;
    QMap<QString, Utils::FilePath> m_filesToPull;

    QStringList m_androidABIs;
    Utils::BoolAspect *m_uninstallPreviousPackage = nullptr;
    bool m_uninstallPreviousPackageRun = false;
    bool m_useAndroiddeployqt = false;
    bool m_askForUninstall = false;
    static const Utils::Id Id;
    Utils::CommandLine m_androiddeployqtArgs;
    Utils::FilePath m_adbPath;
    Utils::FilePath m_command;
    Utils::FilePath m_workingDirectory;
    Utils::Environment m_environment;
    AndroidDeviceInfo m_deviceInfo;

    Utils::FutureSynchronizer m_synchronizer;
};

} // Android::Internal
