// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

QT_BEGIN_NAMESPACE
class QString;
class QPointF;
QT_END_NAMESPACE

namespace qmt {

class IRelationable
{
public:
    virtual ~IRelationable() { }

    virtual QPointF relationStartPos() const = 0;
    virtual void relationDrawn(const QString &id, const QPointF &toScenePos,
                               const QList<QPointF> &intermediatePoints) = 0;
};

} // namespace qmt
