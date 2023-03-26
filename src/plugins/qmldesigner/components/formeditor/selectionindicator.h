// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"
#include "formeditoritem.h"

#include <QCursor>
#include <QPointer>
#include <QGraphicsPolygonItem>

#include <memory>

namespace QmlDesigner {

class FormEditorAnnotationIcon;

class SelectionIndicator
{
public:
    SelectionIndicator(LayerItem *layerItem);
    ~SelectionIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

    void setCursor(const QCursor &cursor);
private:
    void adjustAnnotationPosition(const QRectF &itemRect, const QRectF &labelRect, qreal scaleFactor);

private:
    QHash<FormEditorItem*, QGraphicsPolygonItem *> m_indicatorShapeHash;
    QPointer<LayerItem> m_layerItem;
    QCursor m_cursor;
    std::unique_ptr<QGraphicsPolygonItem> m_labelItem;
    FormEditorAnnotationIcon *m_annotationItem = nullptr; //handled by m_labelItem
};

} // namespace QmlDesigner
