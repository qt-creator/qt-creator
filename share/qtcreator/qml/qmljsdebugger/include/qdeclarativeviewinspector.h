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

#ifndef QDECLARATIVEVIEWINSPECTOR_H
#define QDECLARATIVEVIEWINSPECTOR_H

#include "qmljsdebugger_global.h"
#include "qmlinspectorconstants.h"

#include <QtCore/QScopedPointer>
#include <QtDeclarative/QDeclarativeView>

QT_FORWARD_DECLARE_CLASS(QDeclarativeItem)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QToolBar)

namespace QmlJSDebugger {

class CrumblePath;
class QDeclarativeViewInspectorPrivate;

class QMLJSDEBUGGER_EXPORT QDeclarativeViewInspector : public QObject
{
    Q_OBJECT
public:

    explicit QDeclarativeViewInspector(QDeclarativeView *view, QObject *parent = 0);
    ~QDeclarativeViewInspector();

    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems();

    QDeclarativeView *declarativeView();

    static QString idStringForObject(QObject *obj);
    QRectF adjustToScreenBoundaries(const QRectF &boundingRectInSceneSpace);

    bool showAppOnTop() const;

public Q_SLOTS:
    void setDesignModeBehavior(bool value);
    bool designModeBehavior();

    void setShowAppOnTop(bool appOnTop);

    void setAnimationSpeed(qreal factor);
    void setAnimationPaused(bool paused);

Q_SIGNALS:
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloadRequested();
    void marqueeSelectToolActivated();
    void selectToolActivated();
    void zoomToolActivated();
    void colorPickerActivated();
    void selectedColorChanged(const QColor &color);

    void animationSpeedChanged(qreal factor);
    void animationPausedChanged(bool paused);

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

private slots:
    void animationSpeedChangeRequested(qreal factor);
    void animationPausedChangeRequested(bool paused);

private:
    Q_DISABLE_COPY(QDeclarativeViewInspector)

    inline QDeclarativeViewInspectorPrivate *d_func() { return data.data(); }
    QScopedPointer<QDeclarativeViewInspectorPrivate> data;
    friend class QDeclarativeViewInspectorPrivate;
    friend class AbstractLiveEditTool;
};

} // namespace QmlJSDebugger

#endif // QDECLARATIVEVIEWINSPECTOR_H
