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

#include "quickitemnodeinstance.h"

#include "qt5nodeinstanceserver.h"

#include <QQmlExpression>
#include <QQuickView>
#include <cmath>

#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>

#include <QHash>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

bool QuickItemNodeInstance::s_createEffectItem = false;

QuickItemNodeInstance::QuickItemNodeInstance(QQuickItem *item)
   : ObjectNodeInstance(item),
     m_hasHeight(false),
     m_hasWidth(false),
     m_isResizable(true),
     m_hasContent(true),
     m_isMovable(true),
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

bool QuickItemNodeInstance::hasContent() const
{
    if (m_hasContent)
        return true;

    return childItemsHaveContent(quickItem());
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

QList<ServerNodeInstance> QuickItemNodeInstance::childItemsForChild(QQuickItem *childItem) const
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

void QuickItemNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
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

QPointF QuickItemNodeInstance::position() const
{
    return quickItem()->position();
}

static QTransform transformForItem(QQuickItem *item, NodeInstanceServer *nodeInstanceServer)
{
    QTransform toParentTransform = DesignerSupport::parentTransform(item);
    if (item->parentItem() && !nodeInstanceServer->hasInstanceForObject(item->parentItem()))
        return transformForItem(item->parentItem(), nodeInstanceServer) * toParentTransform;

    return toParentTransform;
}

QTransform QuickItemNodeInstance::transform() const
{
    return transformForItem(quickItem(), nodeInstanceServer());
}

QTransform QuickItemNodeInstance::customTransform() const
{
    return QTransform();
}

QTransform QuickItemNodeInstance::sceneTransform() const
{
    return DesignerSupport::windowTransform(quickItem());
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

double QuickItemNodeInstance::opacity() const
{
    return quickItem()->opacity();
}

QObject *QuickItemNodeInstance::parent() const
{
    if (!quickItem() || !quickItem()->parentItem())
        return 0;

    return quickItem()->parentItem();
}

bool QuickItemNodeInstance::equalQuickItem(QQuickItem *item) const
{
    return item == quickItem();
}

void QuickItemNodeInstance::updateDirtyNodeRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem))
            updateDirtyNodeRecursive(childItem);
    }

    DesignerSupport::updateDirtyNode(parentItem);
}

QImage QuickItemNodeInstance::renderImage() const
{
    updateDirtyNodeRecursive(quickItem());

    QRectF boundingRect = boundingRectWithStepChilds(quickItem());

    QImage renderImage = designerSupport()->renderImageForItem(quickItem(), boundingRect, boundingRect.size().toSize());

    renderImage = renderImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    return renderImage;
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

static void disableTextCursor(QQuickItem *item)
{
    foreach (QQuickItem *childItem, item->childItems())
        disableTextCursor(childItem);

    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(item);
    if (textInput)
        textInput->setCursorVisible(false);

    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(item);
    if (textEdit)
        textEdit->setCursorVisible(false);
}

void QuickItemNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    disableTextCursor(quickItem());

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

bool QuickItemNodeInstance::isQuickItem() const
{
    return true;
}

QSizeF QuickItemNodeInstance::size() const
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

QRectF QuickItemNodeInstance::boundingRectWithStepChilds(QQuickItem *parentItem) const
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

void QuickItemNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
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

    refresh();

    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

void QuickItemNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "state")
        return; // states are only set by us

    ObjectNodeInstance::setPropertyBinding(name, expression);

    quickItem()->update();

    refresh();

    if (isInPositioner())
        parentInstance()->refreshPositioner();
}

QVariant QuickItemNodeInstance::property(const QString &name) const
{
    return ObjectNodeInstance::property(name);
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
        setPropertyVariant("height", quickItem()->implicitWidth());
    }
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

void doComponentCompleteRecursive(QQuickItem *item)
{
    if (item) {
        if (DesignerSupport::isComponentComplete(item))
            return;

        foreach (QQuickItem *childItem, item->childItems())
            doComponentCompleteRecursive(childItem);

        static_cast<QQmlParserStatus*>(item)->componentComplete();
    }
}

void QuickItemNodeInstance::doComponentComplete()
{
    doComponentCompleteRecursive(quickItem());

    quickItem()->update();
}

bool QuickItemNodeInstance::isResizable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isResizable && quickItem() && quickItem()->parentItem();
}

void QuickItemNodeInstance::setResizable(bool resizeable)
{
    m_isResizable = resizeable;
}

int QuickItemNodeInstance::penWidth() const
{
    return DesignerSupport::borderWidth(quickItem());
}

void QuickItemNodeInstance::resetProperty(const QString &name)
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

void QuickItemNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty)
{
    if (oldParentInstance && oldParentInstance->isPositioner()) {
        setInPositioner(false);
        setMovable(true);
    }

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (newParentInstance && newParentInstance->isPositioner()) {
        setInPositioner(true);
        setMovable(false);
    }

    if (oldParentInstance && oldParentInstance->isPositioner() && !(newParentInstance && newParentInstance->isPositioner())) {
        if (!hasBindingForProperty("x"))
            setPropertyVariant("x", m_x);

        if (!hasBindingForProperty("y"))
            setPropertyVariant("y", m_y);
    }

    refresh();
    DesignerSupport::updateDirtyNode(quickItem());

    if (parentInstance() && isInPositioner())
        parentInstance()->refreshPositioner();
}

static bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << "anchors.top"
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

bool QuickItemNodeInstance::hasAnchor(const QString &name) const
{
    return DesignerSupport::hasAnchor(quickItem(), name);
}

QPair<QString, ServerNodeInstance> QuickItemNodeInstance::anchor(const QString &name) const
{
    if (!isValidAnchorName(name) || !DesignerSupport::hasAnchor(quickItem(), name))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair = DesignerSupport::anchorLineTarget(quickItem(), name, context());

    QObject *targetObject = nameObjectPair.second;
    QString targetName = nameObjectPair.first;

    if (targetObject && nodeInstanceServer()->hasInstanceForObject(targetObject)) {
        return qMakePair(targetName, nodeInstanceServer()->instanceForObject(targetObject));
    } else {
        return ObjectNodeInstance::anchor(name);
    }
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

bool QuickItemNodeInstance::isAnchoredByChildren() const
{
    if (DesignerSupport::areChildrenAnchoredTo(quickItem(), quickItem())) // search in children for a anchor to this item
        return true;

    return false;
}

QQuickItem *QuickItemNodeInstance::quickItem() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QQuickItem*>(object()));
    return static_cast<QQuickItem*>(object());
}

DesignerSupport *QuickItemNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *QuickItemNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}

void QuickItemNodeInstance::createEffectItem(bool createEffectItem)
{
    s_createEffectItem = createEffectItem;
}

} // namespace Internal
} // namespace QmlDesigner

