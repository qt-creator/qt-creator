// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace qmt {

class IWindable
{
public:
    virtual ~IWindable() { }

    virtual QPointF grabHandle(int index) = 0;
    virtual void insertHandle(int beforeIndex, const QPointF &pos, double rasterWidth, double rasterHeight) = 0;
    virtual void deleteHandle(int index) = 0;
    virtual void setHandlePos(int index, const QPointF &pos) = 0;
    virtual void dropHandle(int index, double rasterWidth, double rasterHeight) = 0;
};

} // namespace qmt
