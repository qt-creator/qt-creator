/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitioneditorgraphicsscene.h"

#include "transitioneditorgraphicslayout.h"
#include "transitioneditorpropertyitem.h"
#include "transitioneditorsectionitem.h"
#include "transitioneditortoolbar.h"
#include "transitioneditorview.h"
#include "transitioneditorwidget.h"

#include "timelineactions.h"
#include "timelineitem.h"
#include "timelinemovableabstractitem.h"
#include "timelinemovetool.h"
#include "timelineplaceholder.h"
#include "timelinepropertyitem.h"
#include "timelinesectionitem.h"

#include <designdocumentview.h>
#include <exception.h>
#include <rewritertransaction.h>
#include <rewriterview.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <qmltimelinekeyframegroup.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>

#include <cmath>

namespace QmlDesigner {

static int deleteKey()
{
    if (Utils::HostOsInfo::isMacHost())
        return Qt::Key_Backspace;

    return Qt::Key_Delete;
}

TransitionEditorGraphicsScene::TransitionEditorGraphicsScene(TransitionEditorWidget *parent)
    : AbstractScrollGraphicsScene(parent)
    , m_parent(parent)
    , m_layout(new TransitionEditorGraphicsLayout(this))
    , m_tools(this)
{
    addItem(m_layout);

    setSceneRect(m_layout->geometry());

    connect(m_layout, &QGraphicsWidget::geometryChanged, this, [this]() {
        auto rect = m_layout->geometry();

        setSceneRect(rect);

        if (auto *gview = graphicsView())
            gview->setSceneRect(rect.adjusted(0, TimelineConstants::rulerHeight, 0, 0));

        if (auto *rview = rulerView())
            rview->setSceneRect(rect);
    });

    auto changeScale = [this](int factor) {
        transitionEditorWidget()->changeScaleFactor(factor);
        setRulerScaling(qreal(factor));
    };
    connect(m_layout, &TransitionEditorGraphicsLayout::scaleFactorChanged, changeScale);
}

TransitionEditorGraphicsScene::~TransitionEditorGraphicsScene()
{
    QSignalBlocker block(this);
    qDeleteAll(items());
}

void TransitionEditorGraphicsScene::invalidateScrollbar()
{
    double max = m_layout->maximumScrollValue();
    transitionEditorWidget()->setupScrollbar(0, max, scrollOffset());
    if (scrollOffset() > max)
        setScrollOffset(max);
}

void TransitionEditorGraphicsScene::onShow()
{
    emit m_layout->scaleFactorChanged(0);
}

void TransitionEditorGraphicsScene::setTransition(const ModelNode &transition)
{
    clearSelection();
    m_layout->setTransition(transition);
}

void TransitionEditorGraphicsScene::clearTransition()
{
    m_transition = {};
    m_layout->setTransition({});
}

void TransitionEditorGraphicsScene::setWidth(int width)
{
    m_layout->setWidth(width);
    invalidateScrollbar();
}

void TransitionEditorGraphicsScene::invalidateLayout()
{
    m_layout->invalidate();
}

void TransitionEditorGraphicsScene::setDuration(int duration)
{
    if (m_transition.isValid())
        m_transition.setAuxiliaryData("transitionDuration", duration);
    m_layout->setDuration(duration);
    qreal scaling = m_layout->rulerScaling();
    setRulerScaling(scaling);
}

qreal TransitionEditorGraphicsScene::rulerScaling() const
{
    return m_layout->rulerScaling();
}

int TransitionEditorGraphicsScene::rulerWidth() const
{
    return m_layout->rulerWidth();
}

qreal TransitionEditorGraphicsScene::rulerDuration() const
{
    return m_layout->rulerDuration();
}

qreal TransitionEditorGraphicsScene::endFrame() const
{
    return m_layout->endFrame();
}

qreal TransitionEditorGraphicsScene::startFrame() const
{
    return 0;
}

qreal TransitionEditorGraphicsScene::mapToScene(qreal x) const
{
    return TimelineConstants::sectionWidth + TimelineConstants::timelineLeftOffset
           + (x - startFrame()) * rulerScaling() - scrollOffset();
}

qreal TransitionEditorGraphicsScene::mapFromScene(qreal x) const
{
    auto xPosOffset = (x - TimelineConstants::sectionWidth - TimelineConstants::timelineLeftOffset)
                      + scrollOffset();

    return xPosOffset / rulerScaling() + startFrame();
}

void TransitionEditorGraphicsScene::setRulerScaling(int scaleFactor)
{
    m_layout->setRulerScaleFactor(scaleFactor);

    setScrollOffset(0);
    invalidateSections();
    invalidateScrollbar();
    update();
}

void TransitionEditorGraphicsScene::invalidateSectionForTarget(const ModelNode &target)
{
    if (!target.isValid())
        return;

    bool found = false;

    const QList<QGraphicsItem *> items = m_layout->childItems();
    for (auto child : items)
        TimelineSectionItem::updateDataForTarget(child, target, &found);

    if (!found)
        invalidateScene();

    clearSelection();
    invalidateLayout();
}

void TransitionEditorGraphicsScene::invalidateScene()
{
    invalidateScrollbar();
}

void TransitionEditorGraphicsScene::invalidateCurrentValues()
{
    const QList<QGraphicsItem *> constItems = items();
    for (auto item : constItems)
        TimelinePropertyItem::updateTextEdit(item);
}

QGraphicsView *TransitionEditorGraphicsScene::graphicsView() const
{
    const QList<QGraphicsView *> constViews = views();
    for (auto *v : constViews)
        if (v->objectName() == "SceneView")
            return v;

    return nullptr;
}

QGraphicsView *TransitionEditorGraphicsScene::rulerView() const
{
    const QList<QGraphicsView *> constViews = views();
    for (auto *v : constViews)
        if (v->objectName() == "RulerView")
            return v;

    return nullptr;
}

QRectF TransitionEditorGraphicsScene::selectionBounds() const
{
    QRectF bbox;

    return bbox;
}

void TransitionEditorGraphicsScene::clearSelection()
{
    if (m_selectedProperty)
        m_selectedProperty->update();

    m_selectedProperty = nullptr;
    AbstractScrollGraphicsScene::clearSelection();
}

QList<QGraphicsItem *> TransitionEditorGraphicsScene::itemsAt(const QPointF &pos)
{
    QTransform transform;

    if (auto *gview = graphicsView())
        transform = gview->transform();

    return items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder, transform);
}

void TransitionEditorGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));

    m_tools.mousePressEvent(topItem, event);
    QGraphicsScene::mousePressEvent(event);
}

void TransitionEditorGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    m_tools.mouseMoveEvent(topItem, event);
    QGraphicsScene::mouseMoveEvent(event);
}

void TransitionEditorGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    /* The tool has handle the event last. */
    QGraphicsScene::mouseReleaseEvent(event);
    m_tools.mouseReleaseEvent(topItem, event);
}

void TransitionEditorGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    m_tools.mouseDoubleClickEvent(topItem, event);
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void TransitionEditorGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if (qgraphicsitem_cast<QGraphicsProxyWidget *>(focusItem())) {
        keyEvent->ignore();
        QGraphicsScene::keyPressEvent(keyEvent);
        return;
    }

    if (keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
        QGraphicsScene::keyPressEvent(keyEvent);
    } else {
        switch (keyEvent->key()) {
        case Qt::Key_Left:
            emit scroll(TimelineUtils::Side::Left);
            keyEvent->accept();
            break;

        case Qt::Key_Right:
            emit scroll(TimelineUtils::Side::Right);
            keyEvent->accept();
            break;

        default:
            QGraphicsScene::keyPressEvent(keyEvent);
            break;
        }
    }
}

void TransitionEditorGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (qgraphicsitem_cast<QGraphicsProxyWidget *>(focusItem())) {
        keyEvent->ignore();
        QGraphicsScene::keyReleaseEvent(keyEvent);
        return;
    }

    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void TransitionEditorGraphicsScene::invalidateSections()
{
    const QList<QGraphicsItem *> children = m_layout->childItems();
    for (auto child : children)
        TransitionEditorSectionItem::updateData(child);

    clearSelection();
    invalidateLayout();
}

TransitionEditorView *TransitionEditorGraphicsScene::transitionEditorView() const
{
    return m_parent->transitionEditorView();
}

TransitionEditorWidget *TransitionEditorGraphicsScene::transitionEditorWidget() const
{
    return m_parent;
}

TransitionEditorToolBar *TransitionEditorGraphicsScene::toolBar() const
{
    return transitionEditorWidget()->toolBar();
}

void TransitionEditorGraphicsScene::activateLayout()
{
    m_layout->activate();
}

AbstractView *TransitionEditorGraphicsScene::abstractView() const
{
    return transitionEditorView();
}

bool TransitionEditorGraphicsScene::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(event)->key() == deleteKey()) {
            QGraphicsScene::keyPressEvent(static_cast<QKeyEvent *>(event));
            event->accept();
            return true;
        }
        Q_FALLTHROUGH();
    default:
        return QGraphicsScene::event(event);
    }
}

ModelNode TransitionEditorGraphicsScene::transitionModelNode() const
{
    if (transitionEditorView()->isAttached()) {
        const QString timelineId = transitionEditorWidget()->toolBar()->currentTransitionId();
        return transitionEditorView()->modelNodeForId(timelineId);
    }

    return ModelNode();
}

TransitionEditorPropertyItem *TransitionEditorGraphicsScene::selectedPropertyItem() const
{
    return m_selectedProperty;
}

void TransitionEditorGraphicsScene::setSelectedPropertyItem(TransitionEditorPropertyItem *item)
{
    if (m_selectedProperty)
        m_selectedProperty->update();
    m_selectedProperty = item;
    emit selectionChanged();
}

} // namespace QmlDesigner
