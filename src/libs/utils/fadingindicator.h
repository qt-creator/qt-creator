// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Utils {
namespace FadingIndicator {

enum TextSize {
    SmallText,
    LargeText
};

QTCREATOR_UTILS_EXPORT void showText(QWidget *parent, const QString &text,
                                     TextSize size = LargeText);
QTCREATOR_UTILS_EXPORT void showPixmap(QWidget *parent, const QString &pixmap);

} // FadingIndicator
} // Utils
