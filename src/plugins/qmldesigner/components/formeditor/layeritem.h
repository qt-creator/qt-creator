/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef LAYERITEM_H
#define LAYERITEM_H

#include <QGraphicsObject>

namespace QmlDesigner {

class FormEditorScene;

class LayerItem : public QGraphicsObject
{
public:
    enum
    {
        Type = 0xEAAA
    };
    LayerItem(FormEditorScene* scene);
    ~LayerItem();
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    QRectF boundingRect() const;
    int type() const;

    QList<QGraphicsItem*> findAllChildItems() const;

protected:
    QList<QGraphicsItem*> findAllChildItems(const QGraphicsItem *item) const;
};

inline int LayerItem::type() const
{
    return Type;
}

}

#endif // LAYERITEM_H
