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

#ifndef ABSTRACTFORMEDITORTOOL_H
#define ABSTRACTFORMEDITORTOOL_H

#include "formeditoritem.h"

#include <qmldesignercorelib_global.h>

#include "snapper.h"

QT_BEGIN_NAMESPACE
class QGraphicsItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorView;

class QMLDESIGNERCORE_EXPORT AbstractFormEditorTool
{
    friend class FormEditorView;
public:
    AbstractFormEditorTool(FormEditorView* view);

    virtual ~AbstractFormEditorTool();

    virtual void mousePressEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);

    virtual void hoverMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) = 0;

    virtual void dropEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event);
    virtual void dragEnterEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event);
    virtual void dragLeaveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) = 0;
    virtual void dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) = 0;

    virtual void keyPressEvent(QKeyEvent *event) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;

    virtual void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) = 0;
    //    virtual QVariant itemChange(QList<QGraphicsItem*> itemList,
//                        QGraphicsItem::GraphicsItemChange change,
//                        const QVariant &value ) = 0;
//    virtual void update() = 0;
    virtual void clear();

    virtual void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) = 0;

    virtual void instancesCompleted(const QList<FormEditorItem*> &itemList) = 0;
    virtual void instancesParentChanged(const QList<FormEditorItem*> &itemList) = 0;
    virtual void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) = 0;

    void setItems(const QList<FormEditorItem*> &itemList);
    QList<FormEditorItem*> items() const;

    static QList<FormEditorItem*> toFormEditorItemList(const QList<QGraphicsItem*> &itemList);
    static QGraphicsItem* topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList);
    static FormEditorItem* topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList, bool selectOnlyContentItems);
    bool topItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topItemIsResizeHandle(const QList<QGraphicsItem*> &itemList);

    QList<FormEditorItem*> filterSelectedModelNodes(const QList<FormEditorItem*> &itemList) const;

    FormEditorItem *nearestFormEditorItem(const QPointF &point, const QList<QGraphicsItem *> &itemList);

protected:
    virtual void selectedItemsChanged(const QList<FormEditorItem*> &itemList) = 0;
    virtual void showContextMenu(QGraphicsSceneMouseEvent *event);
    Snapper::Snapping generateUseSnapping(Qt::KeyboardModifiers keyboardModifier) const;
    FormEditorItem *containerFormEditorItem(const QList<QGraphicsItem*> &itemUnderMouseList, const QList<FormEditorItem*> &selectedItemList) const;

    FormEditorView *view() const;
    void setView(FormEditorView *view);
    FormEditorScene* scene() const;
private:
    FormEditorView *m_view;
    QList<FormEditorItem*> m_itemList;
};

}

#endif // ABSTRACTFORMEDITORTOOL_H
