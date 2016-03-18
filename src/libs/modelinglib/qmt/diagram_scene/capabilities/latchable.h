/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
