// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsObject>
#include <QTransform>

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
    ~LayerItem() override;
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;
    QRectF boundingRect() const override;
    int type() const override;

    QList<QGraphicsItem*> findAllChildItems() const;

    QTransform viewportTransform() const;

protected:
    QList<QGraphicsItem*> findAllChildItems(const QGraphicsItem *item) const;
};

inline int LayerItem::type() const
{
    return Type;
}

} // namespace QmlDesigner
