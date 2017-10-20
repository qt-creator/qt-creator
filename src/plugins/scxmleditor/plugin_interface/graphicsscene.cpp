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

#include "graphicsscene.h"
#include "actionhandler.h"
#include "actionprovider.h"
#include "finalstateitem.h"
#include "initialstateitem.h"
#include "parallelitem.h"
#include "sceneutils.h"
#include "scxmleditorconstants.h"
#include "scxmltagutils.h"
#include "scxmluifactory.h"
#include "snapline.h"
#include "stateitem.h"
#include "transitionitem.h"
#include "utilsprovider.h"
#include "warning.h"
#include "warningitem.h"

#include <utils/qtcassert.h>

#include <QAction>
#include <QGuiApplication>
#include <QClipboard>
#include <QMenu>
#include <QMimeData>
#include <QThread>
#include <QUndoStack>

using namespace ScxmlEditor::PluginInterface;

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    //setMinimumRenderSize(5);
    setItemIndexMethod(QGraphicsScene::NoIndex);
}

GraphicsScene::~GraphicsScene()
{
    clear();
}

void GraphicsScene::unselectAll()
{
    const QList<QGraphicsItem*> selectedItems = this->selectedItems();
    foreach (QGraphicsItem *it, selectedItems)
        it->setSelected(false);
    if (m_document)
        m_document->setCurrentTag(0);
}

void GraphicsScene::unhighlightAll()
{
    foreach (BaseItem *it, m_baseItems)
        it->setHighlight(false);
}

void GraphicsScene::highlightItems(const QVector<ScxmlTag*> &lstIds)
{
    foreach (BaseItem *it, m_baseItems)
        it->setHighlight(lstIds.contains(it->tag()));
}

QRectF GraphicsScene::selectedBoundingRect() const
{
    QRectF r;
    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected())
            r = r.united(item->sceneBoundingRect());
    }
    return r;
}

qreal GraphicsScene::selectedMaxWidth() const
{
    qreal maxw = 0;
    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected() && item->type() >= InitialStateType)
            maxw = qMax(maxw, item->sceneBoundingRect().width());
    }
    return maxw;
}

qreal GraphicsScene::selectedMaxHeight() const
{
    qreal maxh = 0;
    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected() && item->type() >= InitialStateType)
            maxh = qMax(maxh, item->sceneBoundingRect().height());
    }
    return maxh;
}

void GraphicsScene::alignStates(int alignType)
{
    if (alignType >= ActionAlignLeft && alignType <= ActionAlignVertical) {
        m_document->undoStack()->beginMacro(tr("Align states"));
        QRectF r = selectedBoundingRect();

        if (r.isValid()) {
            switch (alignType) {
            case ActionAlignLeft:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(r.left() - item->sceneBoundingRect().left(), 0);
                }
                break;
            case ActionAlignRight:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(r.right() - item->sceneBoundingRect().right(), 0);
                }
                break;
            case ActionAlignTop:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(0, r.top() - item->sceneBoundingRect().top());
                }
                break;
            case ActionAlignBottom:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(0, r.bottom() - item->sceneBoundingRect().bottom());
                }
                break;
            case ActionAlignHorizontal:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(0, r.center().y() - item->sceneBoundingRect().center().y());
                }
                break;
            case ActionAlignVertical:
                foreach (BaseItem *item, m_baseItems) {
                    if (item->isSelected() && item->type() >= InitialStateType)
                        item->moveStateBy(r.center().x() - item->sceneBoundingRect().center().x(), 0);
                }
                break;
            default:
                break;
            }
        }

        m_document->undoStack()->endMacro();
    }
}

void GraphicsScene::adjustStates(int adjustType)
{
    if (adjustType >= ActionAdjustWidth && adjustType <= ActionAdjustSize) {
        m_document->undoStack()->beginMacro(tr("Adjust states"));
        qreal maxw = selectedMaxWidth();
        qreal maxh = selectedMaxHeight();

        foreach (BaseItem *item, m_baseItems) {
            if (item->isSelected() && item->type() >= InitialStateType) {
                QRectF rr = item->boundingRect();
                if ((adjustType == ActionAdjustWidth || adjustType == ActionAdjustSize) && !qFuzzyCompare(rr.width(), maxw))
                    rr.setWidth(maxw);
                if ((adjustType == ActionAdjustHeight || adjustType == ActionAdjustSize) && !qFuzzyCompare(rr.height(), maxh))
                    rr.setHeight(maxh);

                item->setItemBoundingRect(rr);
                qgraphicsitem_cast<ConnectableItem*>(item)->updateTransitions(true);
            }
        }
        m_document->undoStack()->endMacro();
    }
}
void GraphicsScene::cut()
{
    m_document->undoStack()->beginMacro(tr("Cut"));
    copy();
    removeSelectedItems();
    m_document->undoStack()->endMacro();
}

void GraphicsScene::removeSelectedItems()
{
    QVector<ScxmlTag*> tags = SceneUtils::findRemovedTags(m_baseItems);
    if (tags.count() > 0) {
        m_document->undoStack()->beginMacro(tr("Remove items"));

        // Then remove found tags
        for (int i = tags.count(); i--;) {
            m_document->setCurrentTag(tags[i]);
            m_document->removeTag(tags[i]);
        }
        m_document->setCurrentTag(0);
        m_document->undoStack()->endMacro();
    }
}

void GraphicsScene::copy()
{
    if (!m_document->currentTag())
        return;

    QPointF minPos;
    QVector<ScxmlTag*> tags;
    if (m_document->currentTag()->tagType() == Scxml) {
        QVector<BaseItem*> items;
        foreach (BaseItem *item, m_baseItems) {
            if (!item->parentItem())
                items << item;
        }
        tags = SceneUtils::findCopyTags(items, minPos);
    } else {
        tags = SceneUtils::findCopyTags(m_baseItems, minPos);
    }

    if (tags.isEmpty() && m_document->currentTag())
        tags << m_document->currentTag();

    if (tags.count() > 0) {
        auto mime = new QMimeData;
        QByteArray result = m_document->content(tags);
        mime->setText(QLatin1String(result));
        mime->setData("StateChartEditor/StateData", result);
        QStringList strTypes;
        foreach (const ScxmlTag *tag, tags) {
            strTypes << tag->tagName(false);
        }
        mime->setData("StateChartEditor/CopiedTagTypes", strTypes.join(",").toLocal8Bit());
        mime->setData("StateChartEditor/CopiedMinPos", QString::fromLatin1("%1:%2").arg(minPos.x()).arg(minPos.y()).toLocal8Bit());
        QGuiApplication::clipboard()->setMimeData(mime);
    }

    checkPaste();
}

void GraphicsScene::checkPaste()
{
    const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
    const QString copiedTagTypes = QLatin1String(mimeData->data("StateChartEditor/CopiedTagTypes"));

    emit pasteAvailable(TagUtils::checkPaste(copiedTagTypes, m_document->currentTag()));
}

void GraphicsScene::paste(const QPointF &targetPos)
{
    const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();

    QPointF startPos(targetPos);

    BaseItem *targetItem = nullptr;
    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected() && item->type() >= StateType) {
            targetItem = item;
            break;
        }
    }

    if (m_lastPasteTargetItem != targetItem)
        m_pasteCounter = 0;
    m_lastPasteTargetItem = targetItem;

    if (m_lastPasteTargetItem)
        startPos = m_lastPasteTargetItem->boundingRect().topLeft();
    QPointF pastedPos = startPos + QPointF(m_pasteCounter * 30, m_pasteCounter * 30);
    m_pasteCounter++;

    QString strMinPos = QLatin1String(mimeData->data("StateChartEditor/CopiedMinPos"));
    QPointF minPos(0, 0);
    if (!strMinPos.isEmpty()) {
        QStringList coords = strMinPos.split(":", QString::SkipEmptyParts);
        if (coords.count() == 2)
            minPos = QPointF(coords[0].toDouble(), coords[1].toDouble());
    }

    m_document->pasteData(mimeData->data("StateChartEditor/StateData"), minPos, pastedPos);
}

void GraphicsScene::setEditorInfo(const QString &key, const QString &value)
{
    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected() && item->type() >= TransitionType)
            item->setEditorInfo(key, value);
    }
}

void GraphicsScene::setDocument(ScxmlDocument *document)
{
    if (m_document)
        disconnect(m_document, 0, this, 0);

    m_document = document;

    init();
    connectDocument();
}

void GraphicsScene::connectDocument()
{
    if (m_document) {
        connect(m_document.data(), &ScxmlDocument::beginTagChange, this, &GraphicsScene::beginTagChange);
        connect(m_document.data(), &ScxmlDocument::endTagChange, this, &GraphicsScene::endTagChange);
    }
}

void GraphicsScene::disconnectDocument()
{
    if (m_document)
        m_document->disconnect(this);
}

void GraphicsScene::init()
{
    m_initializing = true;

    disconnectDocument();
    clear();
    addItem(m_lineX = new SnapLine);
    addItem(m_lineY = new SnapLine);

    if (m_document) {
        const ScxmlTag *rootTag = m_document->rootTag();
        if (rootTag) {

            for (int i = 0; i < rootTag->childCount(); ++i) {
                ScxmlTag *child = rootTag->child(i);
                ConnectableItem *newItem = SceneUtils::createItemByTagType(child->tagType());
                if (newItem) {
                    addItem(newItem);
                    newItem->init(child);
                }
            }

            const QList<QGraphicsItem*> items = this->items();
            for (int i = 0; i < items.count(); ++i) {
                if (items[i]->type() >= TransitionType) {
                    auto item = qgraphicsitem_cast<BaseItem*>(items[i]);
                    if (item)
                        item->finalizeCreation();
                }
            }
        }
    }

    m_initializing = false;
    warningVisibilityChanged(0, 0);
    emit selectedStateCountChanged(0);
    emit selectedBaseItemCountChanged(0);
}

void GraphicsScene::runLayoutToSelectedStates()
{
    m_document->undoStack()->beginMacro(tr("Re-layout"));

    QVector<BaseItem*> selectedItems;
    foreach (BaseItem *node, m_baseItems) {
        if (node->isSelected()) {
            int index = 0;
            for (int i = 0; i < selectedItems.count(); ++i) {
                if (node->depth() <= selectedItems[i]->depth()) {
                    index = i;
                    break;
                }
            }
            selectedItems.insert(index, node);
        }
    }

    // Layout selected items
    for (int i = 0; i < selectedItems.count(); ++i)
        selectedItems[i]->doLayout(selectedItems[i]->depth());

    // Layout scene items if necessary
    if (selectedItems.isEmpty()) {
        QList<QGraphicsItem*> sceneItems;
        foreach (BaseItem *item, m_baseItems) {
            if (item->type() >= InitialStateType && !item->parentItem())
                sceneItems << item;
        }
        SceneUtils::layout(sceneItems);

        foreach (QGraphicsItem *item, sceneItems) {
            if (item->type() >= StateType)
                static_cast<StateItem*>(item)->shrink();
        }
    }

    // Update properties
    foreach (BaseItem *node, selectedItems) {
        node->updateUIProperties();
    }

    m_document->undoStack()->endMacro();
}

void GraphicsScene::runAutomaticLayout()
{
    m_autoLayoutRunning = true;

    // 1. Find max depth
    int maxDepth = 0;
    foreach (BaseItem *node, m_baseItems) {
        maxDepth = qMax(maxDepth, node->depth());
        node->setBlockUpdates(true);
    }

    // 2. Layout every depth-level separately
    for (int d = (maxDepth + 1); d--;) {
        foreach (BaseItem *node, m_baseItems)
            node->doLayout(d);
    }

    // 3. Layout scene items
    QList<QGraphicsItem*> sceneItems;
    foreach (BaseItem *item, m_baseItems) {
        if (item->type() >= InitialStateType && !item->parentItem())
            sceneItems << item;
    }
    SceneUtils::layout(sceneItems);

    foreach (QGraphicsItem *item, sceneItems) {
        if (item->type() >= StateType)
            static_cast<StateItem*>(item)->shrink();
    }

    foreach (BaseItem *node, m_baseItems) {
        node->updateUIProperties();
        node->setBlockUpdates(false);
    }

    m_autoLayoutRunning = false;
}

void GraphicsScene::beginTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    switch (change) {
    case ScxmlDocument::TagRemoveChild: {
        if (tag)
            removeItems(tag->child(value.toInt()));
        break;
    }
    default:
        break;
    }
}

void GraphicsScene::endTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    Q_UNUSED(value)
    QTC_ASSERT(tag, return);

    switch (change) {
    case ScxmlDocument::TagAttributesChanged: {
        foreach (BaseItem *item, m_baseItems) {
            if (item->tag() == tag)
                item->updateAttributes();
        }
        break;
    }
    case ScxmlDocument::TagEditorInfoChanged: {
        foreach (BaseItem *item, m_baseItems) {
            if (item->tag() == tag)
                item->updateEditorInfo();
        }
        break;
    }
    case ScxmlDocument::TagCurrentChanged: {
        foreach (BaseItem *item, m_baseItems) {
            if (!item->isSelected() && item->tag() == tag)
                item->setSelected(true);
        }
        checkPaste();
        break;
    }
    case ScxmlDocument::TagChangeParent: {
        auto childItem = qobject_cast<ConnectableItem*>(findItem(tag));

        if (childItem) {
            BaseItem *newParentItem = findItem(tag->parentTag());
            BaseItem *oldParentItem = childItem->parentBaseItem();

            QPointF sPos = childItem->scenePos();
            if (oldParentItem) {
                childItem->setParentItem(nullptr);
                childItem->setPos(sPos);
            }

            if (newParentItem)
                childItem->setPos(newParentItem->mapFromScene(childItem->sceneBoundingRect().center()) - childItem->boundingRect().center());

            childItem->setParentItem(newParentItem);
            childItem->updateUIProperties();
            childItem->updateTransitions(true);
            childItem->updateTransitionAttributes(true);
            childItem->checkWarnings();
            childItem->checkInitial();
            if (newParentItem) {
                newParentItem->checkInitial();
                newParentItem->updateAttributes();
                newParentItem->checkWarnings();
                newParentItem->checkOverlapping();
                newParentItem->updateUIProperties();
            }

            if (oldParentItem)
                oldParentItem->checkInitial();

            if (!oldParentItem || !newParentItem)
                checkInitialState();
        }
        break;
    }
    case ScxmlDocument::TagAddTags: {
        // Finalize transitions
        QVector<ScxmlTag*> childTransitionTags;
        if (tag->tagName(false) == "transition")
            childTransitionTags << tag;

        TagUtils::findAllTransitionChildren(tag, childTransitionTags);
        for (int i = 0; i < childTransitionTags.count(); ++i) {
            BaseItem *item = findItem(childTransitionTags[i]);
            if (item)
                item->finalizeCreation();
        }
    }
    break;
    case ScxmlDocument::TagAddChild: {
        ScxmlTag *childTag = tag->child(value.toInt());
        if (childTag) {
            // Check that there is no any item with this tag
            BaseItem *childItem = findItem(childTag);
            BaseItem *parentItem = findItem(tag);
            if (!childItem) {
                if (childTag->tagType() == Transition || childTag->tagType() == InitialTransition) {
                    auto transition = new TransitionItem;
                    addItem(transition);
                    transition->setStartItem(qgraphicsitem_cast<ConnectableItem*>(parentItem));
                    transition->init(childTag, 0, false, false);
                    transition->updateAttributes();
                } else {
                    childItem = SceneUtils::createItemByTagType(childTag->tagType(), QPointF());
                    if (childItem) {
                        childItem->init(childTag, parentItem, false);
                        if (!parentItem)
                            addItem(childItem);

                        childItem->finalizeCreation();
                        childItem->updateUIProperties();
                    }
                }
            }

            if (parentItem) {
                parentItem->updateAttributes();
                parentItem->updateUIProperties();
                parentItem->checkInitial();
            } else
                checkInitialState();
        }
        break;
    }
    case ScxmlDocument::TagRemoveChild: {
        BaseItem *parentItem = findItem(tag);
        if (parentItem) {
            parentItem->updateAttributes();
            parentItem->checkInitial();
        } else {
            checkInitialState();
        }
        break;
    }
    case ScxmlDocument::TagChangeOrder: {
        BaseItem *parentItem = findItem(tag->parentTag());
        if (parentItem)
            parentItem->updateAttributes();
        else
            checkInitialState();
    }
    default:
        break;
    }
}

void GraphicsScene::setTopMostScene(bool topmost)
{
    m_topMostScene = topmost;
}

bool GraphicsScene::topMostScene() const
{
    return m_topMostScene;
}

void GraphicsScene::setActionHandler(ActionHandler *mgr)
{
    m_actionHandler = mgr;
}

void GraphicsScene::setWarningModel(ScxmlEditor::OutputPane::WarningModel *model)
{
    m_warningModel = model;
}

void GraphicsScene::setUiFactory(ScxmlUiFactory *uifactory)
{
    m_uiFactory = uifactory;
}

ActionHandler *GraphicsScene::actionHandler() const
{
    return m_actionHandler;
}

ScxmlEditor::OutputPane::WarningModel *GraphicsScene::warningModel() const
{
    return m_warningModel;
}

ScxmlUiFactory *GraphicsScene::uiFactory() const
{
    return m_uiFactory;
}

void GraphicsScene::addConnectableItem(ItemType type, const QPointF &pos, BaseItem *parentItem)
{
    m_document->undoStack()->beginMacro(tr("Add new state"));
    ConnectableItem *newItem = SceneUtils::createItem(type, pos);

    if (newItem) {
        ScxmlTag *newTag = SceneUtils::createTag(type, m_document);
        ScxmlTag *parentTag = parentItem ? parentItem->tag() : m_document->rootTag();

        newItem->setTag(newTag);
        newItem->setParentItem(parentItem);
        if (!parentItem)
            addItem(newItem);

        newItem->updateAttributes();
        newItem->updateEditorInfo();
        newItem->updateUIProperties();

        if (parentItem)
            parentItem->updateUIProperties();

        m_document->addTag(parentTag, newTag);
        unselectAll();
        newItem->setSelected(true);
    }
    m_document->undoStack()->endMacro();
}

void GraphicsScene::keyPressEvent(QKeyEvent *event)
{
    QGraphicsItem *focusItem = this->focusItem();
    if (!focusItem || focusItem->type() != TextType) {
        if (event->key() == Qt::Key_Delete)
            removeSelectedItems();
    }
    QGraphicsScene::keyPressEvent(event);
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *it = itemAt(event->scenePos(), QTransform());
    if (!it || it->type() == LayoutType) {
        if (event->button() == Qt::LeftButton) {
            QGraphicsScene::mousePressEvent(event);
            m_document->setCurrentTag(m_document->rootTag());
            return;
        } else if (m_actionHandler && event->button() == Qt::RightButton) {
            event->accept();
            QMenu menu;
            menu.addAction(m_actionHandler->action(ActionCopy));
            menu.addAction(m_actionHandler->action(ActionPaste));
            menu.addAction(m_actionHandler->action(ActionScreenshot));
            menu.addAction(m_actionHandler->action(ActionExportToImage));
            menu.addSeparator();
            menu.addAction(m_actionHandler->action(ActionZoomIn));
            menu.addAction(m_actionHandler->action(ActionZoomOut));
            menu.addAction(m_actionHandler->action(ActionFitToView));

            if (m_uiFactory) {
                auto actionProvider = static_cast<ActionProvider*>(m_uiFactory->object(Constants::C_UI_FACTORY_OBJECT_ACTIONPROVIDER));
                if (actionProvider) {
                    menu.addSeparator();
                    actionProvider->initStateMenu(m_document->rootTag(), &menu);
                }
            }

            menu.exec(event->screenPos());
            return;
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

BaseItem *GraphicsScene::findItem(const ScxmlTag *tag) const
{
    if (!tag)
        return nullptr;

    foreach (BaseItem *it, m_baseItems) {
        if (it->tag() == tag)
            return it;
    }

    return nullptr;
}

void GraphicsScene::removeItems(const ScxmlTag *tag)
{
    if (tag) {
        // Find right items
        QVector<BaseItem*> items;
        foreach (BaseItem *it, m_baseItems) {
            if (it->tag() == tag)
                items << it;
        }

        // Then delete them
        for (int i = items.count(); i--;) {
            items[i]->setTag(0);
            delete items[i];
        }
    }
}

QPair<bool, bool> GraphicsScene::checkSnapToItem(BaseItem *item, const QPointF &p, QPointF &pp)
{
    if (m_selectedStateCount > 1)
        return QPair<bool, bool>(false, false);

    QGraphicsItem *parentItem = item->parentItem();

    qreal diffX = 8;
    qreal diffXdY = 2000;

    qreal diffY = 8;
    qreal diffYdX = 2000;

    foreach (BaseItem *it, m_baseItems) {
        if (!it->isSelected() && it != item && it->parentItem() == parentItem && it->type() >= InitialStateType) {
            QPointF c = it->sceneCenter();
            qreal dX = qAbs(c.x() - p.x());
            qreal dY = qAbs(c.y() - p.y());

            if (dX < 7 && dY < diffXdY) {
                pp.setX(c.x());
                diffX = dX;
                diffXdY = dY;
                m_lineY->show(c.x(), c.y(), c.x(), p.y());
            }

            if (dY < 7 && dX < diffYdX) {
                pp.setY(c.y());
                diffY = dY;
                diffYdX = dX;
                m_lineX->show(c.x(), c.y(), p.x(), c.y());
            }
        }
    }

    if (qFuzzyCompare(diffX, 8))
        m_lineY->hideLine();
    if (qFuzzyCompare(diffY, 8))
        m_lineX->hideLine();

    return QPair<bool, bool>(diffX < 8, diffY < 8);
}

void GraphicsScene::selectionChanged(bool para)
{
    Q_UNUSED(para)

    int count = 0;
    int baseCount = 0;
    int stateTypeCount = 0;

    foreach (BaseItem *item, m_baseItems) {
        if (item->isSelected()) {
            if (item->type() >= TransitionType)
                baseCount++;
            if (item->type() >= InitialStateType)
                count++;
            if (item->type() >= StateType)
                stateTypeCount++;
        }
    }

    m_selectedStateTypeCount = stateTypeCount;

    if (count != m_selectedStateCount) {
        m_selectedStateCount = count;
        emit selectedStateCountChanged(m_selectedStateCount);
    }

    if (baseCount != m_selectedBaseItemCount) {
        m_selectedBaseItemCount = baseCount;
        emit selectedBaseItemCountChanged(m_selectedBaseItemCount);
    }
}

void GraphicsScene::addWarningItem(WarningItem *item)
{
    if (!m_allWarnings.contains(item)) {
        m_allWarnings << item;
        if (!m_autoLayoutRunning && !m_initializing)
            QMetaObject::invokeMethod(this, "warningVisibilityChanged", Qt::QueuedConnection, Q_ARG(int, 0));
    }
}

void GraphicsScene::removeWarningItem(WarningItem *item)
{
    m_allWarnings.removeAll(item);

    if (!m_autoLayoutRunning && !m_initializing)
        QMetaObject::invokeMethod(this, "warningVisibilityChanged", Qt::QueuedConnection, Q_ARG(int, 0));
}

void GraphicsScene::warningVisibilityChanged(int type, WarningItem *item)
{
    if (!m_autoLayoutRunning && !m_initializing) {
        foreach (WarningItem *it, m_allWarnings) {
            if (it != item && (type == 0 || it->type() == type))
                it->check();
        }
    }
}

ScxmlTag *GraphicsScene::tagByWarning(const ScxmlEditor::OutputPane::Warning *w) const
{
    ScxmlTag *tag = nullptr;
    foreach (WarningItem *it, m_allWarnings) {
        if (it->warning() == w) {
            tag = it->tag();
            break;
        }
    }
    return tag;
}

void GraphicsScene::highlightWarningItem(const ScxmlEditor::OutputPane::Warning *w)
{
    ScxmlTag *tag = tagByWarning(w);

    if (tag)
        highlightItems(QVector<ScxmlTag*>() << tag);
    else
        unhighlightAll();
}

void GraphicsScene::selectWarningItem(const ScxmlEditor::OutputPane::Warning *w)
{
    ScxmlTag *tag = tagByWarning(w);

    if (tag) {
        unselectAll();
        m_document->setCurrentTag(tag);
    }
}

QList<QGraphicsItem*> GraphicsScene::sceneItems(Qt::SortOrder order) const
{
    QList<QGraphicsItem*> children;
    QList<QGraphicsItem*> items = this->items(order);
    for (int i = 0; i < items.count(); ++i) {
        if (!items[i]->parentItem() && items[i]->type() >= InitialStateType)
            children << items[i];
    }

    return children;
}

void GraphicsScene::addChild(BaseItem *item)
{
    if (!m_baseItems.contains(item)) {
        connect(item, &BaseItem::selectedStateChanged, this, &GraphicsScene::selectionChanged);
        connect(item, &BaseItem::openToDifferentView, this, [=](BaseItem *item){
            openStateView(item);
        }, Qt::QueuedConnection);
        m_baseItems << item;
    }
}

void GraphicsScene::removeChild(BaseItem *item)
{
    if (item)
        disconnect(item, 0, this, 0);
    m_baseItems.removeAll(item);

    selectionChanged(false);
}

void GraphicsScene::checkItemsVisibility(double scaleFactor)
{
    foreach (BaseItem *item, m_baseItems) {
        item->checkVisibility(scaleFactor);
    }
}

void GraphicsScene::checkInitialState()
{
    if (m_document) {
        QList<QGraphicsItem*> sceneItems;
        foreach (BaseItem *item, m_baseItems) {
            if (item->type() >= InitialStateType && !item->parentItem())
                sceneItems << item;
        }
        if (m_uiFactory) {
            auto utilsProvider = static_cast<UtilsProvider*>(m_uiFactory->object("utilsProvider"));
            if (utilsProvider)
                utilsProvider->checkInitialState(sceneItems, m_document->rootTag());
        }
    }
}

void GraphicsScene::clearAllTags()
{
    foreach (BaseItem *it, m_baseItems) {
        it->setTag(0);
    }
}

void GraphicsScene::setBlockUpdates(bool block)
{
    foreach (BaseItem *it, m_baseItems) {
        it->setBlockUpdates(block);
    }
}
