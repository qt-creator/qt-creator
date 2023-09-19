// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QProxyStyle>

namespace QmlDesigner {
class StudioStylePrivate;

class QMLDESIGNERBASE_EXPORT StudioStyle : public QProxyStyle
{
    Q_OBJECT

public:
    using Super = QProxyStyle;
    StudioStyle(QStyle *style = nullptr);
    StudioStyle(const QString &key);
    virtual ~StudioStyle() override;

    // Drawing Methods
    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget = nullptr) const override;

    // Topology
    QSize sizeFromContents(ContentsType type,
                           const QStyleOption *option,
                           const QSize &size,
                           const QWidget *widget) const override;

    QRect subControlRect(ComplexControl control,
                         const QStyleOptionComplex *option,
                         SubControl subControl,
                         const QWidget *widget) const override;

    int styleHint(StyleHint hint,
                  const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    int pixelMetric(PixelMetric metric,
                    const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

    QPalette standardPalette() const override;

    void polish(QWidget *widget) override;

private:
    void drawQmlEditorIcon(PrimitiveElement element,
                           const QStyleOption *option,
                           const char *propertyName,
                           QPainter *painter,
                           const QWidget *widget = nullptr) const;

    StudioStylePrivate *d = nullptr;
};

} // namespace QmlDesigner
