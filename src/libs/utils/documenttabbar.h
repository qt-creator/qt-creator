// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QTabBar>

namespace Utils {

class QTCREATOR_UTILS_EXPORT DocumentTabBar : public QTabBar
{
public:
    using QTabBar::QTabBar;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

} // namespace Utils
