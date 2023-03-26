
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QPalette>
#include <QPointF>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

double scaleX(const QTransform &transform)
{
    return transform.m11();
}

double scaleY(const QTransform &transform)
{
    return transform.m22();
}

void grow(QRectF &rect, const QPointF &point)
{
    if (rect.left() > point.x())
        rect.setLeft(point.x());

    if (rect.right() < point.x())
        rect.setRight(point.x());

    if (rect.top() > point.y())
        rect.setTop(point.y());

    if (rect.bottom() < point.y())
        rect.setBottom(point.y());
}

QRectF bbox(const QRectF &rect, const QTransform &transform)
{
    QRectF out = rect;
    grow(out, transform.map(rect.topLeft()));
    grow(out, transform.map(rect.topRight()));
    grow(out, transform.map(rect.bottomLeft()));
    grow(out, transform.map(rect.bottomRight()));
    return out;
}

QPalette singleColorPalette(const QColor &color)
{
    QPalette palette;
    palette.setColor(QPalette::Window, color);
    palette.setColor(QPalette::WindowText, color);
    palette.setColor(QPalette::Base, color);
    palette.setColor(QPalette::AlternateBase, color);
    palette.setColor(QPalette::ToolTipBase, color);
    palette.setColor(QPalette::ToolTipText, color);
    palette.setColor(QPalette::Text, color);

    palette.setColor(QPalette::Button, color);
    palette.setColor(QPalette::ButtonText, color);
    palette.setColor(QPalette::BrightText, color);
    palette.setColor(QPalette::Light, color);
    palette.setColor(QPalette::Midlight, color);
    palette.setColor(QPalette::Dark, color);
    palette.setColor(QPalette::Mid, color);
    palette.setColor(QPalette::Shadow, color);
    palette.setColor(QPalette::Highlight, color);
    palette.setColor(QPalette::HighlightedText, color);
    palette.setColor(QPalette::Link, color);
    palette.setColor(QPalette::LinkVisited, color);
    return palette;
}

} // End namespace QmlDesigner.
