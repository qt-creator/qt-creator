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

#ifndef QMT_ALIGNBUTTONSITEM_H
#define QMT_ALIGNBUTTONSITEM_H

#include "qmt/diagram_scene/capabilities/alignable.h"

#include <QGraphicsItem>

namespace qmt {

class IAlignable;


class AlignButtonsItem :
        public QGraphicsItem
{

    class AlignButtonItem;

public:

    enum {
        NORMAL_PIXMAP_WIDTH = 14,
        NORMAL_PIXMAP_HEIGHT = NORMAL_PIXMAP_WIDTH,
        INNER_BORDER = 2,
        NORMAL_BUTTON_WIDTH = NORMAL_PIXMAP_WIDTH + 2 * INNER_BORDER,
        NORMAL_BUTTON_HEIGHT = NORMAL_PIXMAP_HEIGHT + 2 * INNER_BORDER,
        HORIZONTAL_DISTANCE_TO_OBJECT = 4,
        VERTICAL_DISTANCE_TO_OBEJCT = HORIZONTAL_DISTANCE_TO_OBJECT
    };

public:

    AlignButtonsItem(IAlignable *alignable, QGraphicsItem *parent = 0);

    ~AlignButtonsItem();

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public:

    void clear();

    void addButton(IAlignable::AlignType align_type, const QString &identifier, qreal pos);

private:

    IAlignable *m_alignable;

    QList<AlignButtonItem *> m_alignItems;

};

}

#endif // QMT_ALIGNBUTTONSITEM_H
