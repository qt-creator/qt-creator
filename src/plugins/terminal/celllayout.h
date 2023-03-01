// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <vterm.h>

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>
#include <QTextLayout>

#include <functional>

namespace Terminal::Internal {

QColor toQColor(const VTermColor &c);

void createTextLayout(QTextLayout &textLayout,
                      std::u32string *resultText,
                      VTermColor defaultBg,
                      QRect cellRect,
                      qreal lineSpacing,
                      std::function<const VTermScreenCell *(int x, int y)> fetchCell);

std::u32string cellToString(const VTermScreenCell &cell);

} // namespace Terminal::Internal
