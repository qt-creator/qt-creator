// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

QTCREATOR_UTILS_EXPORT QAction *execMenuAtWidget(QMenu *menu, QWidget *widget);
QTCREATOR_UTILS_EXPORT void addToolTipsToMenu(QMenu *menu);

} // namespace Utils
