/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SINGLESELECTIONMANIPULATOR_H
#define SINGLESELECTIONMANIPULATOR_H

#include <QtCore/QPointF>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem);

namespace QmlJSDebugger {

class QDeclarativeViewObserver;

class SingleSelectionManipulator
{
public:
    SingleSelectionManipulator(QDeclarativeViewObserver *editorView);

    enum SelectionType {
        ReplaceSelection,
        AddToSelection,
        RemoveFromSelection,
        InvertSelection
    };

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void end(const QPointF& updatePoint);

    void select(SelectionType selectionType, const QList<QGraphicsItem*> &items,
                bool selectOnlyContentItems);
    void select(SelectionType selectionType, bool selectOnlyContentItems);

    void clear();

    QPointF beginPoint() const;

    bool isActive() const;

private:
    QList<QGraphicsItem*> m_oldSelectionList;
    QPointF m_beginPoint;
    QDeclarativeViewObserver *m_editorView;
    bool m_isActive;
};

}

#endif // SINGLESELECTIONMANIPULATOR_H
