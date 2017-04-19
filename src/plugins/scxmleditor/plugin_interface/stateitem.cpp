/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "stateitem.h"
#include "finalstateitem.h"
#include "graphicsitemprovider.h"
#include "graphicsscene.h"
#include "idwarningitem.h"
#include "imageprovider.h"
#include "initialstateitem.h"
#include "parallelitem.h"
#include "sceneutils.h"
#include "scxmleditorconstants.h"
#include "scxmltagutils.h"
#include "scxmluifactory.h"
#include "statewarningitem.h"
#include "textitem.h"
#include "transitionitem.h"
#include "utilsprovider.h"

#include <QBrush>
#include <QCoreApplication>
#include <QDebug>
#include <QPainter>
#include <QTextOption>
#include <QUndoStack>
#include <QtMath>

using namespace ScxmlEditor::PluginInterface;

const qreal TEXT_ITEM_HEIGHT = 20;
const qreal TEXT_ITEM_CAP = 10;
const qreal STATE_RADIUS = 10;

StateItem::StateItem(const QPointF &pos, BaseItem *parent)
    : ConnectableItem(pos, parent)
{
    m_stateNameItem = new TextItem(this);
    m_stateNameItem->setParentItem(this);

    checkWarningItems();

    connect(m_stateNameItem, &TextItem::selected, this, [=](bool sel){
        setItemSelected(sel);
    });
    connect(m_stateNameItem, &TextItem::textChanged, this, &StateItem::updateTextPositions);
    connect(m_stateNameItem, &TextItem::textReady, this, &StateItem::titleHasChanged);

    m_pen = QPen(QColor(0x45, 0x45, 0x45));

    updateColors();
    updatePolygon();
}

void StateItem::checkWarningItems()
{
    ScxmlUiFactory *uifactory = uiFactory();
    if (uifactory) {
        auto provider = static_cast<GraphicsItemProvider*>(uifactory->object("graphicsItemProvider"));
        if (provider) {
            if (!m_idWarningItem)
                m_idWarningItem = static_cast<IdWarningItem*>(provider->createWarningItem(Constants::C_STATE_WARNING_ID, this));
            if (!m_stateWarningItem)
                m_stateWarningItem = static_cast<StateWarningItem*>(provider->createWarningItem(Constants::C_STATE_WARNING_STATE, this));

            if (m_idWarningItem && m_stateWarningItem)
                m_stateWarningItem->setIdWarning(m_idWarningItem);

            checkWarnings();

            if (m_idWarningItem || m_stateWarningItem)
                updateAttributes();
        }
    }
}

QVariant StateItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant retValue = ConnectableItem::itemChange(change, value);

    switch (change) {
    case QGraphicsItem::ItemSceneHasChanged:
        checkWarningItems();
        break;
    default:
        break;
    }

    return retValue;
}

void StateItem::updateAttributes()
{
    if (tag()) {
        ConnectableItem::updateAttributes();

        // Check initial attribute
        QString strNewId = tagValue("id", true);
        if (!m_parallelState) {
            QStringList NSIDs = strNewId.split(tag()->document()->nameSpaceDelimiter(), QString::SkipEmptyParts);
            if (NSIDs.count() > 0) {
                NSIDs[NSIDs.count() - 1] = m_stateNameItem->toPlainText();
                QString strOldId = NSIDs.join(tag()->document()->nameSpaceDelimiter());
                ScxmlTag *parentTag = tag()->parentTag();
                if (parentTag && !strOldId.isEmpty() && parentTag->attribute("initial") == strOldId)
                    parentTag->setAttribute("initial", strNewId);
            }
        }

        m_stateNameItem->setText(tagValue("id"));
        if (m_idWarningItem)
            m_idWarningItem->setId(strNewId);
        updateTextPositions();

        if (m_parallelState)
            checkInitial(true);
    }

    updateBoundingRect();
}

void StateItem::updateEditorInfo(bool allChildren)
{
    ConnectableItem::updateEditorInfo(allChildren);

    QString color = editorInfo(Constants::C_SCXML_EDITORINFO_FONTCOLOR);
    m_stateNameItem->setDefaultTextColor(color.isEmpty() ? QColor(Qt::black) : QColor(color));

    // Update child too if necessary
    if (allChildren) {
        QList<QGraphicsItem*> children = childItems();
        for (int i = 0; i < children.count(); ++i) {
            if (children[i]->type() >= InitialStateType) {
                auto child = qgraphicsitem_cast<BaseItem*>(children[i]);
                if (child)
                    child->updateEditorInfo(allChildren);
            }
        }
    }
}

void StateItem::updateColors()
{
    updateDepth();
    m_parallelState = parentItem() && parentItem()->type() == ParallelType;

    if (m_parallelState)
        m_pen.setStyle(Qt::DashLine);
    else
        m_pen.setStyle(Qt::SolidLine);

    // Update child color-indices too
    QList<QGraphicsItem*> children = childItems();
    for (int i = 0; i < children.count(); ++i) {
        if (children[i]->type() >= StateType) {
            auto child = qgraphicsitem_cast<StateItem*>(children[i]);
            if (child)
                child->updateColors();
        }
    }

    update();
}

void StateItem::updateBoundingRect()
{
    QRectF r2 = childItemsBoundingRect();

    // Check if we need to increase parent boundingrect
    if (!r2.isNull()) {
        QRectF r = boundingRect();
        QRectF r3 = r.united(r2);

        if (r != r3) {
            setItemBoundingRect(r3);
            updateTransitions();
            updateUIProperties();
            checkOverlapping();
        }
    }
}

void StateItem::shrink()
{
    QRectF trect;
    foreach (TransitionItem *item, outputTransitions()) {
        if (item->targetType() == TransitionItem::InternalSameTarget || item->targetType() == TransitionItem::InternalNoTarget) {
            trect = trect.united(item->wholeBoundingRect());
        }
    }

    QRectF r = boundingRect();
    QRectF r2 = childItemsBoundingRect();
    double minWidth = qMax(120.0, r2.width());
    double minH = qMax((double)minHeight(), r2.height());
    minWidth = qMax(minWidth, m_stateNameItem->boundingRect().width() + (double)WARNING_ITEM_SIZE + (double)TEXT_ITEM_CAP);

    if (!m_backgroundImage.isNull()) {
        minWidth = qMax(minWidth, (double)m_backgroundImage.width() + 3 * STATE_RADIUS + trect.width());
        minH = qMax(minH, ((double)m_backgroundImage.height() + 3 * STATE_RADIUS + TEXT_ITEM_HEIGHT) / 0.94);
    }

    r2 = QRectF(0, 0, minWidth, minH);
    r2.moveCenter(r.center());

    // Check if we need to increase parent boundingrect
    if (r != r2) {
        setItemBoundingRect(r2);
        updateTransitions();
        updateUIProperties();
    }
}

void StateItem::transitionsChanged()
{
    QRectF rr = boundingRect();
    QRectF rectInternalTransitions;
    QVector<TransitionItem*> internalTransitions = outputTransitions();
    foreach (TransitionItem *item, internalTransitions) {
        if (item->targetType() <= TransitionItem::InternalNoTarget) {
            QRectF br = mapFromItem(item, item->boundingRect()).boundingRect();
            br.setLeft(rr.left() + 20);
            br.setTop(br.top() + 4);
            br.setWidth(br.width() + item->textWidth());
            rectInternalTransitions = rectInternalTransitions.united(br);
        }
    }

    m_transitionRect = rectInternalTransitions;
    updateBoundingRect();
}

void StateItem::transitionCountChanged()
{
    ConnectableItem::transitionsChanged();
    checkWarnings();
    transitionsChanged();
}

QRectF StateItem::childItemsBoundingRect() const
{
    QRectF r;
    QRectF rr = boundingRect();

    QList<QGraphicsItem*> children = childItems();
    for (int i = 0; i < children.count(); ++i) {
        if (children[i]->type() >= InitialStateType) {
            QRectF br = children[i]->boundingRect();
            QPointF p = children[i]->pos() + br.topLeft();
            br.moveTopLeft(p);
            r = r.united(br);
        }
    }

    if (m_transitionRect.isValid()) {
        r.setLeft(r.left() - m_transitionRect.width());
        r.setHeight(qMax(r.height(), m_transitionRect.height()));
        r.moveBottom(qMax(r.bottom(), m_transitionRect.bottom()));
    }

    if (!r.isNull())
        r.adjust(-20, -(rr.height() * 0.06 + 40), 20, 20);

    return r;
}

void StateItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    ConnectableItem::mouseDoubleClickEvent(event);
    shrink();
    updateUIProperties();
}

void StateItem::createContextMenu(QMenu *menu)
{
    QVariantMap data;
    if (!m_parallelState) {
        data[Constants::C_SCXMLTAG_ACTIONTYPE] = TagUtils::SetAsInitial;
        menu->addAction(tr("Set as Initial"))->setData(data);
    }

    data[Constants::C_SCXMLTAG_ACTIONTYPE] = TagUtils::ZoomToState;
    menu->addAction(tr("Zoom to State"))->setData(data);

    if (type() == ParallelType) {
        data[Constants::C_SCXMLTAG_ACTIONTYPE] = TagUtils::Relayout;
        menu->addAction(tr("Relayout"))->setData(data);
    }

    menu->addSeparator();
    ConnectableItem::createContextMenu(menu);
}

void StateItem::selectedMenuAction(const QAction *action)
{
    if (!action)
        return;

    const ScxmlTag *tag = this->tag();

    if (tag) {
        const QVariantMap data = action->data().toMap();
        const int actionType = data.value(Constants::C_SCXMLTAG_ACTIONTYPE, -1).toInt();

        ScxmlDocument *document = tag->document();
        switch (actionType) {
        case TagUtils::SetAsInitial: {
            ScxmlTag *parentTag = tag->parentTag();
            if (parentTag) {
                document->undoStack()->beginMacro(tr("Change initial state"));

                ScxmlTag *initialTag = parentTag->child("initial");
                if (initialTag) {
                    ScxmlTag *initialTransitionTag = initialTag->child("transition");
                    if (initialTransitionTag)
                        document->setValue(initialTransitionTag, "target", tag->attribute("id", true));
                    else {
                        auto newTransition = new ScxmlTag(Transition, document);
                        newTransition->setAttribute("target", tag->attribute("id", true));
                        document->addTag(initialTag, newTransition);
                    }
                } else
                    document->setValue(parentTag, "initial", tag->attribute("id", true));

                checkInitial(true);
                document->undoStack()->endMacro();
            }
            break;
        }
        case TagUtils::Relayout: {
            document->undoStack()->beginMacro(tr("Relayout"));
            doLayout(depth());
            document->undoStack()->endMacro();
            break;
        }
        case TagUtils::ZoomToState: {
            emit BaseItem::openToDifferentView(this);
            break;
        }
        default:
            ConnectableItem::selectedMenuAction(action);
            break;
        }
    }
}

void StateItem::checkInitial(bool parent)
{
    QList<QGraphicsItem*> items;
    ScxmlTag *tag = nullptr;

    if (parent) {
        if (parentItem()) {
            items = parentItem()->childItems();
            if (parentBaseItem())
                tag = parentBaseItem()->tag();
        } else {
            auto sc = static_cast<GraphicsScene*>(scene());
            if (sc) {
                sc->checkInitialState();
                return;
            }
        }
    } else {
        items = childItems();
        tag = this->tag();
    }

    if (items.count() > 0 && tag && uiFactory()) {
        auto utilsProvider = static_cast<UtilsProvider*>(uiFactory()->object("utilsProvider"));
        if (utilsProvider)
            utilsProvider->checkInitialState(items, tag);
    }
}

void StateItem::titleHasChanged(const QString &text)
{
    QString strOldId = tagValue("id", true);
    setTagValue("id", text);

    // Check initial attribute
    if (tag() && !m_parallelState) {
        ScxmlTag *parentTag = tag()->parentTag();
        if (!strOldId.isEmpty() && parentTag->attribute("initial") == strOldId)
            parentTag->setAttribute("initial", tagValue("id", true));
    }
}

void StateItem::updateTextPositions()
{
    if (m_parallelState) {
        m_stateNameItem->setPos(m_titleRect.x(), m_titleRect.top());
        m_stateNameItem->setItalic(true);
    } else {
        m_stateNameItem->setPos(m_titleRect.center().x() - m_stateNameItem->boundingRect().width() / 2, m_titleRect.top());
        m_stateNameItem->setItalic(false);
    }

    QPointF p(m_stateNameItem->pos().x() - WARNING_ITEM_SIZE, m_titleRect.center().y() - WARNING_ITEM_SIZE / 2);
    if (m_idWarningItem)
        m_idWarningItem->setPos(p);
    if (m_stateWarningItem)
        m_stateWarningItem->setPos(p);
}

void StateItem::updatePolygon()
{
    m_drawingRect = boundingRect().adjusted(5, 5, -5, -5);

    m_polygon.clear();
    m_polygon << m_drawingRect.topLeft()
              << m_drawingRect.topRight()
              << m_drawingRect.bottomRight()
              << m_drawingRect.bottomLeft()
              << m_drawingRect.topLeft();

    m_titleRect = QRectF(m_drawingRect.left(), m_drawingRect.top(), m_drawingRect.width(), TEXT_ITEM_HEIGHT + m_drawingRect.height() * 0.06);
    QFont f = m_stateNameItem->font();
    f.setPixelSize(m_titleRect.height() * 0.65);
    m_stateNameItem->setFont(f);

    updateTextPositions();
}

bool StateItem::canStartTransition(ItemType /*type*/) const
{
    return !m_parallelState;
}

void StateItem::connectToParent(BaseItem *parentItem)
{
    ConnectableItem::connectToParent(parentItem);
    updateTextPositions();
    checkInitial(true);
}

void StateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    ConnectableItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Set opacity and color
    painter->setOpacity(getOpacity());
    m_pen.setColor(overlapping() ? qRgb(0xff, 0x00, 0x60) : qRgb(0x45, 0x45, 0x45));
    painter->setPen(m_pen);
    QColor stateColor(editorInfo(Constants::C_SCXML_EDITORINFO_STATECOLOR));
    if (!stateColor.isValid())
        stateColor = tag() ? tag()->document()->getColor(depth()) : QColor(0x12, 0x34, 0x56);

    // Draw basic frame
    QRectF r = boundingRect();
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    grad.setColorAt(0, stateColor.lighter(115));
    grad.setColorAt(1, stateColor);
    painter->setBrush(QBrush(grad));

    painter->drawRoundedRect(m_drawingRect, STATE_RADIUS, STATE_RADIUS);

    // Background image
    ScxmlUiFactory *uifactory = uiFactory();
    if (uifactory) {
        auto imageProvider = static_cast<ImageProvider*>(uifactory->object("imageProvider"));
        if (imageProvider) {
            const QImage *image = imageProvider->backgroundImage(tag());
            if (image) {
                int w = m_drawingRect.width() - 2 * STATE_RADIUS;
                int h = m_drawingRect.height() - m_titleRect.height() - 2 * STATE_RADIUS;
                int left = m_drawingRect.left() + STATE_RADIUS;
                int top = m_drawingRect.top() + m_titleRect.height() + STATE_RADIUS;
                if (m_transitionRect.isValid()) {
                    w = m_drawingRect.right() - m_transitionRect.right() - 2 * STATE_RADIUS;
                    left = m_transitionRect.right() + STATE_RADIUS;
                }

                m_backgroundImage = image->scaled(w, h, Qt::KeepAspectRatio);
                painter->drawImage(QPointF(left + (w - m_backgroundImage.width()) / 2, top + (h - m_backgroundImage.height()) / 2), m_backgroundImage);
            } else
                m_backgroundImage = QImage();
        }
    }

    // Draw title rect
    if (!m_parallelState) {
        painter->drawLine(QLineF(m_titleRect.bottomLeft(), m_titleRect.bottomRight()));

        if (m_initial) {
            double size = m_titleRect.height() * 0.3;
            painter->setBrush(QColor(0x4d, 0x4d, 0x4d));
            painter->drawEllipse(QPointF(m_titleRect.left() + 2 * size, m_titleRect.center().y()), size, size);
        }
    }

    // Transition part
    if (m_transitionRect.isValid()) {
        qreal tx = m_transitionRect.right();
        painter->drawLine(QLineF(QPointF(tx, m_titleRect.bottom() + STATE_RADIUS), QPointF(tx, m_drawingRect.bottom() - STATE_RADIUS)));
    }

    painter->restore();
}

void StateItem::init(ScxmlTag *tag, BaseItem *parentItem, bool initChildren, bool blockUpdates)
{
    setBlockUpdates(blockUpdates);
    ConnectableItem::init(tag, parentItem);

    if (initChildren) {
        for (int i = 0; i < tag->childCount(); ++i) {
            ScxmlTag *child = tag->child(i);
            ConnectableItem *newItem = SceneUtils::createItemByTagType(child->tagType(), QPointF());
            if (newItem) {
                newItem->init(child, this, initChildren, blockUpdates);
                newItem->finalizeCreation();
            }
        }
    }
    if (blockUpdates)
        setBlockUpdates(false);
}

QString StateItem::itemId() const
{
    return m_stateNameItem ? m_stateNameItem->toPlainText() : QString();
}

void StateItem::doLayout(int d)
{
    if (depth() != d)
        return;

    SceneUtils::layout(childItems());
    updateBoundingRect();
    shrink();
}

void StateItem::setInitial(bool initial)
{
    m_initial = initial;
    update();
    checkWarnings();
}

void StateItem::checkWarnings()
{
    if (m_idWarningItem)
        m_idWarningItem->check();
    if (m_stateWarningItem)
        m_stateWarningItem->check();

    if (parentItem() && parentItem()->type() == StateType)
        qgraphicsitem_cast<StateItem*>(parentItem())->checkWarnings();
}

bool StateItem::isInitial() const
{
    return m_initial;
}
