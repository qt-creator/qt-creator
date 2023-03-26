// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QPointF;
class QRectF;
class QSizeF;
QT_END_NAMESPACE

namespace qmt {

class IResizable
{
public:
    enum Side {
        SideNone,
        SideLeftOrTop,
        SideRightOrBottom
    };

    virtual ~IResizable() { }

    virtual QPointF pos() const = 0;
    virtual QRectF rect() const = 0;
    virtual QSizeF minimumSize() const = 0;

    virtual void setPosAndRect(const QPointF &originalPos, const QRectF &originalRect,
                               const QPointF &topLeftDelta, const QPointF &bottomRightDelta) = 0;
    virtual void alignItemSizeToRaster(Side adjustHorizontalSide, Side adjustVerticalSide,
                                       double rasterWidth, double rasterHeight) = 0;
};

} // namespace qmt
