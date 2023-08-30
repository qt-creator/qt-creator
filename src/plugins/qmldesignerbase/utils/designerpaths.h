// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <utils/filepath.h>

namespace QmlDesigner::Paths {

constexpr char exampleDownloadPath[] = "StudioConfig/ExamplesDownloadPath";
constexpr char bundlesDownloadPath[] = "StudioConfig/BundlesDownloadPath";

QMLDESIGNERBASE_EXPORT Utils::FilePath defaultExamplesPath();
QMLDESIGNERBASE_EXPORT Utils::FilePath defaultBundlesPath();
QMLDESIGNERBASE_EXPORT QString examplesPathSetting();
QMLDESIGNERBASE_EXPORT QString bundlesPathSetting();

} // namespace QmlDesigner::Paths
