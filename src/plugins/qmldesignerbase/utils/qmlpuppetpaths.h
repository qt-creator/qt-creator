// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <utils/filepath.h>

namespace ProjectExplorer {
class Kit;
}
namespace QmlDesigner {
class DesignerSettings;

namespace QmlPuppetPaths {
QMLDESIGNERBASE_EXPORT std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetPaths(
    ProjectExplorer::Kit *kit, const DesignerSettings &settings);
}
} // namespace QmlDesigner
