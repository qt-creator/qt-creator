// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "selectionrectangle.h"
#include "formeditorview.h"
#include "formeditoritem.h"

namespace QmlDesigner {

class RubberBandSelectionManipulator
{
public:
    enum SelectionType {
        ReplaceSelection,
        AddToSelection,
        RemoveFromSelection
    };

    RubberBandSelectionManipulator(LayerItem *layerItem, FormEditorView *editorView);

    void setItems(const QList<FormEditorItem*> &itemList);

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void end();

    void clear();

    void select(SelectionType selectionType);

    QPointF beginPoint() const;

    bool isActive() const;

protected:
    FormEditorItem *topFormEditorItem(const QList<QGraphicsItem*> &itemList);

private:
    QList<FormEditorItem*> m_itemList;
    QList<QmlItemNode> m_oldSelectionList;
    SelectionRectangle m_selectionRectangleElement;
    QPointF m_beginPoint;
    FormEditorView *m_editorView;
    FormEditorItem *m_beginFormEditorItem;
    bool m_isActive;
};

} // namespace QmlDesigner
