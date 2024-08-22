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

namespace AndroidManager
{

// With Qt 5.4, a couple of android tooling features moved over from Qt Creator to Qt.
constexpr auto firstQtWithAndroidDeployQt = {5, 4, 0};

QString packageName(const ProjectExplorer::Target *target);
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

bool checkKeystorePassword(const Utils::FilePath &keystorePath,
                           const QString &keystorePasswd);
bool checkCertificatePassword(const Utils::FilePath &keystorePath,
                              const QString &keystorePasswd,
                              const QString &alias, const QString &certificatePasswd);
bool checkCertificateExists(const Utils::FilePath &keystorePath,
                            const QString &keystorePasswd, const QString &alias);

QJsonObject deploymentSettings(const ProjectExplorer::Target *target);
bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

} // namespace AndroidManager
} // namespace Android
