#include "nodeinstance.h"

#include <QPainter>
#include <modelnode.h>
#include "commondefines.h"

#include <QtDebug>

namespace QmlDesigner {

class ProxyNodeInstanceData
{
public:
    ProxyNodeInstanceData()
        : parentInstanceId(-1),
          penWidth(1),
          isAnchoredBySibling(false),
          isAnchoredByChildren(false),
          hasContent(false),
          isMovable(false),
          isResizable(false),
          isInPositioner(false)
    {}

    qint32 parentInstanceId;
    ModelNode modelNode;
    QRectF boundingRect;
    QPointF position;
    QSizeF size;
    QTransform transform;
    QTransform sceneTransform;
    int penWidth;
    bool isAnchoredBySibling;
    bool isAnchoredByChildren;
    bool hasContent;
    bool isMovable;
    bool isResizable;
    bool isInPositioner;


    QHash<QString, QVariant> propertyValues;
    QHash<QString, bool> hasBindingForProperty;
    QHash<QString, bool> hasAnchors;
    QHash<QString, QString> instanceTypes;

    QImage renderImage;
    QHash<QString, QPair<QString, qint32> > anchors;
};

NodeInstance::NodeInstance()
{
}

NodeInstance::NodeInstance(ProxyNodeInstanceData *dPointer)
    : d(dPointer)
{
}

NodeInstance NodeInstance::create(const ModelNode &node)
{
    ProxyNodeInstanceData *d = new ProxyNodeInstanceData;

    d->modelNode = node;

    return NodeInstance(d);
}

NodeInstance::~NodeInstance()
{
}

NodeInstance::NodeInstance(const NodeInstance &other)
  : d(other.d)
{
}

NodeInstance &NodeInstance::operator=(const NodeInstance &other)
{
    d = other.d;
    return *this;
}

ModelNode NodeInstance::modelNode() const
{
    if (d) {
        return  d->modelNode;
    } else {
        return ModelNode();
    }
}

qint32 NodeInstance::instanceId() const
{
    if (d) {
        return d->modelNode.internalId();
    } else {
        return -1;
    }
}

bool NodeInstance::isValid() const
{
    return instanceId() >= 0 && modelNode().isValid();
}

void NodeInstance::makeInvalid()
{
    if (d)
        d->modelNode = ModelNode();
}

QRectF NodeInstance::boundingRect() const
{
    if (isValid()) {
        return  d->boundingRect;
    } else {
        return QRectF();
    }
}

bool NodeInstance::hasContent() const
{
    if (isValid()) {
        return d->hasContent;
    } else {
        return false;
    }
}

bool NodeInstance::isAnchoredBySibling() const
{
    if (isValid()) {
        return d->isAnchoredBySibling;
    } else {
        return false;
    }
}

bool NodeInstance::isAnchoredByChildren() const
{
    if (isValid()) {
        return d->isAnchoredByChildren;
    } else {
        return false;
    }
}

bool NodeInstance::isMovable() const
{
    if (isValid()) {
        return d->isMovable;
    } else {
        return false;
    }
}

bool NodeInstance::isResizable() const
{
    if (isValid()) {
        return d->isResizable;
    } else {
        return false;
    }
}

QTransform NodeInstance::transform() const
{
    if (isValid()) {
        return d->transform;
    } else {
        return QTransform();
    }
}
QTransform NodeInstance::sceneTransform() const
{
    if (isValid()) {
        return d->sceneTransform;
    } else {
        return QTransform();
    }
}
bool NodeInstance::isInPositioner() const
{
    if (isValid()) {
        return d->isInPositioner;
    } else {
        return false;
    }
}

QPointF NodeInstance::position() const
{
    if (isValid()) {
        return d->position;
    } else {
        return QPointF();
    }
}

QSizeF NodeInstance::size() const
{
    if (isValid()) {
        return d->size;
    } else {
        return QSizeF();
    }
}

int NodeInstance::penWidth() const
{
    if (isValid()) {
        return d->penWidth;
    } else {
        return 1;
    }
}

void NodeInstance::paint(QPainter *painter)
{
    if (isValid() && !d->renderImage.isNull())
        painter->drawImage(boundingRect().topLeft(), d->renderImage);
}

QVariant NodeInstance::property(const QString &name) const
{
    if (isValid())
        return d->propertyValues.value(name);

    return QVariant();
}

bool NodeInstance::hasBindingForProperty(const QString &name) const
{
    if (isValid())
        return d->hasBindingForProperty.value(name, false);

    return false;
}

QString NodeInstance::instanceType(const QString &name) const
{
    if (isValid())
        return d->instanceTypes.value(name);

    return QString();
}

qint32 NodeInstance::parentId() const
{
    if (isValid()) {
        return d->parentInstanceId;
    } else {
        return false;
    }
}

bool NodeInstance::hasAnchor(const QString &name) const
{
    if (isValid())
        return d->hasAnchors.value(name, false);

    return false;
}

QPair<QString, qint32> NodeInstance::anchor(const QString &name) const
{
    if (isValid())
        return d->anchors.value(name, QPair<QString, qint32>(QString(), qint32(-1)));

    return QPair<QString, qint32>(QString(), -1);
}

void NodeInstance::setProperty(const QString &name, const QVariant &value)
{
    d->propertyValues.insert(name, value);
}

void NodeInstance::setRenderImage(const QImage &image)
{
    d->renderImage = image;
}

void NodeInstance::setParentId(qint32 instanceId)
{
    d->parentInstanceId = instanceId;
}

void NodeInstance::setInformation(InformationName name, const QVariant &information, const QVariant &secondInformation, const QVariant &thirdInformation)
{
    switch (name) {
    case Size: d->size = information.toSizeF(); break;
    case BoundingRect: d->boundingRect = information.toRectF(); break;
    case Transform: d->transform = information.value<QTransform>(); break;
    case PenWidth: d->penWidth = information.toInt(); break;
    case Position: d->position = information.toPointF(); break;
    case IsInPositioner: d->isInPositioner = information.toBool(); break;
    case SceneTransform: d->sceneTransform = information.value<QTransform>(); break;
    case IsResizable: d->isResizable = information.toBool(); break;
    case IsMovable: d->isMovable = information.toBool(); break;
    case IsAnchoredByChildren: d->isAnchoredByChildren  = information.toBool(); break;
    case IsAnchoredBySibling: d->isAnchoredBySibling = information.toBool(); break;
    case HasContent: d->hasContent = information.toBool(); break;
    case HasAnchor: d->hasAnchors.insert(information.toString(), secondInformation.toBool());break;
    case Anchor: d->anchors.insert(information.toString(), qMakePair(secondInformation.toString(), thirdInformation.value<qint32>())); break;
    case InstanceTypeForProperty: d->instanceTypes.insert(information.toString(), secondInformation.toString()); break;
    case HasBindingForProperty: d->hasBindingForProperty.insert(information.toString(), secondInformation.toBool()); break;
    case NoName:
    default: break;
    }
}

}
