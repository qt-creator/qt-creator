// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designerpaths.h"

#include <coreplugin/icore.h>

#include <QStandardPaths>

namespace QmlDesigner::Paths {

Utils::FilePath defaultExamplesPath()
{
    QStandardPaths::StandardLocation location = QStandardPaths::DocumentsLocation;

    return Utils::FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio/examples");
}

QString examplesPathSetting()
{
    return Core::ICore::settings()->value(exampleDownloadPath, defaultExamplesPath().toUrlishString())
        .toString();
}

} // namespace QmlDesigner::Paths
