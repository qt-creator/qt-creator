// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace ProjectExplorer {
class BuildConfiguration;
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

QString packageName(const ProjectExplorer::BuildConfiguration *bc);
QString activityName(const ProjectExplorer::BuildConfiguration *bc);

QString deviceSerialNumber(const ProjectExplorer::BuildConfiguration *bc);
void setDeviceSerialNumber(ProjectExplorer::BuildConfiguration *bc,
                           const QString &deviceSerialNumber);

QString apkDevicePreferredAbi(const ProjectExplorer::BuildConfiguration *bc);
void setDeviceAbis(ProjectExplorer::BuildConfiguration *bc, const QStringList &deviceAbis);

int deviceApiLevel(const ProjectExplorer::BuildConfiguration *bc);
void setDeviceApiLevel(ProjectExplorer::BuildConfiguration *bc, int level);

QString buildTargetSDK(const ProjectExplorer::BuildConfiguration *bc);

int minimumSDK(const ProjectExplorer::BuildConfiguration *bc);
int minimumSDK(const ProjectExplorer::Kit *kit);
int defaultMinimumSDK(const QtSupport::QtVersion *qtVersion);

QStringList applicationAbis(const ProjectExplorer::Kit *k);
QString archTriplet(const QString &abi);

bool isQt5CmakeProject(const ProjectExplorer::Target *target);

Utils::FilePath androidBuildDirectory(const ProjectExplorer::BuildConfiguration *bc);
Utils::FilePath buildDirectory(const ProjectExplorer::BuildConfiguration *bc);
Utils::FilePath manifestPath(const ProjectExplorer::BuildConfiguration *bc);
void setManifestPath(ProjectExplorer::BuildConfiguration *bc, const Utils::FilePath &path);
ProjectExplorer::Abi androidAbi2Abi(const QString &androidAbi);
bool skipInstallationAndPackageSteps(const ProjectExplorer::BuildConfiguration *bc);

QString androidNameForApiLevel(int x);

QJsonObject deploymentSettings(const ProjectExplorer::Kit *k);
bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

QStringList adbSelector(const QString &serialNumber);

Tasking::ExecutableItem serialNumberRecipe(const QString &avdName,
                                           const Tasking::Storage<QString> &serialNumberStorage);
Tasking::ExecutableItem startAvdRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);

} // namespace Android::Internal
