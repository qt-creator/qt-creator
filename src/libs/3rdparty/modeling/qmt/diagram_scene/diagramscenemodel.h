/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_DIAGRAMSCENEMODEL_H
#define QMT_DIAGRAMSCENEMODEL_H

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <QList>
#include <QHash>
#include <QSet>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QPointF;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE


namespace qmt {

class DiagramGraphicsScene;
class LatchController;

class Uid;
class DiagramController;
class DiagramSceneController;
class StereotypeController;
class StyleController;
class DSelection;
class MDiagram;
class DElement;
class DObject;

class QMT_EXPORT DiagramSceneModel :
        public QObject
{
    Q_OBJECT

    class CreationVisitor;
    class UpdateVisitor;
    class OriginItem;

    friend class UpdateVisitor;
    friend class DiagramGraphicsScene;

public:

    enum CollidingMode {
        COLLIDING_INNER_ITEMS,
        COLLIDING_ITEMS,
        COLLIDING_OUTER_ITEMS
    };


public:

    DiagramSceneModel(QObject *parent = 0);

    ~DiagramSceneModel();

signals:

    void diagramSceneActivated(const MDiagram *diagram);

    void selectionChanged(const MDiagram *diagram);

public:

    DiagramController *getDiagramController() const { return _diagram_controller; }

    void setDiagramController(DiagramController *diagram_controller);

    DiagramSceneController *getDiagramSceneController() const { return _diagram_scene_controller; }

    void setDiagramSceneController(DiagramSceneController *diagram_scene_controller);

    StyleController *getStyleController() const { return _style_controller; }

    void setStyleController(StyleController *style_controller);

    StereotypeController *getStereotypeController() const { return _stereotype_controller; }

    void setStereotypeController(StereotypeController *stereotype_controller);

    MDiagram *getDiagram() const { return _diagram; }

    void setDiagram(MDiagram *diagram);

    QGraphicsScene *getGraphicsScene() const;

public:

    bool hasSelection() const;

    bool hasMultiObjectsSelection() const;

    DSelection getSelectedElements() const;

    DElement *findTopmostElement(const QPointF &scene_pos) const;

public:

    QList<QGraphicsItem *> getGraphicsItems() const { return _graphics_items; }

    QGraphicsItem *getGraphicsItem(DElement *element) const;

    QGraphicsItem *getGraphicsItem(const Uid &uid) const;

    QGraphicsItem *getFocusItem() const { return _focus_item; }

    bool isSelectedItem(QGraphicsItem *item) const;

    QSet<QGraphicsItem *> getSelectedItems() const { return _selected_items; }

    DElement *getElement(QGraphicsItem *item) const;

    bool isElementEditable(const DElement *element) const;

public:

    void selectAllElements();

    void selectElement(DElement *element);

    void editElement(DElement *element);

    void copyToClipboard();

    bool exportPng(const QString &file_name);

    void exportPdf(const QString &file_name);

public:

    void selectItem(QGraphicsItem *item, bool multi_select);

    void moveSelectedItems(QGraphicsItem *grabbed_item, const QPointF &delta);

    void alignSelectedItemsPositionOnRaster();

    void onDoubleClickedItem(QGraphicsItem *item);

    QList<QGraphicsItem *> collectCollidingObjectItems(const QGraphicsItem *item, CollidingMode colliding_mode) const;

private:

    void sceneActivated();

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void mousePressEventReparenting(QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void mouseMoveEventReparenting(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEventReparenting(QGraphicsSceneMouseEvent *event);

private slots:

    void onBeginResetAllDiagrams();

    void onEndResetAllDiagrams();

    void onBeginResetDiagram(const MDiagram *diagram);

    void onEndResetDiagram(const MDiagram *diagram);

    void onBeginUpdateElement(int row, const MDiagram *diagram);

    void onEndUpdateElement(int row, const MDiagram *diagram);

    void onBeginInsertElement(int row, const MDiagram *diagram);

    void onEndInsertElement(int row, const MDiagram *diagram);

    void onBeginRemoveElement(int row, const MDiagram *diagram);

    void onEndRemoveElement(int row, const MDiagram *diagram);

private slots:

    void onSelectionChanged();

private:

    void clearGraphicsScene();

    void removeExtraSceneItems();

    void addExtraSceneItems();

    QGraphicsItem *createGraphicsItem(DElement *element);

    void updateGraphicsItem(QGraphicsItem *item, DElement *element);

    void deleteGraphicsItem(QGraphicsItem *item, DElement *element);

    void updateFocusItem(const QSet<QGraphicsItem *> &selected_items);

    void unsetFocusItem();

    bool isInFrontOf(const QGraphicsItem *front_item, const QGraphicsItem *back_item);

private:

    enum Busy {
        NOT_BUSY,
        RESET_DIAGRAM,
        UPDATE_ELEMENT,
        INSERT_ELEMENT,
        REMOVE_ELEMENT
    };

private:

    DiagramController *_diagram_controller;

    DiagramSceneController *_diagram_scene_controller;

    StyleController *_style_controller;

    StereotypeController *_stereotype_controller;

    MDiagram *_diagram;

    DiagramGraphicsScene *_graphics_scene;

    LatchController *_latch_controller;

    QList<QGraphicsItem *> _graphics_items;

    QHash<const QGraphicsItem *, DElement *> _item_to_element_map;

    QHash<const DElement *, QGraphicsItem *> _element_to_item_map;

    QSet<QGraphicsItem *> _selected_items;

    QSet<QGraphicsItem *> _secondary_selected_items;

    Busy _busy;

    OriginItem *_origin_item;

    QGraphicsItem *_focus_item;
};

}

#endif // QMT_DIAGRAMSCENEMODEL_H
