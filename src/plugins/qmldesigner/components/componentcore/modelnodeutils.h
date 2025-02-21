// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

namespace QmlDesigner::ModelNodeUtils {
void backupPropertyAndRemove(const ModelNode &node,
                             const PropertyName &propertyName,
                             Utils::SmallStringView auxName);
void restoreProperty(const ModelNode &node,
                     const PropertyName &propertyName,
                     Utils::SmallStringView auxName);
} // namespace QmlDesigner::ModelNodeUtils
