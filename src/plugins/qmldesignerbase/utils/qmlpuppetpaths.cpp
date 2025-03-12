// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlpuppetpaths.h"

#include "designersettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
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

Utils::FilePath qmlPuppetFallbackDirectory(const DesignerSettings &settings)
{
    auto puppetFallbackDirectory = Utils::FilePath::fromString(
        settings.value(DesignerSettingsKey::PUPPET_DEFAULT_DIRECTORY).toString());
    if (puppetFallbackDirectory.isEmpty() || !puppetFallbackDirectory.exists())
        return Core::ICore::libexecPath();
    return puppetFallbackDirectory;
}

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetFallbackPaths(const DesignerSettings &settings)
{
    auto workingDirectory = qmlPuppetFallbackDirectory(settings);

    return {workingDirectory, qmlPuppetExecutablePath(workingDirectory)};
}

std::pair<Utils::FilePath, Utils::FilePath> pathsForKitPuppet(ProjectExplorer::Kit *kit)
{
    if (!kit)
        return {};

    QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(kit);

    if (currentQtVersion) {
        auto path = currentQtVersion->binPath();
        return {path, qmlPuppetExecutablePath(path)};
    }

    return {};
}
} // namespace

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetPaths(ProjectExplorer::Kit *kit,
                                                           const DesignerSettings &settings)
{
    auto [workingDirectoryPath, puppetPath] = pathsForKitPuppet(kit);

    if (workingDirectoryPath.isEmpty() || !puppetPath.exists())
        return qmlPuppetFallbackPaths(settings);

    return {workingDirectoryPath, puppetPath};
}

} // namespace QmlPuppetPaths
} // namespace QmlDesigner
