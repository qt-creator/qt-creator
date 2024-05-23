// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

class QWidget;

namespace Utils {

QTCREATOR_UTILS_EXPORT void setWheelScrollingWithoutFocusBlocked(QWidget *widget);

QTCREATOR_UTILS_EXPORT QWidget *dialogParent(QWidget *parent);
QTCREATOR_UTILS_EXPORT void setDialogParentGetter(QWidget *(*getter)());

} // namespace Utils
