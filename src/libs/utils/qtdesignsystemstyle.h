// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QCommonStyle>

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtDesignSystemStyle : public QCommonStyle
{
public:
    QtDesignSystemStyle();
    ~QtDesignSystemStyle();

    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *painter,
                     const QWidget *widget = nullptr) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *opt, QPainter *painter,
                       const QWidget *widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *opt = nullptr,
                    const QWidget *widget = nullptr) const override;
    QIcon standardIcon(StandardPixmap sp, const QStyleOption *opt,
                       const QWidget *widget) const override;
    void polish(QWidget *widget) override;

    static QStyle *instance();
};

} // namespace Utils
