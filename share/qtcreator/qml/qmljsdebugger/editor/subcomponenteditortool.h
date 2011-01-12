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

#ifndef SUBCOMPONENTEDITORTOOL_H
#define SUBCOMPONENTEDITORTOOL_H

#include "abstractformeditortool.h"
#include <QStack>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QGraphicsObject)
QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace QmlJSDebugger {

class SubcomponentMaskLayerItem;

class SubcomponentEditorTool : public AbstractFormEditorTool
{
    Q_OBJECT

public:
    SubcomponentEditorTool(QDeclarativeViewObserver *view);
    ~SubcomponentEditorTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void clear();

    bool containsCursor(const QPoint &mousePos) const;
    bool itemIsChildOfQmlSubComponent(QGraphicsItem *item) const;

    bool isChildOfContext(QGraphicsItem *item) const;
    bool isDirectChildOfContext(QGraphicsItem *item) const;
    QGraphicsItem *firstChildOfContext(QGraphicsItem *item) const;

    void setCurrentItem(QGraphicsItem *contextObject);

    void pushContext(QGraphicsObject *contextItem);

    QGraphicsObject *currentRootItem() const;
    QGraphicsObject *setContext(int contextIndex);
    int contextIndex() const;

signals:
    void exitContextRequested();
    void cleared();
    void contextPushed(const QString &contextTitle);
    void contextPopped();
    void contextPathChanged(const QStringList &path);

protected:
    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private slots:
    void animate();
    void contextDestroyed(QObject *context);
    void resizeMask();

private:
    QGraphicsObject *popContext();
    void aboutToPopContext();

private:
    QStack<QGraphicsObject *> m_currentContext;
    QStringList m_path;

    qreal m_animIncrement;
    SubcomponentMaskLayerItem *m_mask;
    QTimer *m_animTimer;
};

} // namespace QmlJSDebugger

#endif // SUBCOMPONENTEDITORTOOL_H
