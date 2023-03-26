// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace qmt {

class IMoveable
{
public:
    virtual ~IMoveable() { }

    virtual void moveDelta(const QPointF &delta) = 0;
    virtual void alignItemPositionToRaster(double rasterWidth, double rasterHeight) = 0;
};

} // namespace qmt
