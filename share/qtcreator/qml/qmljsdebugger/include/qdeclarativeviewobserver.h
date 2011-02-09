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

#ifndef QDECLARATIVEVIEWOBSERVER_H
#define QDECLARATIVEVIEWOBSERVER_H

#include "qmljsdebugger_global.h"
#include "qmlobserverconstants.h"

#include <QtCore/QScopedPointer>
#include <QtDeclarative/QDeclarativeView>

QT_FORWARD_DECLARE_CLASS(QDeclarativeItem)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QToolBar)

namespace QmlJSDebugger {

class CrumblePath;
class QDeclarativeViewObserverPrivate;

class QMLJSDEBUGGER_EXPORT QDeclarativeViewObserver : public QObject
{
    Q_OBJECT
public:

    explicit QDeclarativeViewObserver(QDeclarativeView *view, QObject *parent = 0);
    ~QDeclarativeViewObserver();

    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems();

    QDeclarativeView *declarativeView();

    QToolBar *toolbar() const;
    static QString idStringForObject(QObject *obj);
    QRectF adjustToScreenBoundaries(const QRectF &boundingRectInSceneSpace);

    bool showAppOnTop() const;

public Q_SLOTS:
    void setDesignModeBehavior(bool value);
    bool designModeBehavior();

    void setShowAppOnTop(bool appOnTop);

    void changeAnimationSpeed(qreal slowdownFactor);
    void continueExecution(qreal slowdownFactor = 1.0f);
    void pauseExecution();

    void setObserverContext(int contextIndex);

Q_SIGNALS:
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloadRequested();
    void marqueeSelectToolActivated();
    void selectToolActivated();
    void zoomToolActivated();
    void colorPickerActivated();
    void selectedColorChanged(const QColor &color);

    void executionStarted(qreal slowdownFactor);
    void executionPaused();

    void inspectorContextCleared();
    void inspectorContextPushed(const QString &contextTitle);
    void inspectorContextPopped();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

    bool leaveEvent(QEvent *);
    bool mousePressEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent *event);
    bool keyPressEvent(QKeyEvent *event);
    bool keyReleaseEvent(QKeyEvent *keyEvent);
    bool mouseDoubleClickEvent(QMouseEvent *event);
    bool wheelEvent(QWheelEvent *event);

    void setSelectedItemsForTools(QList<QGraphicsItem *> items);

private:
    Q_DISABLE_COPY(QDeclarativeViewObserver)
    Q_PRIVATE_SLOT(d_func(), void _q_reloadView())
    Q_PRIVATE_SLOT(d_func(), void _q_onStatusChanged(QDeclarativeView::Status))
    Q_PRIVATE_SLOT(d_func(), void _q_onCurrentObjectsChanged(QList<QObject*>))
    Q_PRIVATE_SLOT(d_func(), void _q_applyChangesFromClient())
    Q_PRIVATE_SLOT(d_func(), void _q_createQmlObject(const QString &, QObject *, const QStringList &, const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_reparentQmlObject(QObject *, QObject *))
    Q_PRIVATE_SLOT(d_func(), void _q_changeToSingleSelectTool())
    Q_PRIVATE_SLOT(d_func(), void _q_changeToMarqueeSelectTool())
    Q_PRIVATE_SLOT(d_func(), void _q_changeToZoomTool())
    Q_PRIVATE_SLOT(d_func(), void _q_changeToColorPickerTool())
    Q_PRIVATE_SLOT(d_func(), void _q_changeContextPathIndex(int index))
    Q_PRIVATE_SLOT(d_func(), void _q_clearComponentCache())
    Q_PRIVATE_SLOT(d_func(), void _q_removeFromSelection(QObject *))

    inline QDeclarativeViewObserverPrivate *d_func() { return data.data(); }
    QScopedPointer<QDeclarativeViewObserverPrivate> data;
    friend class QDeclarativeViewObserverPrivate;
    friend class AbstractLiveEditTool;
};

} //namespace QmlJSDebugger

#endif // QDECLARATIVEVIEWOBSERVER_H
