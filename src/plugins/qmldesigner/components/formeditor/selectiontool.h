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

#ifndef SELECTIONTOOL_H
#define SELECTIONTOOL_H


#include "abstractformeditortool.h"
#include "rubberbandselectionmanipulator.h"
#include "singleselectionmanipulator.h"
#include "selectionindicator.h"
#include "resizeindicator.h"
#include "anchorindicator.h"
#include "bindingindicator.h"
#include "contentnoteditableindicator.h"

#include <QTime>

namespace QmlDesigner {


class SelectionTool : public AbstractFormEditorTool
{
public:
    SelectionTool(FormEditorView* editorView);
    ~SelectionTool();

    void mousePressEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;

    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void dragLeaveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent * event) override;

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) override;
    //    QVariant itemChange(const QList<QGraphicsItem*> &itemList,
//                        QGraphicsItem::GraphicsItemChange change,
//                        const QVariant &value );

//    void update();

    void clear() override;

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem*> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void selectUnderPoint(QGraphicsSceneMouseEvent *event);

    void setCursor(const QCursor &cursor);

private:
    RubberBandSelectionManipulator m_rubberbandSelectionManipulator;
    SingleSelectionManipulator m_singleSelectionManipulator;
    SelectionIndicator m_selectionIndicator;
    ResizeIndicator m_resizeIndicator;
    AnchorIndicator m_anchorIndicator;
    BindingIndicator m_bindingIndicator;
    ContentNotEditableIndicator m_contentNotEditableIndicator;
    QTime m_mousePressTimer;
    QCursor m_cursor;
};

}
#endif // SELECTIONTOOL_H
