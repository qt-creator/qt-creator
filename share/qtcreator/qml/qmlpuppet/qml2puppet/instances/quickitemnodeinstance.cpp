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

#include "quickitemnodeinstance.h"
#include "qt5nodeinstanceserver.h"

#include <qmlprivategate.h>

#include <QQmlProperty>
#include <QQmlExpression>
#include <QQuickView>
#include <cmath>

#include <QHash>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

bool QuickItemNodeInstance::s_createEffectItem = false;

QuickItemNodeInstance::QuickItemNodeInstance(QQuickItem *item)
   : ObjectNodeInstance(item),
     m_isResizable(true),
     m_isMovable(true),
     m_hasHeight(false),
     m_hasWidth(false),
     m_hasContent(true),
     m_x(0.0),
     m_y(0.0),
     m_width(0.0),
     m_height(0.0)
{
}

QuickItemNodeInstance::~QuickItemNodeInstance()
{
    if (quickItem())
        designerSupport()->derefFromEffectItem(quickItem());
}

static bool isContentItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{

    return item->parentItem()
            && nodeInstanceServer->hasInstanceForObject(item->parentItem())
            && nodeInstanceServer->instanceForObject(item->parentItem()).internalInstance()->contentItem() == item;
}

static QTransform transformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    if (isContentItem(item, nodeInstanceServer))
        return QTransform();

    QTransform toParentTransform = DesignerSupport::parentTransform(item);
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {

        return transformForItem(item->parentItem(), nodeInstanceServer) * toParentTransform;
    }

    return toParentTransform;
}

QTransform QuickItemNodeInstance::transform() const
{   if (quickItem()->parentItem())
        return DesignerSupport::parentTransform(quickItem());

    return QTransform();
}


QObject *QuickItemNodeInstance::parent() const
{
    if (!quickItem() || !quickItem()->parentItem())
        return 0;

    return quickItem()->parentItem();
}

QList<ServerNodeInstance> QuickItemNodeInstance::childItems() const
{
    QList<ServerNodeInstance> instanceList;

    foreach (QQuickItem *childItem, quickItem()->childItems())
    {
        if (childItem && nodeInstanceServer()->hasInstanceForObject(childItem)) {
            instanceList.append(nodeInstanceServer()->instanceForObject(childItem));
        } else { //there might be an item in between the parent instance
                 //and the child instance.
                 //Popular example is flickable which has a viewport item between
                 //the flickable item and the flickable children
            instanceList.append(childItemsForChild(childItem)); //In such a case we go deeper inside the item and
                                                           //search for child items with instances.
        }
    }

    return instanceList;
}

bool QuickItemNodeInstance::isMovable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isMovable && quickItem() && quickItem()->parentItem();
}

void QuickItemNodeInstance::setMovable(bool movable)
{
    m_isMovable = movable;
}

QuickItemNodeInstance::Pointer QuickItemNodeInstance::create(QObject *object)
{
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);

    Q_ASSERT(quickItem);

    Pointer instance(new QuickItemNodeInstance(quickItem));

    instance->setHasContent(anyItemHasContent(quickItem));
    quickItem->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(quickItem)->classBegin();

    instance->populateResetHashes();

    return instance;
}

void QuickItemNodeInstance::createEffectItem(bool createEffectItem)
{
    s_createEffectItem = createEffectItem;
}

void QuickItemNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
{

    if (instanceId() == 0) {
        DesignerSupport::setRootItem(nodeInstanceServer()->quickView(), quickItem());
    } else {
        quickItem()->setParentItem(qobject_cast<QQuickItem*>(nodeInstanceServer()->quickView()->rootObject()));
    }

    if (quickItem()->window()) {
        if (s_createEffectItem || instanceId() == 0)
            designerSupport()->refFromEffectItem(quickItem());
    }

    ObjectNodeInstance::initialize(objectNodeInstance);
    quickItem()->update();
}

QQuickItem *QuickItemNodeInstance::contentItem() const
{
    return m_contentItem.data();
}

bool QuickItemNodeInstance::hasContent() const
{
    if (m_hasContent)
        return true;

    return childItemsHaveContent(quickItem());
}

void QuickItemNodeInstance::doComponentComplete()
{
    ObjectNodeInstance::doComponentComplete();

    QQmlProperty contentItemProperty(quickItem(), "contentItem", engine());
    if (contentItemProperty.isValid())
        m_contentItem = contentItemProperty.read().value<QQuickItem*>();

    QmlPrivateGate::disableTextCursor(quickItem());

    DesignerSupport::emitComponentCompleteSignalForAttachedProperty(quickItem());

    quickItem()->update();
}

static QList<QQuickItem *> allChildItemsRecursive(QQuickItem *parentItem)
{
     QList<QQuickItem *> itemList;

     itemList.append(parentItem->childItems());

     foreach (QQuickItem *childItem, parentItem->childItems())
         itemList.append(allChildItemsRecursive(childItem));

     return itemList;
}

QList<QQuickItem *> QuickItemNodeInstance::allItemsRecursive() const
{
    QList<QQuickItem *> itemList;


    if (quickItem()) {
        if (quickItem()->parentItem())
           itemList.append(quickItem()->parentItem());

        itemList.append(quickItem());
        itemList.append(allChildItemsRecursive(quickItem()));
    }

    return itemList;
}

QRectF QuickItemNodeInstance::contentItemBoundingBox() const
{
    if (contentItem()) {
        QTransform contentItemTransform = DesignerSupport::parentTransform(contentItem());
        return contentItemTransform.mapRect(contentItem()->boundingRect());
    }

    return QRectF();
}

QRectF QuickItemNodeInstance::boundingRect() const
{
    if (quickItem()) {
        if (quickItem()->clip()) {
            return quickItem()->boundingRect();
        } else {
            return boundingRectWithStepChilds(quickItem());
        }
    }

    return QRectF();
}

static QTransform contentTransformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    QTransform contentTransform;
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {
        contentTransform = DesignerSupport::parentTransform(item->parentItem());
        return contentTransformForItem(item->parentItem(), nodeInstanceServer) * contentTransform;
    }

    return contentTransform;
}

QTransform QuickItemNodeInstance::contentTransform() const
{
    return contentTransformForItem(quickItem(), nodeInstanceServer());
}

QTransform QuickItemNodeInstance::sceneTransform() const
{
    return DesignerSupport::windowTransform(quickItem());
}

double QuickItemNodeInstance::opacity() const
{
    return quickItem()->opacity();
}

double QuickItemNodeInstance::rotation() const
{
    return quickItem()->rotation();
}

double QuickItemNodeInstance::scale() const
{
    return quickItem()->scale();
}

QPointF QuickItemNodeInstance::transformOriginPoint() const
{
    return quickItem()->transformOriginPoint();
}

double QuickItemNodeInstance::zValue() const
{
    return quickItem()->z();
}

QPointF QuickItemNodeInstance::position() const
{
    return quickItem()->position();
}

QSizeF QuickItemNodeInstance::size() const
{
    double width;

    if (DesignerSupport::isValidHeight(quickItem())) { // isValidHeight is QQuickItemPrivate::get(item)->widthValid
        width = quickItem()->width();
    } else {
        width = quickItem()->implicitWidth();
    }

    double height;

    if (DesignerSupport::isValidWidth(quickItem())) { // isValidWidth is QQuickItemPrivate::get(item)->heightValid
        height = quickItem()->height();
    } else {
        height = quickItem()->implicitHeight();
    }


    return QSizeF(width, height);
}

static QTransform contentItemTransformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    QTransform toParentTransform = DesignerSupport::parentTransform(item);
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {

        return transformForItem(item->parentItem(), nodeInstanceServer) * toParentTransform;
    }

    return toParentTransform;
}

QTransform QuickItemNodeInstance::contentItemTransform() const
{
    if (contentItem())
        return contentItemTransformForItem(contentItem(), nodeInstanceServer());

    return QTransform();
}

int QuickItemNodeInstance::penWidth() const
{
    return DesignerSupport::borderWidth(quickItem());
}

double QuickItemNodeInstance::x() const
{
    return m_x;
}

double QuickItemNodeInstance::y() const
{
    return m_y;
}

QImage QuickItemNodeInstance::renderImage() const
{
    updateDirtyNodesRecursive(quickItem());

    QRectF renderBoundingRect = boundingRect();

    QSize size = renderBoundingRect.size().toSize();
    static double devicePixelRatio = qgetenv("FORMEDITOR_DEVICE_PIXEL_RATIO").toDouble();
    size *= devicePixelRatio;

    QImage renderImage = designerSupport()->renderImageForItem(quickItem(), renderBoundingRect, size);

    renderImage.setDevicePixelRatio(devicePixelRatio);

    return renderImage;
}

QImage QuickItemNodeInstance::renderPreviewImage(const QSize &previewImageSize) const
{
    QRectF previewItemBoundingRect = boundingRect();

    if (previewItemBoundingRect.isValid() && quickItem()) {
        if (quickItem()->isVisible()) {
            return designerSupport()->renderImageForItem(quickItem(), previewItemBoundingRect, previewImageSize);
        } else {
            QImage transparentImage(previewImageSize, QImage::Format_ARGB32_Premultiplied);
            transparentImage.fill(Qt::transparent);
            return transparentImage;
        }
    }

    return QImage();
}

void QuickItemNodeInstance::updateAllDirtyNodesRecursive()
{
    updateAllDirtyNodesRecursive(quickItem());
}

bool QuickItemNodeInstance::isQuickItem() const
{
    return true;
}

QList<ServerNodeInstance> QuickItemNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
    QList<QObject*> stateList = DesignerSupport::statesForItem(quickItem());
    foreach (QObject *state, stateList)
    {
        if (state && nodeInstanceServer()->hasInstanceForObject(state))
            instanceList.append(nodeInstanceServer()->instanceForObject(state));
    }

    return instanceList;
}

bool QuickItemNodeInstance::isResizable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isResizable && quickItem() && quickItem()->parentItem();
}

void QuickItemNodeInstance::setResizable(bool resizable)
{
    m_isResizable = resizable;
}

void QuickItemNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
}

DesignerSupport *QuickItemNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *QuickItemNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}

void QuickItemNodeInstance::updateDirtyNodesRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem))
            updateDirtyNodesRecursive(childItem);
    }

    QmlPrivateGate::disableNativeTextRendering(parentItem);
    DesignerSupport::updateDirtyNode(parentItem);
}

void QuickItemNodeInstance::updateAllDirtyNodesRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems())
            updateAllDirtyNodesRecursive(childItem);

    DesignerSupport::updateDirtyNode(parentItem);
}

static inline bool isRectangleSane(const QRectF &rect)
{
    return rect.isValid() && (rect.width() < 10000) && (rect.height() < 10000);
}

QRectF QuickItemNodeInstance::boundingRectWithStepChilds(QQuickItem *parentItem) const
{
    QRectF boundingRect = parentItem->boundingRect();

    boundingRect = boundingRect.united(QRectF(QPointF(0, 0), size()));

    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem)) {
            QRectF transformedRect = childItem->mapRectToItem(parentItem, boundingRectWithStepChilds(childItem));
            if (isRectangleSane(transformedRect))
                boundingRect = boundingRect.united(transformedRect);
        }
    }

    return boundingRect;
}

void QuickItemNodeInstance::resetHorizontal()
{
    setPropertyVariant("x", m_x);
    if (m_width > 0.0) {
        setPropertyVariant("width", m_width);
    } else {
        setPropertyVariant("width", quickItem()->implicitWidth());
    }
}

void QuickItemNodeInstance::resetVertical()
{
    setPropertyVariant("y", m_y);
    if (m_height > 0.0) {
        setPropertyVariant("height", m_height);
    } else {
        setPropertyVariant("height", quickItem()->implicitHeight());
    }
}

QList<ServerNodeInstance> QuickItemNodeInstance::childItemsForChild(QQuickItem *item) const
{
    QList<ServerNodeInstance> instanceList;

    if (item) {
        foreach (QQuickItem *childItem, item->childItems())
        {
            if (childItem && nodeInstanceServer()->hasInstanceForObject(childItem)) {
                instanceList.append(nodeInstanceServer()->instanceForObject(childItem));
            } else {
                instanceList.append(childItemsForChild(childItem));
            }
        }
    }
    return instanceList;
}

static void repositioning(QQuickItem *item)
{
    if (!item)
        return;

//    QQmlBasePositioner *positioner = qobject_cast<QQmlBasePositioner*>(item);
//    if (positioner)
//        positioner->rePositioning();

    if (item->parentItem())
        repositioning(item->parentItem());
}
void QuickItemNodeInstance::refresh()
{
    repositioning(quickItem());
}

bool QuickItemNodeInstance::anyItemHasContent(QQuickItem *quickItem)
{
    if (quickItem->flags().testFlag(QQuickItem::ItemHasContents))
        return true;

    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

bool QuickItemNodeInstance::childItemsHaveContent(QQuickItem *quickItem)
{
    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

static bool instanceIsValidLayoutable(const ObjectNodeInstance::Pointer &instance, const PropertyName &propertyName)
{
    return instance && instance->isLayoutable() && !instance->ignoredProperties().contains(propertyName);
}

void QuickItemNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    if (instanceIsValidLayoutable(oldParentInstance, oldParentProperty)) {
        setInLayoutable(false);
        setMovable(true);
    }

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (instanceIsValidLayoutable(newParentInstance, newParentProperty)) {
        setInLayoutable(true);
        setMovable(false);
    }

    if (instanceIsValidLayoutable(oldParentInstance, oldParentProperty) && !instanceIsValidLayoutable(newParentInstance, newParentProperty)) {
        if (!hasBindingForProperty("x"))
            setPropertyVariant("x", x());

        if (!hasBindingForProperty("y"))
            setPropertyVariant("y", y());
    }

    if (quickItem()->parentItem()) {
        refresh();
        DesignerSupport::updateDirtyNode(quickItem());

        if (instanceIsValidLayoutable(oldParentInstance, oldParentProperty))
            oldParentInstance->refreshLayoutable();

        if (instanceIsValidLayoutable(newParentInstance, newParentProperty))
            newParentInstance->refreshLayoutable();
    }
}

void QuickItemNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    if (ignoredProperties().contains(name))
        return;

    if (name == "state")
        return; // states are only set by us

    if (name == "height") {
        m_height = value.toDouble();
       if (value.isValid())
           m_hasHeight = true;
       else
           m_hasHeight = false;
    }

    if (name == "width") {
       m_width = value.toDouble();
       if (value.isValid())
           m_hasWidth = true;
       else
           m_hasWidth = false;
    }

    if (name == "x")
        m_x = value.toDouble();

    if (name == "y")
        m_y = value.toDouble();

    ObjectNodeInstance::setPropertyVariant(name, value);

    refresh();

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

void QuickItemNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (ignoredProperties().contains(name))
        return;

    if (name == "state")
        return; // states are only set by us

    if (name.startsWith("anchors.") && isRootNodeInstance())
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);

    refresh();

    /* Evaluate properties of the root item in the context of the dummy context if they contain parent.
     * This is done manually because we cannot "overwrite" the parent property
     */

    if (isRootNodeInstance() && expression.contains(QLatin1String("parent."))) {
        QQmlExpression qmlContextExpression(context(), nodeInstanceServer()->dummyContextObject(), expression);
        QVariant value = qmlContextExpression.evaluate();
        setPropertyVariant(name, value);
    }

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

QVariant QuickItemNodeInstance::property(const PropertyName &name) const
{
    if (ignoredProperties().contains(name))
        return QVariant();

    if (name == "visible")
        return quickItem()->isVisible();

    return ObjectNodeInstance::property(name);
}

void QuickItemNodeInstance::resetProperty(const PropertyName &name)
{
    if (ignoredProperties().contains(name))
        return;

    if (name == "height") {
        m_hasHeight = false;
        m_height = 0.0;
    }

    if (name == "width") {
        m_hasWidth = false;
        m_width = 0.0;
    }

    if (name == "x")
        m_x = 0.0;

    if (name == "y")
        m_y = 0.0;

    DesignerSupport::resetAnchor(quickItem(), QString::fromUtf8(name));

    if (name == "anchors.fill") {
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.centerIn") {
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.top") {
        resetVertical();
    } else if (name == "anchors.left") {
        resetHorizontal();
    } else if (name == "anchors.right") {
        resetHorizontal();
    } else if (name == "anchors.bottom") {
        resetVertical();
    } else if (name == "anchors.horizontalCenter") {
        resetHorizontal();
    } else if (name == "anchors.verticalCenter") {
        resetVertical();
    } else if (name == "anchors.baseline") {
        resetVertical();
    }

    ObjectNodeInstance::resetProperty(name);

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

bool QuickItemNodeInstance::isAnchoredByChildren() const
{
    return DesignerSupport::areChildrenAnchoredTo(quickItem(), quickItem());
}

bool QuickItemNodeInstance::hasAnchor(const PropertyName &name) const
{
    return DesignerSupport::hasAnchor(quickItem(), QString::fromUtf8(name));
}

static bool isValidAnchorName(const PropertyName &name)
{
    static PropertyNameList anchorNameList({"anchors.top",
                                            "anchors.left",
                                            "anchors.right",
                                            "anchors.bottom",
                                            "anchors.verticalCenter",
                                            "anchors.horizontalCenter",
                                            "anchors.fill",
                                            "anchors.centerIn",
                                            "anchors.baseline"});

    return anchorNameList.contains(name);
}

QPair<PropertyName, ServerNodeInstance> QuickItemNodeInstance::anchor(const PropertyName &name) const
{
    if (!isValidAnchorName(name) || !DesignerSupport::hasAnchor(quickItem(), QString::fromUtf8(name)))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair =
            DesignerSupport::anchorLineTarget(quickItem(), QString::fromUtf8(name), context());

    QObject *targetObject = nameObjectPair.second;
    PropertyName targetName = nameObjectPair.first.toUtf8();

    while (targetObject) {
        if (nodeInstanceServer()->hasInstanceForObject(targetObject))
            return qMakePair(targetName, nodeInstanceServer()->instanceForObject(targetObject));
        else
            targetObject = parentObject(targetObject);
    }

    return ObjectNodeInstance::anchor(name);
}

bool QuickItemNodeInstance::isAnchoredBySibling() const
{
    if (quickItem()->parentItem()) {
        foreach (QQuickItem *siblingItem, quickItem()->parentItem()->childItems()) { // search in siblings for a anchor to this item
            if (siblingItem) {
                if (DesignerSupport::isAnchoredTo(siblingItem, quickItem()))
                    return true;
            }
        }
    }

    return false;
}



QQuickItem *QuickItemNodeInstance::quickItem() const
{
    if (object() == 0)
        return 0;

    return static_cast<QQuickItem*>(object());
}

} // namespace Internal
} // namespace QmlDesigner

