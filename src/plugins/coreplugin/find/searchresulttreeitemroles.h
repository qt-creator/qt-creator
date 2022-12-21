// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemView>

namespace Core {
namespace Internal {
namespace ItemDataRoles {

enum Roles {
    ResultItemRole = Qt::UserRole,
    ResultLineRole,
    ResultBeginLineNumberRole,
    ResultIconRole,
    ResultHighlightBackgroundColor,
    ResultHighlightForegroundColor,
    FunctionHighlightBackgroundColor,
    FunctionHighlightForegroundColor,
    ResultBeginColumnNumberRole,
    SearchTermLengthRole,
    ContainingFunctionNameRole,
    IsGeneratedRole
};

} // namespace Internal
} // namespace Core
} // namespace ItemDataRoles
