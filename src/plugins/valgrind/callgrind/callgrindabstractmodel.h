// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemView>

namespace Valgrind::Callgrind {

class ParseData;

enum AbstractModelRoles
{
    ParentCostRole = Qt::UserRole,
    RelativeTotalCostRole,
    RelativeParentCostRole,
    NextCustomRole
};

} // Valgrind::Internal
