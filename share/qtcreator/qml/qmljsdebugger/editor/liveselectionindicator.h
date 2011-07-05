/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef LIVESELECTIONINDICATOR_H
#define LIVESELECTIONINDICATOR_H

#include <QtCore/QWeakPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QGraphicsObject;
class QGraphicsRectItem;
class QGraphicsItem;
class QPolygonF;
QT_END_NAMESPACE

namespace QmlJSDebugger {

class QDeclarativeViewInspector;

class LiveSelectionIndicator
{
public:
    LiveSelectionIndicator(QDeclarativeViewInspector *viewInspector, QGraphicsObject *layerItem);
    ~LiveSelectionIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<QWeakPointer<QGraphicsObject> > &itemList);

private:
    QHash<QGraphicsItem*, QGraphicsRectItem *> m_indicatorShapeHash;
    QWeakPointer<QGraphicsObject> m_layerItem;
    QDeclarativeViewInspector *m_view;
};

}

#endif // LIVESELECTIONINDICATOR_H
