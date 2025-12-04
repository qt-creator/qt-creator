// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesigner_global.h"

#include <utils/filepath.h>

namespace ProjectExplorer {
class Kit;
}
namespace QmlDesigner {

namespace QmlPuppetPaths {
QMLDESIGNER_EXPORT std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetPaths(ProjectExplorer::Kit *kit);
}
} // namespace QmlDesigner
