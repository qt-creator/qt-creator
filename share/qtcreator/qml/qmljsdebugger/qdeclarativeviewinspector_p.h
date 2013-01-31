/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QDECLARATIVEVIEWINSPECTOR_P_H
#define QDECLARATIVEVIEWINSPECTOR_P_H

#include <QWeakPointer>
#include <QPointF>

#include "qdeclarativeviewinspector.h"
#include "qdeclarativeinspectorservice.h"

namespace QmlJSDebugger {

class JSDebuggerAgent;
class QDeclarativeViewInspector;
class LiveSelectionTool;
class ZoomTool;
class ColorPickerTool;
class LiveLayerItem;
class BoundingRectHighlighter;
class CrumblePath;
class AbstractLiveEditTool;

class QDeclarativeViewInspectorPrivate : public QObject
{
    Q_OBJECT
public:
    QDeclarativeViewInspectorPrivate(QDeclarativeViewInspector *);
    ~QDeclarativeViewInspectorPrivate();

    QDeclarativeView *view;
    QDeclarativeViewInspector *q;
    QDeclarativeInspectorService *debugService;
    QWeakPointer<QWidget> viewport;

    QPointF cursorPos;
    QList<QWeakPointer<QGraphicsObject> > currentSelection;

    Constants::DesignTool currentToolMode;
    AbstractLiveEditTool *currentTool;

    LiveSelectionTool *selectionTool;
    ZoomTool *zoomTool;
    ColorPickerTool *colorPickerTool;
    LiveLayerItem *manipulatorLayer;

    BoundingRectHighlighter *boundingRectHighlighter;

    bool designModeBehavior;
    bool showAppOnTop;

    bool animationPaused;
    qreal slowDownFactor;

    void setViewport(QWidget *widget);

    void clearEditorItems();
    void changeToSelectTool();
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
    void highlight(const QList<QGraphicsObject *> &item);
    inline void highlight(QGraphicsObject *item)
    { highlight(QList<QGraphicsObject*>() << item); }

    bool isEditorItem(QGraphicsItem *item) const;

public slots:
    void _q_reloadView();
    void _q_onStatusChanged(QDeclarativeView::Status status);
    void _q_onCurrentObjectsChanged(QList<QObject*> objects);
    void _q_applyChangesFromClient();
    void _q_createQmlObject(const QString &qml, QObject *parent,
                            const QStringList &imports, const QString &filename = QString(), int order = 0);
    void _q_reparentQmlObject(QObject *, QObject *);
    void _q_deleteQmlObject(QObject *);

    void _q_changeToSingleSelectTool();
    void _q_changeToMarqueeSelectTool();
    void _q_changeToZoomTool();
    void _q_changeToColorPickerTool();
    void _q_clearComponentCache();
    void _q_removeFromSelection(QObject *);

public:
    static QDeclarativeViewInspectorPrivate *get(QDeclarativeViewInspector *v) { return v->d_func(); }
};

} // namespace QmlJSDebugger

#endif // QDECLARATIVEVIEWINSPECTOR_P_H
