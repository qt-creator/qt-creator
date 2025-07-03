// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlpuppetpaths.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <QCoreApplication>

namespace QmlDesigner {
namespace QmlPuppetPaths {

namespace {

Utils::FilePath qmlPuppetExecutablePath(const Utils::FilePath &workingDirectory)
{
    return workingDirectory
        .pathAppended(QString{"qmlpuppet-"} + QCoreApplication::applicationVersion())
        .withExecutableSuffix();
}

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetFallbackPaths()
{
    auto workingDirectory = Core::ICore::libexecPath();
    return {workingDirectory, qmlPuppetExecutablePath(workingDirectory)};
}

std::pair<Utils::FilePath, Utils::FilePath> pathsForKitPuppet(ProjectExplorer::Target *target)
{
    if (!target || !target->kit())
        return {};

    QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());

    if (currentQtVersion) {
        auto path = currentQtVersion->binPath();
        return {path, qmlPuppetExecutablePath(path)};
    }

    return {};
}
} // namespace

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetPaths(ProjectExplorer::Target *target)
{
    auto [workingDirectoryPath, puppetPath] = pathsForKitPuppet(target);

    if (workingDirectoryPath.isEmpty() || !puppetPath.exists())
        return qmlPuppetFallbackPaths();

    return {workingDirectoryPath, puppetPath};
}

} // namespace QmlPuppetPaths
} // namespace QmlDesigner
