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

#ifndef QDECLARATIVEDESIGNVIEW_H
#define QDECLARATIVEDESIGNVIEW_H

#include "qmljsdebugger_global.h"
#include "qmlviewerconstants.h"
#include <qdeclarativeview.h>
#include <QWeakPointer>

QT_FORWARD_DECLARE_CLASS(QDeclarativeItem);
QT_FORWARD_DECLARE_CLASS(QMouseEvent);
QT_FORWARD_DECLARE_CLASS(QToolBar);

namespace QmlViewer {

class CrumblePath;
class QDeclarativeDesignViewPrivate;

class QMLJSDEBUGGER_EXPORT QDeclarativeDesignView : public QDeclarativeView
{
    Q_OBJECT
public:
    enum ContextFlags {
        IgnoreContext,
        ContextSensitive
    };

    explicit QDeclarativeDesignView(QWidget *parent = 0);
    ~QDeclarativeDesignView();

    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems();

    QGraphicsObject *manipulatorLayer() const;
    void changeTool(Constants::DesignTool tool,
                    Constants::ToolFlags flags = Constants::NoToolFlags);

    void clearHighlight();
    void highlight(QList<QGraphicsItem *> item, ContextFlags flags = ContextSensitive);
    void highlight(QGraphicsItem *item, ContextFlags flags = ContextSensitive);

    bool mouseInsideContextItem() const;
    bool isEditorItem(QGraphicsItem *item) const;

    QList<QGraphicsItem*> selectableItems(const QPoint &pos) const;
    QList<QGraphicsItem*> selectableItems(const QPointF &scenePos) const;
    QList<QGraphicsItem*> selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const;
    QGraphicsItem *currentRootItem() const;

    CrumblePath *crumblePathWidget() const;
    QToolBar *toolbar() const;
    static QString idStringForObject(QObject *obj);
    QRectF adjustToScreenBoundaries(const QRectF &boundingRectInSceneSpace);

public Q_SLOTS:
    void setDesignModeBehavior(bool value);
    bool designModeBehavior() const;
    void changeToSingleSelectTool();
    void changeToMarqueeSelectTool();
    void changeToZoomTool();
    void changeToColorPickerTool();

    void changeAnimationSpeed(qreal slowdownFactor);
    void continueExecution(qreal slowdownFactor = 1.0f);
    void pauseExecution();

Q_SIGNALS:
    void designModeBehaviorChanged(bool inDesignMode);
    void reloadRequested();
    void marqueeSelectToolActivated();
    void selectToolActivated();
    void zoomToolActivated();
    void colorPickerActivated();
    void selectedColorChanged(const QColor &color);

    void executionStarted(qreal slowdownFactor);
    void executionPaused();

protected:
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private Q_SLOTS:
    void reloadView();
    void onStatusChanged(QDeclarativeView::Status status);
    void onCurrentObjectsChanged(QList<QObject*> objects);
    void applyChangesFromClient();
    void createQmlObject(const QString &qml, QObject *parent,
                         const QStringList &imports, const QString &filename = QString());

private:
    void clearEditorItems();
    void createToolbar();
    void changeToSelectTool();
    QList<QGraphicsItem*> filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const;
    QList<QGraphicsItem*> filterForSelection(QList<QGraphicsItem*> &itemlist) const;

    QDeclarativeDesignViewPrivate *data;

private:
    Q_DISABLE_COPY(QDeclarativeDesignView)

};

} //namespace QmlViewer

#endif // QDECLARATIVEDESIGNVIEW_H
