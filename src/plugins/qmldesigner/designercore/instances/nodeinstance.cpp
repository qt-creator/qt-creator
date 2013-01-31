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

#include "nodeinstance.h"

#include <QPainter>
#include <modelnode.h>
#include "commondefines.h"

#include <QDebug>

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

    QPixmap renderPixmap;
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
    if (d)
        return  d->modelNode;
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
        return  d->boundingRect;
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
QTransform NodeInstance::sceneTransform() const
{
    if (isValid())
        return d->sceneTransform;
    else
        return QTransform();
}
bool NodeInstance::isInPositioner() const
{
    if (isValid())
        return d->isInPositioner;
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

void NodeInstance::paint(QPainter *painter)
{
    if (isValid() && !d->renderPixmap.isNull())
        painter->drawPixmap(boundingRect().topLeft(), d->renderPixmap);
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
    if (isValid())
        return d->parentInstanceId;
    else
        return false;
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

QPixmap NodeInstance::renderPixmap() const
{
    return d->renderPixmap;
}

void NodeInstance::setRenderPixmap(const QImage &image)
{
    if (!image.isNull())
        d->renderPixmap = QPixmap::fromImage(image);
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

InformationName NodeInstance::setInformationTransform(const QTransform &transform)
{
    if (d->transform != transform) {
        d->transform = transform;
        return Transform;
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

InformationName NodeInstance::setInformationIsInPositioner(bool isInPositioner)
{
    if (d->isInPositioner != isInPositioner) {
        d->isInPositioner = isInPositioner;
        return IsInPositioner;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationSceneTransform(const QTransform &sceneTransform)
{
    if (d->sceneTransform != sceneTransform) {
        d->sceneTransform = sceneTransform;
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

InformationName NodeInstance::setInformationHasAnchor(const QString &sourceAnchorLine, bool hasAnchor)
{
    if (d->hasAnchors.value(sourceAnchorLine) != hasAnchor) {
        d->hasAnchors.insert(sourceAnchorLine, hasAnchor);
        return HasAnchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationAnchor(const QString &sourceAnchorLine, const QString &targetAnchorLine, qint32 targetInstanceId)
{
    QPair<QString, qint32>  anchorPair = QPair<QString, qint32>(targetAnchorLine, targetInstanceId);
    if (d->anchors.value(sourceAnchorLine) != anchorPair) {
        d->anchors.insert(sourceAnchorLine, anchorPair);
        return Anchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationInstanceTypeForProperty(const QString &property, const QString &type)
{
    if (d->instanceTypes.value(property) != type) {
        d->instanceTypes.insert(property, type);
        return InstanceTypeForProperty;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasBindingForProperty(const QString &property, bool hasProperty)
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
    case Size: return setInformationSize(information.toSizeF()); break;
    case BoundingRect: return setInformationBoundingRect(information.toRectF()); break;
    case Transform: return setInformationTransform(information.value<QTransform>()); break;
    case PenWidth: return setInformationPenWith(information.toInt()); break;
    case Position: return setInformationPosition(information.toPointF()); break;
    case IsInPositioner: return setInformationIsInPositioner(information.toBool()); break;
    case SceneTransform: return setInformationSceneTransform(information.value<QTransform>()); break;
    case IsResizable: return setInformationIsResizable(information.toBool()); break;
    case IsMovable: return setInformationIsMovable(information.toBool()); break;
    case IsAnchoredByChildren: return setInformationIsAnchoredByChildren(information.toBool()); break;
    case IsAnchoredBySibling: return setInformationIsAnchoredBySibling(information.toBool()); break;
    case HasContent: return setInformationHasContent(information.toBool()); break;
    case HasAnchor: return setInformationHasAnchor(information.toString(), secondInformation.toBool());break;
    case Anchor: return setInformationAnchor(information.toString(), secondInformation.toString(), thirdInformation.value<qint32>()); break;
    case InstanceTypeForProperty: return setInformationInstanceTypeForProperty(information.toString(), secondInformation.toString()); break;
    case HasBindingForProperty: return setInformationHasBindingForProperty(information.toString(), secondInformation.toBool()); break;
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
