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

namespace Utils {
class Environment;
class FilePath;
class Process;
}

QT_BEGIN_NAMESPACE
class QJsonObject;

namespace QtTaskTree {
class ExecutableItem;
template <typename StorageStruct>
class Storage;
}
QT_END_NAMESPACE

namespace Android::Internal {

// The JDK a build runs Java with: the JAVA_HOME of the environment \a env it
// runs in, which the kit, the environment of the kit and the build
// configuration have all had their say in, and the JDK the kit \a k names, or
// the one of the Android settings, where that environment carries none.
// Everything a build runs is to be handed the same one - CMake and Gradle
// read JAVA_HOME, and androiddeployqt is told of it.
Utils::FilePath javaHomeForBuild(const Utils::Environment &env,
                                 const ProjectExplorer::Kit *k);

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

QtTaskTree::ExecutableItem serialNumberRecipe(const QString &avdName,
                                           const QtTaskTree::Storage<QString> &serialNumberStorage);

} // namespace Android::Internal
