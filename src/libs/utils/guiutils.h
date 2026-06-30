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
QTCREATOR_UTILS_EXPORT bool setIgnoreForDirtyHook(QObject *object, bool ignore = true);
QTCREATOR_UTILS_EXPORT bool isIgnoredForDirtyHook(const QObject *object);

QTCREATOR_UTILS_EXPORT void markSettingsDirty();
QTCREATOR_UTILS_EXPORT void checkSettingsDirty();

// Adjusts the suppression nesting level (positive = push, otherwise pop).
// Prefer DirtySettingsGuard RAII-objects. Returns whether suppression was active before the call.
QTCREATOR_UTILS_EXPORT bool suppressSettingsDirtyTrigger(bool suppress);

// Disables use of dirty hooks while active. Reference-counted, so overlapping
// guards (even from different threads) no longer strand the suppression state.
class QTCREATOR_UTILS_EXPORT DirtySettingsGuard
{
public:
    DirtySettingsGuard() { suppressSettingsDirtyTrigger(true); }
    ~DirtySettingsGuard() { suppressSettingsDirtyTrigger(false); }
};

QTCREATOR_UTILS_EXPORT void installCheckSettingsDirtyTrigger(QObject *object);

QTCREATOR_UTILS_EXPORT void installMarkSettingsDirtyTrigger(QObject *object);
QTCREATOR_UTILS_EXPORT void installMarkSettingsDirtyTriggerRecursively(QObject *object); // Avoid.

namespace Internal {
QTCREATOR_UTILS_EXPORT void setMarkSettingsDirtyHook(const std::function<void (bool)> &hook);
QTCREATOR_UTILS_EXPORT void setCheckSettingsDirtyHook(const std::function<void ()> &hook);
}

} // namespace Utils
