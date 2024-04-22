// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Tasking { class GroupItem; }

namespace Android::Internal {

Tasking::GroupItem downloadSdkRecipe();
QString dialogTitle();

} // namespace Android::Internal
