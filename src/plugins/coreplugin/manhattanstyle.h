// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QProxyStyle>

class ManhattanStylePrivate;

class CORE_EXPORT ManhattanStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit ManhattanStyle(const QString &baseStyleName);

    virtual ~ManhattanStyle() override;

    void drawPrimitive(
            PrimitiveElement element,
            const QStyleOption *option,
            QPainter *painter,
            const QWidget *widget = nullptr) const override;

    void drawControl(
            ControlElement element,
            const QStyleOption *option,
            QPainter *painter,
            const QWidget *widget = nullptr) const override;

    void drawComplexControl(
            ComplexControl control,
            const QStyleOptionComplex *option,
            QPainter *painter,
            const QWidget *widget = nullptr) const override;

    QSize sizeFromContents(
            ContentsType type,
            const QStyleOption *option,
            const QSize &size,
            const QWidget *widget) const override;

    QRect subElementRect(
            SubElement element,
            const QStyleOption *option,
            const QWidget *widget) const override;

    QRect subControlRect(
            ComplexControl control,
            const QStyleOptionComplex *option,
            SubControl subControl,
            const QWidget *widget) const override;

    int styleHint(
            StyleHint hint,
            const QStyleOption *option = nullptr,
            const QWidget *widget = nullptr,
            QStyleHintReturn *returnData = nullptr) const override;

    int pixelMetric(
            PixelMetric metric,
            const QStyleOption *option = nullptr,
            const QWidget *widget = nullptr) const override;

    QPalette standardPalette() const override;


    QIcon standardIcon(
            StandardPixmap standardIcon,
            const QStyleOption *option = nullptr,
            const QWidget *widget = nullptr) const override;

    SubControl hitTestComplexControl(
            ComplexControl control,
            const QStyleOptionComplex *option,
            const QPoint &pos,
            const QWidget *widget = nullptr) const override;

    QPixmap standardPixmap(
            StandardPixmap standardPixmap,
            const QStyleOption *opt,
            const QWidget *widget = nullptr) const override;

    QPixmap generatedIconPixmap(
            QIcon::Mode iconMode,
            const QPixmap &pixmap,
            const QStyleOption *opt) const override;

    void polish(QWidget *widget) override;
    void polish(QPalette &pal) override;
    void polish(QApplication *app) override;

    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

private:
    void drawPrimitiveForPanelWidget(
            PrimitiveElement element,
            const QStyleOption *option,
            QPainter *painter,
            const QWidget *widget) const;

    static void drawButtonSeparator(
            QPainter *painter,
            const QRect &rect,
            bool reverse);

    ManhattanStylePrivate *d = nullptr;
};
