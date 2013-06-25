#include "graphicalnodeinstance.h"

#include "qt5nodeinstanceserver.h"

#include <QQmlExpression>

#include <cmath>

#include <QQuickView>

#include <private/qquickitem_p.h>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>

#include <designersupport.h>

namespace QmlDesigner {
namespace Internal {

bool GraphicalNodeInstance::s_createEffectItem = false;

GraphicalNodeInstance::GraphicalNodeInstance(QObject *object)
    : ObjectNodeInstance(object),
      m_hasHeight(false),
      m_hasWidth(false),
      m_hasContent(true),
      m_x(0.0),
      m_y(0.0),
      m_width(0.0),
      m_height(0.0)
{
}

void GraphicalNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
}

DesignerSupport *GraphicalNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *GraphicalNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}


bool GraphicalNodeInstance::isGraphical() const
{
    return true;
}

bool GraphicalNodeInstance::anyItemHasContent(QQuickItem *quickItem)
{
    if (quickItem->flags().testFlag(QQuickItem::ItemHasContents))
        return true;

    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

double GraphicalNodeInstance::x() const
{
    return m_x;
}

double GraphicalNodeInstance::y() const
{
    return m_y;
}

QQuickItem *GraphicalNodeInstance::quickItem() const
{
    return 0;
}

bool GraphicalNodeInstance::childItemsHaveContent(QQuickItem *quickItem)
{
    foreach (QQuickItem *childItem, quickItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

bool GraphicalNodeInstance::hasContent() const
{
    if (m_hasContent)
        return true;

    return childItemsHaveContent(quickItem());
}

void GraphicalNodeInstance::createEffectItem(bool createEffectItem)
{
    s_createEffectItem = createEffectItem;
}

void GraphicalNodeInstance::updateAllDirtyNodesRecursive()
{
    updateAllDirtyNodesRecursive(quickItem());
}

GraphicalNodeInstance::~GraphicalNodeInstance()
{
    if (quickItem())
        designerSupport()->derefFromEffectItem(quickItem());
}

void GraphicalNodeInstance::updateDirtyNodesRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem))
            updateDirtyNodesRecursive(childItem);
    }

    DesignerSupport::updateDirtyNode(parentItem);
}

void GraphicalNodeInstance::updateAllDirtyNodesRecursive(QQuickItem *parentItem) const
{
    foreach (QQuickItem *childItem, parentItem->childItems())
            updateAllDirtyNodesRecursive(childItem);

    DesignerSupport::updateDirtyNode(parentItem);
}

QImage GraphicalNodeInstance::renderImage() const
{
    updateDirtyNodesRecursive(quickItem());

    QRectF renderBoundingRect = boundingRect();

    QImage renderImage = designerSupport()->renderImageForItem(quickItem(), renderBoundingRect, renderBoundingRect.size().toSize());

    return renderImage;
}

QImage GraphicalNodeInstance::renderPreviewImage(const QSize &previewImageSize) const
{
    QRectF previewItemBoundingRect = boundingRect();

    if (previewItemBoundingRect.isValid() && quickItem())
        return designerSupport()->renderImageForItem(quickItem(), previewItemBoundingRect, previewImageSize);

    return QImage();
}

void GraphicalNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
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

QPointF GraphicalNodeInstance::position() const
{
    return quickItem()->position();
}

QTransform GraphicalNodeInstance::customTransform() const
{
    return QTransform();
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

QTransform GraphicalNodeInstance::contentTransform() const
{
    return contentTransformForItem(quickItem(), nodeInstanceServer());
}

QTransform GraphicalNodeInstance::sceneTransform() const
{
    return DesignerSupport::windowTransform(quickItem());
}

double GraphicalNodeInstance::rotation() const
{
    return quickItem()->rotation();
}

double GraphicalNodeInstance::scale() const
{
    return quickItem()->scale();
}

QPointF GraphicalNodeInstance::transformOriginPoint() const
{
    return quickItem()->transformOriginPoint();
}

double GraphicalNodeInstance::zValue() const
{
    return quickItem()->z();
}

double GraphicalNodeInstance::opacity() const
{
    return quickItem()->opacity();
}

QSizeF GraphicalNodeInstance::size() const
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

QRectF GraphicalNodeInstance::boundingRectWithStepChilds(QQuickItem *item) const
{
    QRectF boundingRect = item->boundingRect();

    foreach (QQuickItem *childItem, item->childItems()) {
        if (!nodeInstanceServer()->hasInstanceForObject(childItem)) {
            QRectF transformedRect = childItem->mapRectToItem(item, boundingRectWithStepChilds(childItem));
            if (isRectangleSane(transformedRect))
                boundingRect = boundingRect.united(transformedRect);
        }
    }

    return boundingRect;
}

void GraphicalNodeInstance::resetHorizontal()
 {
    setPropertyVariant("x", m_x);
    if (m_width > 0.0) {
        setPropertyVariant("width", m_width);
    } else {
        setPropertyVariant("width", quickItem()->implicitWidth());
    }
}

void GraphicalNodeInstance::resetVertical()
 {
    setPropertyVariant("y", m_y);
    if (m_height > 0.0) {
        setPropertyVariant("height", m_height);
    } else {
        setPropertyVariant("height", quickItem()->implicitWidth());
    }
}

int GraphicalNodeInstance::penWidth() const
{
    return DesignerSupport::borderWidth(quickItem());
}


QList<ServerNodeInstance> GraphicalNodeInstance::childItemsForChild(QQuickItem *childItem) const
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

QList<ServerNodeInstance> GraphicalNodeInstance::childItems() const
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


void GraphicalNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
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

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

void GraphicalNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (name == "state")
        return; // states are only set by us

    ObjectNodeInstance::setPropertyBinding(name, expression);

    quickItem()->update();

    refresh();

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
}

QVariant GraphicalNodeInstance::property(const PropertyName &name) const
{
    if (name == "visible")
        return quickItem()->isVisible();

    return ObjectNodeInstance::property(name);
}

void GraphicalNodeInstance::resetProperty(const PropertyName &name)
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

    if (isInLayoutable())
        parentInstance()->refreshLayoutable();
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

bool GraphicalNodeInstance::hasAnchor(const PropertyName &name) const
{
    return DesignerSupport::hasAnchor(quickItem(), name);
}

QPair<PropertyName, ServerNodeInstance> GraphicalNodeInstance::anchor(const PropertyName &name) const
{
    if (!isValidAnchorName(name) || !DesignerSupport::hasAnchor(quickItem(), name))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair = DesignerSupport::anchorLineTarget(quickItem(), name, context());

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

void GraphicalNodeInstance::doComponentComplete()
{
    doComponentCompleteRecursive(quickItem(), nodeInstanceServer());

    disableTextCursor(quickItem());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
    DesignerSupport::emitComponentCompleteSignalForAttachedProperty(quickItem());
#endif

    quickItem()->update();
}

bool GraphicalNodeInstance::isAnchoredByChildren() const
{
    if (DesignerSupport::areChildrenAnchoredTo(quickItem(), quickItem())) // search in children for a anchor to this item
        return true;

    return false;
}

QRectF GraphicalNodeInstance::boundingRect() const
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

void GraphicalNodeInstance::refresh()
{
    repositioning(quickItem());
}

QList<ServerNodeInstance> GraphicalNodeInstance::stateInstances() const
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

}
} // namespace QmlDesigner
