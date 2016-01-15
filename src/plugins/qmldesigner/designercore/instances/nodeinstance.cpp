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

#include "nodeinstance.h"

#include <QPainter>
#include <modelnode.h>

#include <QDebug>

QT_BEGIN_NAMESPACE
void qt_blurImage(QPainter *painter, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

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
          isInLayoutable(false),
          directUpdates(false)
    {}

    qint32 parentInstanceId;
    ModelNode modelNode;
    QRectF boundingRect;
    QRectF contentItemBoundingRect;
    QPointF position;
    QSizeF size;
    QTransform transform;
    QTransform contentTransform;
    QTransform contentItemTransform;
    QTransform sceneTransform;
    int penWidth;
    bool isAnchoredBySibling;
    bool isAnchoredByChildren;
    bool hasContent;
    bool isMovable;
    bool isResizable;
    bool isInLayoutable;
    bool directUpdates;


    QHash<PropertyName, QVariant> propertyValues;
    QHash<PropertyName, bool> hasBindingForProperty;
    QHash<PropertyName, bool> hasAnchors;
    QHash<PropertyName, TypeName> instanceTypes;

    QPixmap renderPixmap;
    QPixmap blurredRenderPixmap;

    QString errorMessage;

    QHash<PropertyName, QPair<PropertyName, qint32> > anchors;
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
    if (d)
        return d->modelNode;
    else
        return ModelNode();
}

qint32 NodeInstance::instanceId() const
{
    if (d)
        return d->modelNode.internalId();
    else
        return -1;
}

void NodeInstance::setDirectUpdate(bool directUpdates)
{
    if (d)
        d->directUpdates = directUpdates;
}

bool NodeInstance::directUpdates() const
{
    if (d)
        return d->directUpdates && !(d->transform.isRotating() || d->transform.isScaling() || hasAnchors());
    else
        return true;
}

void NodeInstance::setX(double x)
{
    if (d && directUpdates()) {
        double dx = x - d->transform.dx();
        d->transform.translate(dx, 0.0);
    }
}

void NodeInstance::setY(double y)
{
    if (d && directUpdates()) {
        double dy = y - d->transform.dy();
        d->transform.translate(0.0, dy);
    }
}

bool NodeInstance::hasAnchors() const
{
    return hasAnchor("anchors.fill")
            || hasAnchor("anchors.centerIn")
            || hasAnchor("anchors.top")
            || hasAnchor("anchors.left")
            || hasAnchor("anchors.right")
            || hasAnchor("anchors.bottom")
            || hasAnchor("anchors.horizontalCenter")
            || hasAnchor("anchors.verticalCenter")
            || hasAnchor("anchors.baseline");
}

QString NodeInstance::error() const
{
    return d->errorMessage;
}

bool NodeInstance::hasError() const
{
    return !d->errorMessage.isEmpty();
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
    if (isValid())
        return d->boundingRect;
    else
        return QRectF();
}

QRectF NodeInstance::contentItemBoundingRect() const
{
    if (isValid())
        return d->contentItemBoundingRect;
    else
        return QRectF();
}

bool NodeInstance::hasContent() const
{
    if (isValid())
        return d->hasContent;
    else
        return false;
}

bool NodeInstance::isAnchoredBySibling() const
{
    if (isValid())
        return d->isAnchoredBySibling;
    else
        return false;
}

bool NodeInstance::isAnchoredByChildren() const
{
    if (isValid())
        return d->isAnchoredByChildren;
    else
        return false;
}

bool NodeInstance::isMovable() const
{
    if (isValid())
        return d->isMovable;
    else
        return false;
}

bool NodeInstance::isResizable() const
{
    if (isValid())
        return d->isResizable;
    else
        return false;
}

QTransform NodeInstance::transform() const
{
    if (isValid())
        return d->transform;
    else
        return QTransform();
}

QTransform NodeInstance::contentTransform() const
{
    if (isValid())
        return d->contentTransform;
    else
        return QTransform();
}

QTransform NodeInstance::contentItemTransform() const
{
    if (isValid())
        return d->contentItemTransform;
    else
        return QTransform();
}
QTransform NodeInstance::sceneTransform() const
{
    if (isValid())
        return d->sceneTransform;
    else
        return QTransform();
}
bool NodeInstance::isInLayoutable() const
{
    if (isValid())
        return d->isInLayoutable;
    else
        return false;
}

QPointF NodeInstance::position() const
{
    if (isValid())
        return d->position;
    else
        return QPointF();
}

QSizeF NodeInstance::size() const
{
    if (isValid())
        return d->size;
    else
        return QSizeF();
}

int NodeInstance::penWidth() const
{
    if (isValid())
        return d->penWidth;
    else
        return 1;
}

QVariant NodeInstance::property(const PropertyName &name) const
{
    if (isValid())
        return d->propertyValues.value(name);

    return QVariant();
}

bool NodeInstance::hasProperty(const PropertyName &name) const
{
    if (isValid())
        return d->propertyValues.contains(name);

    return false;
}

bool NodeInstance::hasBindingForProperty(const PropertyName &name) const
{
    if (isValid())
        return d->hasBindingForProperty.value(name, false);

    return false;
}

TypeName NodeInstance::instanceType(const PropertyName &name) const
{
    if (isValid())
        return d->instanceTypes.value(name);

    return TypeName();
}

qint32 NodeInstance::parentId() const
{
    if (isValid())
        return d->parentInstanceId;
    else
        return false;
}

bool NodeInstance::hasAnchor(const PropertyName &name) const
{
    if (isValid())
        return d->hasAnchors.value(name, false);

    return false;
}

QPair<PropertyName, qint32> NodeInstance::anchor(const PropertyName &name) const
{
    if (isValid())
        return d->anchors.value(name, QPair<PropertyName, qint32>(PropertyName(), qint32(-1)));

    return QPair<PropertyName, qint32>(PropertyName(), -1);
}

void NodeInstance::setProperty(const PropertyName &name, const QVariant &value)
{
    d->propertyValues.insert(name, value);
}

QPixmap NodeInstance::renderPixmap() const
{
    return d->renderPixmap;
}

QPixmap NodeInstance::blurredRenderPixmap() const
{
    if (d->blurredRenderPixmap.isNull()) {
        d->blurredRenderPixmap = QPixmap(d->renderPixmap.size());
        QPainter blurPainter(&d->blurredRenderPixmap);
        QImage renderImage = d->renderPixmap.toImage();
        qt_blurImage(&blurPainter, renderImage, 8.0, false, false);
    }

    return d->blurredRenderPixmap;
}

void NodeInstance::setRenderPixmap(const QImage &image)
{
    d->renderPixmap = QPixmap::fromImage(image);
    d->blurredRenderPixmap = QPixmap();
}

bool NodeInstance::setError(const QString &errorMessage)
{
    if (d->errorMessage != errorMessage) {
        d->errorMessage = errorMessage;
        return true;
    }
    return false;
}

void NodeInstance::setParentId(qint32 instanceId)
{
    d->parentInstanceId = instanceId;
}

InformationName NodeInstance::setInformationSize(const QSizeF &size)
{
    if (d->size != size) {
        d->size = size;
        return Size;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationBoundingRect(const QRectF &rectangle)
{
    if (d->boundingRect != rectangle) {
        d->boundingRect = rectangle;
        return BoundingRect;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentItemBoundingRect(const QRectF &rectangle)
{
    if (d->contentItemBoundingRect != rectangle) {
        d->contentItemBoundingRect = rectangle;
        return ContentItemBoundingRect;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationTransform(const QTransform &transform)
{
    if (!directUpdates() && d->transform != transform) {
        d->transform = transform;
        return Transform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentTransform(const QTransform &contentTransform)
{
    if (d->contentTransform != contentTransform) {
        d->contentTransform = contentTransform;
        return ContentTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentItemTransform(const QTransform &contentItemTransform)
{
    if (d->contentItemTransform != contentItemTransform) {
        d->contentItemTransform = contentItemTransform;
        return ContentItemTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationPenWith(int penWidth)
{
    if (d->penWidth != penWidth) {
        d->penWidth = penWidth;
        return PenWidth;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationPosition(const QPointF &position)
{
    if (d->position != position) {
        d->position = position;
        return Position;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsInLayoutable(bool isInLayoutable)
{
    if (d->isInLayoutable != isInLayoutable) {
        d->isInLayoutable = isInLayoutable;
        return IsInLayoutable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationSceneTransform(const QTransform &sceneTransform)
{
  if (d->sceneTransform != sceneTransform) {
        d->sceneTransform = sceneTransform;
        if (!directUpdates())
            return SceneTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsResizable(bool isResizable)
{
    if (d->isResizable != isResizable) {
        d->isResizable = isResizable;
        return IsResizable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsMovable(bool isMovable)
{
    if (d->isMovable != isMovable) {
        d->isMovable = isMovable;
        return IsMovable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsAnchoredByChildren(bool isAnchoredByChildren)
{
    if (d->isAnchoredByChildren != isAnchoredByChildren) {
        d->isAnchoredByChildren = isAnchoredByChildren;
        return IsAnchoredByChildren;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsAnchoredBySibling(bool isAnchoredBySibling)
{
    if (d->isAnchoredBySibling != isAnchoredBySibling) {
        d->isAnchoredBySibling = isAnchoredBySibling;
        return IsAnchoredBySibling;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasContent(bool hasContent)
{
    if (d->hasContent != hasContent) {
        d->hasContent = hasContent;
        return HasContent;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasAnchor(const PropertyName &sourceAnchorLine, bool hasAnchor)
{
    if (d->hasAnchors.value(sourceAnchorLine) != hasAnchor) {
        d->hasAnchors.insert(sourceAnchorLine, hasAnchor);
        return HasAnchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationAnchor(const PropertyName &sourceAnchorLine, const PropertyName &targetAnchorLine, qint32 targetInstanceId)
{
    QPair<PropertyName, qint32>  anchorPair = QPair<PropertyName, qint32>(targetAnchorLine, targetInstanceId);
    if (d->anchors.value(sourceAnchorLine) != anchorPair) {
        d->anchors.insert(sourceAnchorLine, anchorPair);
        return Anchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationInstanceTypeForProperty(const PropertyName &property, const TypeName &type)
{
    if (d->instanceTypes.value(property) != type) {
        d->instanceTypes.insert(property, type);
        return InstanceTypeForProperty;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasBindingForProperty(const PropertyName &property, bool hasProperty)
{
    if (d->hasBindingForProperty.value(property) != hasProperty) {
        d->hasBindingForProperty.insert(property, hasProperty);
        return HasBindingForProperty;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformation(InformationName name, const QVariant &information, const QVariant &secondInformation, const QVariant &thirdInformation)
{
    switch (name) {
    case Size: return setInformationSize(information.toSizeF());
    case BoundingRect: return setInformationBoundingRect(information.toRectF());
    case ContentItemBoundingRect: return setInformationContentItemBoundingRect(information.toRectF());
    case Transform: return setInformationTransform(information.value<QTransform>());
    case ContentTransform: return setInformationContentTransform(information.value<QTransform>());
    case ContentItemTransform: return setInformationContentItemTransform(information.value<QTransform>());
    case PenWidth: return setInformationPenWith(information.toInt());
    case Position: return setInformationPosition(information.toPointF());
    case IsInLayoutable: return setInformationIsInLayoutable(information.toBool());
    case SceneTransform: return setInformationSceneTransform(information.value<QTransform>());
    case IsResizable: return setInformationIsResizable(information.toBool());
    case IsMovable: return setInformationIsMovable(information.toBool());
    case IsAnchoredByChildren: return setInformationIsAnchoredByChildren(information.toBool());
    case IsAnchoredBySibling: return setInformationIsAnchoredBySibling(information.toBool());
    case HasContent: return setInformationHasContent(information.toBool());
    case HasAnchor: return setInformationHasAnchor(information.toByteArray(), secondInformation.toBool());break;
    case Anchor: return setInformationAnchor(information.toByteArray(), secondInformation.toByteArray(), thirdInformation.value<qint32>());
    case InstanceTypeForProperty: return setInformationInstanceTypeForProperty(information.toByteArray(), secondInformation.toByteArray());
    case HasBindingForProperty: return setInformationHasBindingForProperty(information.toByteArray(), secondInformation.toBool());
    case NoName:
    default: break;
    }

    return NoInformationChange;
}

bool operator ==(const NodeInstance &first, const NodeInstance &second)
{
    return first.instanceId() >= 0 && first.instanceId() == second.instanceId();
}

}
