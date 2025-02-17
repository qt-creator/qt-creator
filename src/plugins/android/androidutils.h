// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace ProjectExplorer {
class Abi;
class Kit;
class Target;
}

namespace QtSupport { class QtVersion; }

namespace Tasking {
class ExecutableItem;
template <typename StorageStruct>
class Storage;
}

namespace Utils {
class FilePath;
class Process;
}

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

namespace Android::Internal {

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
ProjectExplorer::Abi androidAbi2Abi(const QString &androidAbi);
bool skipInstallationAndPackageSteps(const ProjectExplorer::Target *target);

QString androidNameForApiLevel(int x);

QJsonObject deploymentSettings(const ProjectExplorer::Target *target);
bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

QStringList adbSelector(const QString &serialNumber);

Tasking::ExecutableItem serialNumberRecipe(const QString &avdName,
                                           const Tasking::Storage<QString> &serialNumberStorage);
Tasking::ExecutableItem startAvdRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);

} // namespace Android::Internal
