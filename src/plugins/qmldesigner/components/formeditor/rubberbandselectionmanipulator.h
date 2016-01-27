/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
