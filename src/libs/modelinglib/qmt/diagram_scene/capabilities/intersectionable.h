// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QPointF;
class QLineF;
QT_END_NAMESPACE

namespace qmt {

class IIntersectionable
{
public:
    virtual ~IIntersectionable() { }

    virtual bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                        QLineF *intersectionLine = nullptr) const = 0;
};

} // namespace qmt
