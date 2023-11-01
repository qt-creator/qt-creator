// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>
#include <projectexplorer/abi.h>

namespace ProjectExplorer {
class Kit;
class Target;
}

namespace Utils {
class FilePath;
class Process;
}

namespace Android {

class SdkToolResult {
public:
    SdkToolResult() = default;
    bool success() const { return m_success; }
    const QString &stdOut() const { return m_stdOut; }
    const QString &stdErr() const { return m_stdErr; }
    const QString &exitMessage() const { return m_exitMessage; }

    bool m_success = false;
    QString m_stdOut;
    QString m_stdErr;
    QString m_exitMessage;
};

namespace AndroidManager
{

QString packageName(const ProjectExplorer::Target *target);
QString packageName(const Utils::FilePath &manifestFile);
QString activityName(const ProjectExplorer::Target *target);

QString deviceSerialNumber(const ProjectExplorer::Target *target);
void setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber);

QString apkDevicePreferredAbi(const ProjectExplorer::Target *target);
void setDeviceAbis(ProjectExplorer::Target *target, const QStringList &deviceAbis);

int deviceApiLevel(const ProjectExplorer::Target *target);
void setDeviceApiLevel(ProjectExplorer::Target *target, int level);

QString buildTargetSDK(const ProjectExplorer::Target *target);

int minimumSDK(const ProjectExplorer::Target *target);
int minimumSDK(const ProjectExplorer::Kit *kit);
int defaultMinimumSDK(const QtSupport::QtVersion *qtVersion);

QStringList applicationAbis(const ProjectExplorer::Target *target);
QString archTriplet(const QString &abi);

bool isQt5CmakeProject(const ProjectExplorer::Target *target);

Utils::FilePath androidBuildDirectory(const ProjectExplorer::Target *target);
Utils::FilePath androidAppProcessDir(const ProjectExplorer::Target *target);
Utils::FilePath buildDirectory(const ProjectExplorer::Target *target);
Utils::FilePath manifestPath(const ProjectExplorer::Target *target);
void setManifestPath(ProjectExplorer::Target *target, const Utils::FilePath &path);
Utils::FilePath packagePath(const ProjectExplorer::Target *target);
ProjectExplorer::Abi androidAbi2Abi(const QString &androidAbi);
bool skipInstallationAndPackageSteps(const ProjectExplorer::Target *target);

QString androidNameForApiLevel(int x);

void installQASIPackage(ProjectExplorer::Target *target, const Utils::FilePath &packagePath);

bool checkKeystorePassword(const Utils::FilePath &keystorePath,
                           const QString &keystorePasswd);
bool checkCertificatePassword(const Utils::FilePath &keystorePath,
                              const QString &keystorePasswd,
                              const QString &alias, const QString &certificatePasswd);
bool checkCertificateExists(const Utils::FilePath &keystorePath,
                            const QString &keystorePasswd, const QString &alias);

Utils::Process *startAdbProcess(const QStringList &args, QString *err = nullptr);
SdkToolResult runAdbCommand(const QStringList &args, const QByteArray &writeData = {},
                            int timeoutS = 30);

QJsonObject deploymentSettings(const ProjectExplorer::Target *target);
bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

} // namespace AndroidManager
} // namespace Android
