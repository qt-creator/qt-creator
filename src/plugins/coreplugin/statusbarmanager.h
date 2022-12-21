// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"

namespace Core {

class CORE_EXPORT StatusBarManager
{
public:
    enum StatusBarPosition { First=0, Second=1, Third=2, LastLeftAligned=Third, RightCorner};

    static void addStatusBarWidget(QWidget *widget,
                                   StatusBarPosition position,
                                   const Context &ctx = Context());
    static void destroyStatusBarWidget(QWidget *widget);
    static void restoreSettings();
};

} // namespace Core
