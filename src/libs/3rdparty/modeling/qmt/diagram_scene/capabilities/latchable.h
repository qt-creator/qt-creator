/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_LATCHABLE_H
#define QMT_LATCHABLE_H

#include <QList>

namespace qmt {

class ILatchable {
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
        Latch()
            : m_latchType(None),
              m_pos(0.0),
              m_otherPos1(0.0),
              m_otherPos2(0.0),
              m_identifier()
        {
        }

        Latch(LatchType latchType, qreal pos, qreal otherPos1, qreal otherPos2, const QString &identifier)
            : m_latchType(latchType),
              m_pos(pos),
              m_otherPos1(otherPos1),
              m_otherPos2(otherPos2),
              m_identifier(identifier)
        {
        }

        LatchType m_latchType;
        qreal m_pos;
        qreal m_otherPos1, m_otherPos2;
        QString m_identifier;
    };

public:

    virtual ~ILatchable() { }

    virtual QList<Latch> horizontalLatches(Action action, bool grabbedItem) const = 0;

    virtual QList<Latch> verticalLatches(Action action, bool grabbedItem) const = 0;

};

}

#endif // QMT_LATCHABLE_H
