/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef RUBBERBANDSELECTIONMANIPULATOR_H
#define RUBBERBANDSELECTIONMANIPULATOR_H


#include "selectionrectangle.h"

namespace QmlJSDebugger {

class QDeclarativeViewObserver;

class RubberBandSelectionManipulator
{
public:
    enum SelectionType {
        ReplaceSelection,
        AddToSelection,
        RemoveFromSelection
    };

    RubberBandSelectionManipulator(QGraphicsObject *layerItem,
                                   QDeclarativeViewObserver *editorView);

    void setItems(const QList<QGraphicsItem*> &itemList);

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void end();

    void clear();

    void select(SelectionType selectionType);

    QPointF beginPoint() const;

    bool isActive() const;

protected:
    QGraphicsItem *topFormEditorItem(const QList<QGraphicsItem*> &itemList);

private:
    QList<QGraphicsItem*> m_itemList;
    QList<QGraphicsItem*> m_oldSelectionList;
    SelectionRectangle m_selectionRectangleElement;
    QPointF m_beginPoint;
    QDeclarativeViewObserver *m_editorView;
    QGraphicsItem *m_beginFormEditorItem;
    bool m_isActive;
};

}

#endif // RUBBERBANDSELECTIONMANIPULATOR_H
