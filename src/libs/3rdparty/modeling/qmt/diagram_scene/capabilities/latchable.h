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
        MOVE,
        RESIZE_LEFT,
        RESIZE_TOP,
        RESIZE_RIGHT,
        RESIZE_BOTTOM
    };

    enum LatchType {
        NONE,
        LEFT,
        TOP,
        RIGHT,
        BOTTOM,
        HCENTER,
        VCENTER
    };

    struct Latch {
        Latch()
            : _latch_type(NONE),
               _pos(0.0),
               _other_pos1(0.0),
               _other_pos2(0.0),
               _identifier()
        {
        }

        Latch(LatchType latch_type, qreal pos, qreal other_pos1, qreal other_pos2, const QString &identifier)
            : _latch_type(latch_type),
              _pos(pos),
              _other_pos1(other_pos1),
              _other_pos2(other_pos2),
              _identifier(identifier)
        {
        }

        LatchType _latch_type;
        qreal _pos;
        qreal _other_pos1, _other_pos2;
        QString _identifier;
    };

public:

    virtual ~ILatchable() { }

    virtual QList<Latch> getHorizontalLatches(Action action, bool grabbed_item) const = 0;

    virtual QList<Latch> getVerticalLatches(Action action, bool grabbed_item) const = 0;

};

}

#endif // QMT_LATCHABLE_H
