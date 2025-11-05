// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesigner_global.h"

#include <utils/filepath.h>

namespace QmlDesigner::Paths {

constexpr char exampleDownloadPath[] = "StudioConfig/ExamplesDownloadPath";
constexpr char bundlesDownloadPath[] = "StudioConfig/BundlesDownloadPath";

QMLDESIGNER_EXPORT Utils::FilePath defaultExamplesPath();
QMLDESIGNER_EXPORT Utils::FilePath defaultBundlesPath();
QMLDESIGNER_EXPORT QString examplesPathSetting();
QMLDESIGNER_EXPORT QString bundlesPathSetting();

} // namespace QmlDesigner::Paths
