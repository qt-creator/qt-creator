/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
class QKeyEvent;
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
class ObjectItem;

class QMT_EXPORT DiagramSceneModel : public QObject
{
    Q_OBJECT

    class CreationVisitor;
    class UpdateVisitor;
    class OriginItem;

    friend class UpdateVisitor;
    friend class DiagramGraphicsScene;

public:
    enum CollidingMode {
        CollidingInnerItems,
        CollidingItems,
        CollidingOuterItems
    };

    explicit DiagramSceneModel(QObject *parent = nullptr);
    ~DiagramSceneModel() override;

signals:
    void diagramSceneActivated(const MDiagram *diagram);
    void selectionHasChanged(const MDiagram *diagram);
    void sceneRectChanged(const QRectF &sceneRect);

public:
    DiagramController *diagramController() const { return m_diagramController; }
    void setDiagramController(DiagramController *diagramController);
    DiagramSceneController *diagramSceneController() const { return m_diagramSceneController; }
    void setDiagramSceneController(DiagramSceneController *diagramSceneController);
    StyleController *styleController() const { return m_styleController; }
    void setStyleController(StyleController *styleController);
    StereotypeController *stereotypeController() const { return m_stereotypeController; }
    void setStereotypeController(StereotypeController *stereotypeController);
    MDiagram *diagram() const { return m_diagram; }
    void setDiagram(MDiagram *diagram);
    QGraphicsScene *graphicsScene() const;

    bool hasSelection() const;
    bool hasMultiObjectsSelection() const;
    DSelection selectedElements() const;
    DElement *findTopmostElement(const QPointF &scenePos) const;
    DObject *findTopmostObject(const QPointF &scenePos) const;
    ObjectItem *findTopmostObjectItem(const QPointF &scenePos) const;

    QList<QGraphicsItem *> graphicsItems() const { return m_graphicsItems; }
    QGraphicsItem *graphicsItem(DElement *element) const;
    QGraphicsItem *graphicsItem(const Uid &uid) const;
    QGraphicsItem *focusItem() const { return m_focusItem; }
    bool isSelectedItem(QGraphicsItem *item) const;
    QSet<QGraphicsItem *> selectedItems() const { return m_selectedItems; }
    DElement *element(QGraphicsItem *item) const;
    bool isElementEditable(const DElement *element) const;

    void selectAllElements();
    void selectElement(DElement *element);
    void editElement(DElement *element);
    void copyToClipboard();
    bool exportImage(const QString &fileName);
    bool exportPdf(const QString &fileName);
    bool exportSvg(const QString &fileName);

    void selectItem(QGraphicsItem *item, bool multiSelect);
    void moveSelectedItems(QGraphicsItem *grabbedItem, const QPointF &delta);
    void alignSelectedItemsPositionOnRaster();
    void onDoubleClickedItem(QGraphicsItem *item);
    QList<QGraphicsItem *> collectCollidingObjectItems(const QGraphicsItem *item,
                                                       CollidingMode collidingMode) const;

private:
    void sceneActivated();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEventReparenting(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEventReparenting(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEventReparenting(QGraphicsSceneMouseEvent *event);

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

    void onSelectionChanged();

    void clearGraphicsScene();
    void removeExtraSceneItems();
    void addExtraSceneItems();
    void recalcSceneRectSize();
    QGraphicsItem *createGraphicsItem(DElement *element);
    void updateGraphicsItem(QGraphicsItem *item, DElement *element);
    void deleteGraphicsItem(QGraphicsItem *item, DElement *element);
    void updateFocusItem(const QSet<QGraphicsItem *> &selectedItems);
    void unsetFocusItem();
    bool isInFrontOf(const QGraphicsItem *frontItem, const QGraphicsItem *backItem);

    enum Busy {
        NotBusy,
        ResetDiagram,
        UpdateElement,
        InsertElement,
        RemoveElement
    };

    DiagramController *m_diagramController = nullptr;
    DiagramSceneController *m_diagramSceneController = nullptr;
    StyleController *m_styleController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    MDiagram *m_diagram = nullptr;
    DiagramGraphicsScene *m_graphicsScene = nullptr;
    LatchController *m_latchController = nullptr;
    QList<QGraphicsItem *> m_graphicsItems;
    QHash<const QGraphicsItem *, DElement *> m_itemToElementMap;
    QHash<const DElement *, QGraphicsItem *> m_elementToItemMap;
    QSet<QGraphicsItem *> m_selectedItems;
    QSet<QGraphicsItem *> m_secondarySelectedItems;
    Busy m_busyState = NotBusy;
    OriginItem *m_originItem = nullptr;
    QGraphicsItem *m_focusItem = nullptr;
};

} // namespace qmt
