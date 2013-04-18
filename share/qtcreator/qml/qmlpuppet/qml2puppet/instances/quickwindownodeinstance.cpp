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

#include "quickwindownodeinstance.h"

#include "qt5nodeinstanceserver.h"

#include <QQmlExpression>
#include <QQuickView>
#include <QQuickItem>

#include <private/qquickitem_p.h>

#include <cmath>

#include <QHash>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

bool QuickWindowNodeInstance::s_createEffectItem = false;

QuickWindowNodeInstance::QuickWindowNodeInstance(QQuickWindow *item)
   : ObjectNodeInstance(item),
     m_hasHeight(false),
     m_hasWidth(false),
     m_hasContent(true),
     m_x(0.0),
     m_y(0.0),
     m_width(0.0),
     m_height(0.0)
{
}

QuickWindowNodeInstance::~QuickWindowNodeInstance()
{
    if (quickItem())
        designerSupport()->derefFromEffectItem(quickItem());
}

bool QuickWindowNodeInstance::hasContent() const
{
    if (m_hasContent)
        return true;

    return childItemsHaveContent(quickItem());
}

QList<ServerNodeInstance> QuickWindowNodeInstance::childItems() const
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

QList<ServerNodeInstance> QuickWindowNodeInstance::childItemsForChild(QQuickItem *childItem) const
{
    QList<ServerNodeInstance> instanceList;

    if (childItem) {
        foreach (QQuickItem *childItem, childItem->childItems())
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

void QuickWindowNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
}


bool QuickWindowNodeInstance::anyItemHasContent(QQuickItem *quickItem)
{
    if (quickItem->flags().testFlag(QQuickItem::ItemHasContents))
        return true;

    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

bool QuickWindowNodeInstance::childItemsHaveContent(QQuickItem *quickItem)
{
    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

QPointF QuickWindowNodeInstance::position() const
{
    return quickItem()->position();
}

QTransform QuickWindowNodeInstance::transform() const
{
     return DesignerSupport::parentTransform(quickItem());
}

QTransform QuickWindowNodeInstance::customTransform() const
{
    return QTransform();
}

QTransform QuickWindowNodeInstance::sceneTransform() const
{
    return DesignerSupport::windowTransform(quickItem());
}

double QuickWindowNodeInstance::rotation() const
{
    return quickItem()->rotation();
}

double QuickWindowNodeInstance::scale() const
{
    return quickItem()->scale();
}

QPointF QuickWindowNodeInstance::transformOriginPoint() const
{
    return quickItem()->transformOriginPoint();
}

double QuickWindowNodeInstance::zValue() const
{
    return quickItem()->z();
}

double QuickWindowNodeInstance::opacity() const
{
    return quickItem()->opacity();
}

QObject *QuickWindowNodeInstance::parent() const
{
    return 0;
}

bool QuickWindowNodeInstance::equalQuickItem(QQuickItem *item) const
{
    return item == quickItem();
}

void QuickWindowNodeInstance::updateDirtyNodeRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem))
            updateDirtyNodeRecursive(childItem);
    }

    DesignerSupport::updateDirtyNode(parentItem);
}

QImage QuickWindowNodeInstance::renderImage() const
{
    updateDirtyNodeRecursive(quickItem());

    QRectF boundingRect = boundingRectWithStepChilds(quickItem());

    QImage renderImage = designerSupport()->renderImageForItem(quickItem(), boundingRect, boundingRect.size().toSize());

    renderImage = renderImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    return renderImage;
}

QImage QuickWindowNodeInstance::renderPreviewImage(const QSize &previewImageSize) const
{
    QRectF previewItemBoundingRect = boundingRect();

    if (previewItemBoundingRect.isValid() && quickItem())
        return designerSupport()->renderImageForItem(quickItem(), previewItemBoundingRect, previewImageSize);

    return QImage();
}

bool QuickWindowNodeInstance::isMovable() const
{
    return false;
}

QuickWindowNodeInstance::Pointer QuickWindowNodeInstance::create(QObject *object)
{
    QQuickWindow *quickWindow = qobject_cast<QQuickWindow*>(object);

    Q_ASSERT(quickWindow);

    Pointer instance(new QuickWindowNodeInstance(quickWindow));

    instance->setHasContent(anyItemHasContent(quickWindow->contentItem()));
    quickWindow->contentItem()->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(quickWindow->contentItem())->classBegin();

    instance->populateResetHashes();

    QQuickItemPrivate *privateItem = static_cast<QQuickItemPrivate*>(QObjectPrivate::get(quickWindow->contentItem()));

    if (privateItem->window) {
        qDebug() << "removing from window";
        if (!privateItem->parentItem)
            QQuickWindowPrivate::get(privateItem->window)->parentlessItems.remove(quickWindow->contentItem());
        privateItem->derefWindow();
        privateItem->window = 0;
    }

    return instance;
}

void QuickWindowNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    if (instanceId() == 0) {
        DesignerSupport::setRootItem(nodeInstanceServer()->quickView(), quickItem());
    } else {
        quickItem()->setParentItem(qobject_cast<QQuickItem*>(nodeInstanceServer()->quickView()->rootObject()));
    }

    if (s_createEffectItem || instanceId() == 0)
        designerSupport()->refFromEffectItem(quickItem());

    ObjectNodeInstance::initialize(objectNodeInstance);
    quickItem()->update();
}

bool QuickWindowNodeInstance::isQuickItem() const
{
    return true;
}

QSizeF QuickWindowNodeInstance::size() const
{
    double width;

    if (DesignerSupport::isValidWidth(quickItem())) {
        width = quickItem()->width();
    } else {
        width = quickItem()->implicitWidth();
    }

    double height;

    if (DesignerSupport::isValidHeight(quickItem())) {
        height = quickItem()->height();
    } else {
        height = quickItem()->implicitHeight();
    }


    return QSizeF(width, height);
}

static inline bool isRectangleSane(const QRectF &rect)
{
    return rect.isValid() && (rect.width() < 10000) && (rect.height() < 10000);
}

QRectF QuickWindowNodeInstance::boundingRectWithStepChilds(QQuickItem *parentItem) const
{
    QRectF boundingRect = parentItem->boundingRect();

    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem)) {
            QRectF transformedRect = childItem->mapRectToItem(parentItem, boundingRectWithStepChilds(childItem));
            if (isRectangleSane(transformedRect))
                boundingRect = boundingRect.united(transformedRect);
        }
    }

    return boundingRect;
}

QRectF QuickWindowNodeInstance::boundingRect() const
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

void QuickWindowNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
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

    quickItem()->update();
}

void QuickWindowNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (name == "state")
        return; // states are only set by us

    ObjectNodeInstance::setPropertyBinding(name, expression);

    quickItem()->update();
}

QVariant QuickWindowNodeInstance::property(const PropertyName &name) const
{
    if (name == "visible")
        return quickItem()->isVisible();
    return ObjectNodeInstance::property(name);
}

void QuickWindowNodeInstance::resetHorizontal()
 {
    setPropertyVariant("x", m_x);
    if (m_width > 0.0) {
        setPropertyVariant("width", m_width);
    } else {
        setPropertyVariant("width", quickItem()->implicitWidth());
    }
}

void QuickWindowNodeInstance::resetVertical()
 {
    setPropertyVariant("y", m_y);
    if (m_height > 0.0) {
        setPropertyVariant("height", m_height);
    } else {
        setPropertyVariant("height", quickItem()->implicitWidth());
    }
}

static void doComponentCompleteRecursive(QQuickItem *item)
{
    if (item) {
        if (DesignerSupport::isComponentComplete(item))
            return;

        foreach (QQuickItem *childItem, item->childItems())
            doComponentCompleteRecursive(childItem);

        static_cast<QQmlParserStatus*>(item)->componentComplete();
    }
}

void QuickWindowNodeInstance::doComponentComplete()
{
    doComponentCompleteRecursive(quickItem());

    quickItem()->update();
}

bool QuickWindowNodeInstance::isResizable() const
{
    return false;
}

int QuickWindowNodeInstance::penWidth() const
{
    return DesignerSupport::borderWidth(quickItem());
}

void QuickWindowNodeInstance::resetProperty(const PropertyName &name)
{
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

    DesignerSupport::resetAnchor(quickItem(), name);

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

    quickItem()->update();

    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

void QuickWindowNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    DesignerSupport::updateDirtyNode(quickItem());
}

static bool isValidAnchorName(const PropertyName &name)
{
    static PropertyNameList anchorNameList(PropertyNameList() << "anchors.top"
                                                    << "anchors.left"
                                                    << "anchors.right"
                                                    << "anchors.bottom"
                                                    << "anchors.verticalCenter"
                                                    << "anchors.horizontalCenter"
                                                    << "anchors.fill"
                                                    << "anchors.centerIn"
                                                    << "anchors.baseline");

    return anchorNameList.contains(name);
}

bool QuickWindowNodeInstance::hasAnchor(const PropertyName &name) const
{
    return DesignerSupport::hasAnchor(quickItem(), name);
}

QPair<PropertyName, ServerNodeInstance> QuickWindowNodeInstance::anchor(const PropertyName &name) const
{
    if (!isValidAnchorName(name) || !DesignerSupport::hasAnchor(quickItem(), name))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair = DesignerSupport::anchorLineTarget(quickItem(), name, context());

    QObject *targetObject = nameObjectPair.second;
    PropertyName targetName = nameObjectPair.first.toUtf8();

    if (targetObject && nodeInstanceServer()->hasInstanceForObject(targetObject)) {
        return qMakePair(targetName, nodeInstanceServer()->instanceForObject(targetObject));
    } else {
        return ObjectNodeInstance::anchor(name);
    }
}

QList<ServerNodeInstance> QuickWindowNodeInstance::stateInstances() const
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

bool QuickWindowNodeInstance::isAnchoredBySibling() const
{
    return false;
}

bool QuickWindowNodeInstance::isAnchoredByChildren() const
{
    if (DesignerSupport::areChildrenAnchoredTo(quickItem(), quickItem())) // search in children for a anchor to this item
        return true;

    return false;
}

QQuickItem *QuickWindowNodeInstance::quickItem() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QQuickWindow*>(object()));
    return static_cast<QQuickWindow*>(object())->contentItem();
}

DesignerSupport *QuickWindowNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *QuickWindowNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}

void QuickWindowNodeInstance::createEffectItem(bool createEffectItem)
{
    s_createEffectItem = createEffectItem;
}

void QuickWindowNodeInstance::updateDirtyNodeRecursive()
{
    foreach (QQuickItem *childItem, quickItem()->childItems())
            updateDirtyNodeRecursive(childItem);

    DesignerSupport::updateDirtyNode(quickItem());
}

} // namespace Internal
} // namespace QmlDesigner

