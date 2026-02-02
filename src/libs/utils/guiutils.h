// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <functional>

QT_BEGIN_NAMESPACE
class QObject;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

QTCREATOR_UTILS_EXPORT void setWheelScrollingWithoutFocusBlocked(QWidget *widget);

QTCREATOR_UTILS_EXPORT QWidget *dialogParent();
QTCREATOR_UTILS_EXPORT void setDialogParentGetter(QWidget *(*getter)());

// returns previous value
QTCREATOR_UTILS_EXPORT bool setIgnoreForDirtyHook(QWidget *widget, bool ignore = true);
QTCREATOR_UTILS_EXPORT bool isIgnoredForDirtyHook(const QObject *object);


QTCREATOR_UTILS_EXPORT void markSettingsDirty(bool dirty = true);
QTCREATOR_UTILS_EXPORT void checkSettingsDirty();

namespace Internal {
QTCREATOR_UTILS_EXPORT void setMarkSettingsDirtyHook(const std::function<void (bool)> &hook);
QTCREATOR_UTILS_EXPORT void setCheckSettingsDirtyHook(const std::function<void ()> &hook);
}

} // namespace Utils
