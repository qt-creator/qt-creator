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

#ifndef QDECLARATIVEVIEWOBSERVER_P_H
#define QDECLARATIVEVIEWOBSERVER_P_H

#include <QtCore/QWeakPointer>
#include <QtCore/QPointF>

#include "qdeclarativeviewobserver.h"
#include "qdeclarativeobserverservice.h"

namespace QmlJSDebugger {

class JSDebuggerAgent;
class QDeclarativeViewObserver;
class LiveSelectionTool;
class ZoomTool;
class ColorPickerTool;
class LiveLayerItem;
class BoundingRectHighlighter;
class SubcomponentEditorTool;
class ToolBox;
class CrumblePath;
class AbstractLiveEditTool;

class QDeclarativeViewObserverPrivate : public QObject
{
    Q_OBJECT
public:
    enum ContextFlags {
        IgnoreContext,
        ContextSensitive
    };

    QDeclarativeViewObserverPrivate(QDeclarativeViewObserver *);
    ~QDeclarativeViewObserverPrivate();

    QDeclarativeView *view;
    QDeclarativeViewObserver *q;
    QDeclarativeObserverService *debugService;
    QWeakPointer<QWidget> viewport;

    QPointF cursorPos;
    QList<QWeakPointer<QGraphicsObject> > currentSelection;

    Constants::DesignTool currentToolMode;
    AbstractLiveEditTool *currentTool;

    LiveSelectionTool *selectionTool;
    ZoomTool *zoomTool;
    ColorPickerTool *colorPickerTool;
    SubcomponentEditorTool *subcomponentEditorTool;
    LiveLayerItem *manipulatorLayer;

    BoundingRectHighlighter *boundingRectHighlighter;

    bool designModeBehavior;
    bool showAppOnTop;

    bool executionPaused;
    qreal slowdownFactor;

    ToolBox *toolBox;

    void setViewport(QWidget *widget);

    void clearEditorItems();
    void createToolBox();
    void changeToSelectTool();
    QList<QGraphicsItem*> filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const;
    QList<QGraphicsItem*> filterForSelection(QList<QGraphicsItem*> &itemlist) const;

    QList<QGraphicsItem*> selectableItems(const QPoint &pos) const;
    QList<QGraphicsItem*> selectableItems(const QPointF &scenePos) const;
    QList<QGraphicsItem*> selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const;

    void setSelectedItemsForTools(QList<QGraphicsItem *> items);
    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems();

    void changeTool(Constants::DesignTool tool,
                    Constants::ToolFlags flags = Constants::NoToolFlags);

    void clearHighlight();
    void highlight(QList<QGraphicsObject *> item, ContextFlags flags = ContextSensitive);
    void highlight(QGraphicsObject *item, ContextFlags flags = ContextSensitive);

    bool mouseInsideContextItem() const;
    bool isEditorItem(QGraphicsItem *item) const;

    QGraphicsItem *currentRootItem() const;

    void enterContext(QGraphicsItem *itemToEnter);

public slots:
    void _q_setToolBoxVisible(bool visible);

    void _q_reloadView();
    void _q_onStatusChanged(QDeclarativeView::Status status);
    void _q_onCurrentObjectsChanged(QList<QObject*> objects);
    void _q_applyChangesFromClient();
    void _q_createQmlObject(const QString &qml, QObject *parent,
                            const QStringList &imports, const QString &filename = QString());
    void _q_reparentQmlObject(QObject *, QObject *);

    void _q_changeToSingleSelectTool();
    void _q_changeToMarqueeSelectTool();
    void _q_changeToZoomTool();
    void _q_changeToColorPickerTool();
    void _q_changeContextPathIndex(int index);
    void _q_clearComponentCache();
    void _q_removeFromSelection(QObject *);

public:
    static QDeclarativeViewObserverPrivate *get(QDeclarativeViewObserver *v) { return v->d_func(); }
};

} // namespace QmlJSDebugger

#endif // QDECLARATIVEVIEWOBSERVER_P_H
