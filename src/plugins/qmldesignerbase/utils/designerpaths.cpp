// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designerpaths.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>

#include <QStandardPaths>

namespace QmlDesigner::Paths {

Utils::FilePath defaultExamplesPath()
{
    QStandardPaths::StandardLocation location = QStandardPaths::DocumentsLocation;

    return Utils::FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio/examples");
}

Utils::FilePath defaultBundlesPath()
{
    QStandardPaths::StandardLocation location = Utils::HostOsInfo::isMacHost()
                                                    ? QStandardPaths::HomeLocation
                                                    : QStandardPaths::DocumentsLocation;

    return Utils::FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio/bundles");
}

QString examplesPathSetting()
{
    return Core::ICore::settings()->value(exampleDownloadPath, defaultExamplesPath().toString())
        .toString();
}

QString bundlesPathSetting()
{
    return Core::ICore::settings()->value(bundlesDownloadPath, defaultBundlesPath().toString())
        .toString();
}

} // namespace QmlDesigner::Paths
