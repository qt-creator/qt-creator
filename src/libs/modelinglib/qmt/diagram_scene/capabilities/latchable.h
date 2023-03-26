// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

namespace qmt {

class ILatchable
{
public:
    enum Action {
        Move,
        ResizeLeft,
        ResizeTop,
        ResizeRight,
        ResizeBottom
    };

    enum LatchType {
        None,
        Left,
        Top,
        Right,
        Bottom,
        Hcenter,
        Vcenter
    };

    class Latch
    {
    public:
        Latch() = default;

        Latch(LatchType latchType, qreal pos, qreal otherPos1, qreal otherPos2,
              const QString &identifier)
            : m_latchType(latchType),
              m_pos(pos),
              m_otherPos1(otherPos1),
              m_otherPos2(otherPos2),
              m_identifier(identifier)
        {
        }

        LatchType m_latchType = LatchType::None;
        qreal m_pos = 0.0;
        qreal m_otherPos1 = 0.0;
        qreal m_otherPos2 = 0.0;
        QString m_identifier;
    };

    virtual ~ILatchable() {}

    virtual Action horizontalLatchAction() const = 0;
    virtual Action verticalLatchAction() const = 0;
    virtual QList<Latch> horizontalLatches(Action action, bool grabbedItem) const = 0;
    virtual QList<Latch> verticalLatches(Action action, bool grabbedItem) const = 0;
};

} // namespace qmt
