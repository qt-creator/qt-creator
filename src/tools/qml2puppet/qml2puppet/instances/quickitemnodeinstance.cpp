// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quickitemnodeinstance.h"
#include "qt5nodeinstanceserver.h"

#include <qmlprivategate.h>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>

#include <private/qquickdesignersupport_p.h>

#include <QQmlProperty>
#include <QQmlExpression>
#include <QQuickView>
#include <cmath>

#include <QHash>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

bool QuickItemNodeInstance::s_createEffectItem = false;
bool QuickItemNodeInstance::s_unifiedRenderPath = false;

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

    QTransform toParentTransform = QQuickDesignerSupport::parentTransform(item);
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {

        return transformForItem(item->parentItem(), nodeInstanceServer) * toParentTransform;
    }

    return toParentTransform;
}

QTransform QuickItemNodeInstance::transform() const
{   if (quickItem()->parentItem())
        return QQuickDesignerSupport::parentTransform(quickItem());

    return QTransform();
}


QObject *QuickItemNodeInstance::parent() const
{
    if (!quickItem() || !quickItem()->parentItem())
        return nullptr;

    return quickItem()->parentItem();
}

QList<ServerNodeInstance> QuickItemNodeInstance::childItems() const
{
    QList<ServerNodeInstance> instanceList;

    const QList<QQuickItem *> childItems = quickItem()->childItems();
    for (QQuickItem *childItem : childItems)
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

void QuickItemNodeInstance::enableUnifiedRenderPath(bool unifiedRenderPath)
{
    s_unifiedRenderPath = unifiedRenderPath;
}

void QuickItemNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                       InstanceContainer::NodeFlags flags)
{

    if (instanceId() == 0)
        nodeInstanceServer()->setRootItem(quickItem());
    else
        quickItem()->setParentItem(nodeInstanceServer()->rootItem());

    ObjectNodeInstance::initialize(objectNodeInstance, flags);
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

    QmlPrivateGate::disableTextCursor(quickItem());

    QmlPrivateGate::emitComponentComplete(quickItem());

    QQmlProperty contentItemProperty(quickItem(), "contentItem", engine());
    if (contentItemProperty.isValid())
        m_contentItem = contentItemProperty.read().value<QQuickItem*>();

    quickItem()->update();
}

static QList<QQuickItem *> allChildItemsRecursive(QQuickItem *parentItem)
{
     QList<QQuickItem *> itemList;

     itemList.append(parentItem->childItems());

     const QList<QQuickItem *> childItems = parentItem->childItems();
     for (QQuickItem *childItem : childItems)
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

QStringList QuickItemNodeInstance::allStates() const
{
    QStringList list;

    QList<QObject*> stateList = QQuickDesignerSupport::statesForItem(quickItem());
    for (QObject *state : stateList) {
        QQmlProperty property(state, "name");
        if (property.isValid())
            list.append(property.read().toString());
    }

    return list;
}

void QuickItemNodeInstance::updateDirtyNode([[maybe_unused]] QQuickItem *item)
{
}

bool QuickItemNodeInstance::unifiedRenderPath()
{
    return s_unifiedRenderPath;
}

void QuickItemNodeInstance::setHiddenInEditor(bool hide)
{
    ObjectNodeInstance::setHiddenInEditor(hide);
    if (s_unifiedRenderPath && !nodeInstanceServer()->isInformationServer()) {
        QQmlProperty property(object(), "visible", context());

        if (!property.isValid())
            return;

        bool visible = property.read().toBool();

        if (hide && visible) {
            setPropertyVariant("visible", false);
            m_hidden = true;
        } else if (!hide && !visible && m_hidden) {
            setPropertyVariant("visible", true);
            m_hidden = false;
        }
    }
}

QRectF QuickItemNodeInstance::contentItemBoundingBox() const
{
    if (contentItem()) {
        QTransform contentItemTransform = QQuickDesignerSupport::parentTransform(contentItem());
        return contentItemTransform.mapRect(contentItem()->boundingRect());
    }

    return QRectF();
}

static bool layerEnabledAndEffect(QQuickItem *item)
{
    QQuickItemPrivate *pItem = QQuickItemPrivate::get(item);

    if (pItem && pItem->layer() && pItem->layer()->enabled() && pItem->layer()->effect())
        return true;

    return false;
}

QRectF QuickItemNodeInstance::boundingRect() const
{
    if (quickItem()) {
        if (quickItem()->clip()) {
            return quickItem()->boundingRect();
        } else if (layerEnabledAndEffect(quickItem())) {
            return ServerNodeInstance::effectAdjustedBoundingRect(quickItem());
        } else {
            QSize maximumSize(4000, 4000);
            auto isValidSize = [maximumSize] (const QRectF& rect) {
                QSize size = rect.size().toSize();
                return size.width() * size.height() <= maximumSize.width() * maximumSize.height();
            };

            QRectF rect = boundingRectWithStepChilds(quickItem());
            if (isValidSize(rect))
                return rect;
            else if (rect = quickItem()->boundingRect(); isValidSize(rect))
                return rect;
            else
                return QRectF(QPointF(0.0, 0.0), maximumSize);
        }
    }

    return QRectF();
}

static QTransform contentTransformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    QTransform contentTransform;
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem())) {
        contentTransform = QQuickDesignerSupport::parentTransform(item->parentItem());
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
    return QQuickDesignerSupport::windowTransform(quickItem());
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

    if (QQuickDesignerSupport::isValidHeight(quickItem())) { // isValidHeight is QQuickItemPrivate::get(item)->widthValid
        width = quickItem()->width();
    } else {
        width = quickItem()->implicitWidth();
    }

    double height;

    if (QQuickDesignerSupport::isValidWidth(quickItem())) { // isValidWidth is QQuickItemPrivate::get(item)->heightValid
        height = quickItem()->height();
    } else {
        height = quickItem()->implicitHeight();
    }


    return QSizeF(width, height);
}

static QTransform contentItemTransformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    QTransform toParentTransform = QQuickDesignerSupport::parentTransform(item);
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
    return QQuickDesignerSupport::borderWidth(quickItem());
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
    if (s_unifiedRenderPath && !isRootNodeInstance())
        return {};

    updateDirtyNodesRecursive(quickItem());

    QRectF renderBoundingRect = boundingRect();
    QImage renderImage;

    if (s_unifiedRenderPath) {
        renderImage = nodeInstanceServer()->grabWindow();
        renderImage = renderImage.copy(renderBoundingRect.toRect());
        /* When grabbing an offscren window the device pixel ratio is 1 */
        renderImage.setDevicePixelRatio(1);
    } else {
        renderImage = nodeInstanceServer()->grabItem(quickItem());
    }

    return renderImage;
}

QImage QuickItemNodeInstance::renderPreviewImage(const QSize &previewImageSize) const
{
    QRectF previewItemBoundingRect = boundingRect();

    if (previewItemBoundingRect.isValid() && quickItem()) {
        static double devicePixelRatio = qgetenv("FORMEDITOR_DEVICE_PIXEL_RATIO").toDouble();
        const QSize size = previewImageSize * devicePixelRatio;
        if (quickItem()->isVisible()) {
            QImage image;
            image = nodeInstanceServer()->grabWindow();
            image = image.copy(previewItemBoundingRect.toRect());
            image = image.scaledToWidth(size.width());
            return image;
        } else {
            QImage transparentImage(size, QImage::Format_ARGB32_Premultiplied);
            transparentImage.fill(Qt::transparent);
            return transparentImage;
        }
    }

    return QImage();
}

QSharedPointer<QQuickItemGrabResult> QuickItemNodeInstance::createGrabResult() const
{
    return quickItem()->grabToImage(size().toSize());
}

void QuickItemNodeInstance::updateAllDirtyNodesRecursive()
{
    updateAllDirtyNodesRecursive(quickItem());
}

bool QuickItemNodeInstance::isQuickItem() const
{
    return true;
}

bool QuickItemNodeInstance::isRenderable() const
{
    return quickItem() && (!s_unifiedRenderPath || isRootNodeInstance());
}

QList<ServerNodeInstance> QuickItemNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
    const QList<QObject*> stateList = QQuickDesignerSupport::statesForItem(quickItem());
    for (QObject *state : stateList)
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

QQuickDesignerSupport *QuickItemNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *QuickItemNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}

void QuickItemNodeInstance::updateDirtyNodesRecursive(QQuickItem *parentItem) const
{
    const QList<QQuickItem *> childItems = parentItem->childItems();
    for (QQuickItem *childItem : childItems) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem))
            updateDirtyNodesRecursive(childItem);
    }

    QmlPrivateGate::disableNativeTextRendering(parentItem);
}

void QuickItemNodeInstance::updateAllDirtyNodesRecursive(QQuickItem *parentItem) const
{
    const QList<QQuickItem *> children = parentItem->childItems();
    for (QQuickItem *childItem : children)
        updateAllDirtyNodesRecursive(childItem);

    updateDirtyNode(parentItem);
}

void QuickItemNodeInstance::setAllNodesDirtyRecursive([[maybe_unused]] QQuickItem *parentItem) const
{
    const QList<QQuickItem *> children = parentItem->childItems();
    for (QQuickItem *childItem : children)
        setAllNodesDirtyRecursive(childItem);
    QQuickDesignerSupport::addDirty(parentItem, QQuickDesignerSupport::Content);
}

static inline bool isRectangleSane(const QRectF &rect)
{
    return rect.isValid() && (rect.width() < 10000) && (rect.height() < 10000);
}

static bool isEffectItem([[maybe_unused]] QQuickItem *item)
{
    if (qobject_cast<QQuickShaderEffectSource *>(item))
        return true;

    const auto propName = "source";

    QQmlProperty prop(item, QString::fromLatin1(propName));
    if (!prop.isValid())
        return false;

    QQuickShaderEffectSource *source = prop.read().value<QQuickShaderEffectSource *>();

    if (source && source->sourceItem()) {
        QQuickItemPrivate *pItem = QQuickItemPrivate::get(source->sourceItem());

        if (pItem && pItem->layer() && pItem->layer()->enabled() && pItem->layer()->effect())
            return true;
    }

    return false;
}

QRectF QuickItemNodeInstance::boundingRectWithStepChilds(QQuickItem *parentItem) const
{
    QRectF boundingRect = parentItem->boundingRect();

    boundingRect = boundingRect.united(QRectF(QPointF(0, 0), size()));

    for (QQuickItem *childItem : parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem) && !isEffectItem(childItem)) {
            QRectF transformedRect = childItem->mapRectToItem(parentItem,
                                                              boundingRectWithStepChilds(childItem));
            if (isRectangleSane(transformedRect))
                boundingRect = boundingRect.united(transformedRect);
        }
    }

    if (boundingRect.isEmpty())
        QRectF{0, 0, 640, 480};

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
        const QList<QQuickItem *> childItems = item->childItems();
        for (QQuickItem *childItem : childItems)
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

    const QList<QQuickItem *> childItems = quickItem->childItems();
    for (QQuickItem *childItem : childItems) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

bool QuickItemNodeInstance::childItemsHaveContent(QQuickItem *quickItem)
{
    const QList<QQuickItem *> childItems = quickItem->childItems();
    for (QQuickItem *childItem : childItems) {
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

    markRepeaterParentDirty();

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (!newParentInstance)
        quickItem()->setParentItem(nullptr);

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

        updateDirtyNode(quickItem());

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

    if (name == "state" && isRootNodeInstance())
        return; // states on the root item are only set by us

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

    if (name == "layer.enabled" || name == "layer.effect")
        setAllNodesDirtyRecursive(quickItem());

    markRepeaterParentDirty();

    ObjectNodeInstance::setPropertyVariant(name, value);

    refresh();

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

void QuickItemNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    static QList<PropertyName> anchorsTargets = {"anchors.top",
                                                 "anchors.bottom",
                                                 "anchors.left",
                                                 "anchors.right",
                                                 "anchors.horizontalCenter",
                                                 "anchors.verticalCenter",
                                                 "anchors.fill",
                                                 "anchors.centerIn",
                                                 "anchors.baseline"};
    if (ignoredProperties().contains(name))
        return;

    if (name == "state" && isRootNodeInstance())
        return; // states on the root item are only set by us

    if (name.startsWith("anchors.") && isRootNodeInstance())
        return;

    markRepeaterParentDirty();

    if (anchorsTargets.contains(name)) {
        //When resolving anchor targets we have to provide the root context the ids are defined in.
        QmlPrivateGate::setPropertyBinding(object(),
                                           context()->engine()->rootContext(),
                                           name,
                                           expression);
    } else {
        ObjectNodeInstance::setPropertyBinding(name, expression);
    }

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

    if (name == "layer.enabled" || name == "layer.effect")
        setAllNodesDirtyRecursive(quickItem());

    QQuickDesignerSupport::resetAnchor(quickItem(), QString::fromUtf8(name));

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

    markRepeaterParentDirty();

    ObjectNodeInstance::resetProperty(name);

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

bool QuickItemNodeInstance::isAnchoredByChildren() const
{
    return QQuickDesignerSupport::areChildrenAnchoredTo(quickItem(), quickItem());
}

bool QuickItemNodeInstance::hasAnchor(const PropertyName &name) const
{
    return QQuickDesignerSupport::hasAnchor(quickItem(), QString::fromUtf8(name));
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
    if (!isValidAnchorName(name) || !QQuickDesignerSupport::hasAnchor(quickItem(), QString::fromUtf8(name)))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair =
            QQuickDesignerSupport::anchorLineTarget(quickItem(), QString::fromUtf8(name), context());

    QObject *targetObject = nameObjectPair.second;
    PropertyName targetName = nameObjectPair.first.toUtf8();

    while (targetObject) {
        if (nodeInstanceServer()->hasInstanceForObject(targetObject))
            return {targetName, nodeInstanceServer()->instanceForObject(targetObject)};
        else
            targetObject = parentObject(targetObject);
    }

    return ObjectNodeInstance::anchor(name);
}

bool QuickItemNodeInstance::isAnchoredBySibling() const
{
    if (quickItem()->parentItem()) {
        const QList<QQuickItem *> childItems = quickItem()->parentItem()->childItems();
        for (QQuickItem *siblingItem : childItems) { // search in siblings for a anchor to this item
            if (siblingItem) {
                if (QQuickDesignerSupport::isAnchoredTo(siblingItem, quickItem()))
                    return true;
            }
        }
    }

    return false;
}



QQuickItem *QuickItemNodeInstance::quickItem() const
{
    if (object() == nullptr)
        return nullptr;

    return static_cast<QQuickItem*>(object());
}

void QuickItemNodeInstance::markRepeaterParentDirty() const
{
    const qint32 id = instanceId();
    if (id <= 0 && !isValid())
        return;

    QQuickItem *item = quickItem();
    if (!item)
        return;

    QQuickItem *parentItem = item->parentItem();
    if (!parentItem)
        return;

    // If a Repeater instance was changed in any way, the parent must be marked dirty to rerender it
    const QByteArray type("QQuickRepeater");
    if (ServerNodeInstance::isSubclassOf(item, type))
        QQuickDesignerSupport::addDirty(parentItem, QQuickDesignerSupport::Content);

    // Repeater's parent must also be dirtied when a child of a repeater was changed
    if (ServerNodeInstance::isSubclassOf(parentItem, type)) {
        QQuickItem *parentsParent = parentItem->parentItem();
        if (parentsParent)
            QQuickDesignerSupport::addDirty(parentsParent, QQuickDesignerSupport::Content);
    }
}



} // namespace Internal
} // namespace QmlDesigner

