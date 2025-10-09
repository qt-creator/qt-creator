// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstance.h"
#include "nodeinstancetracing.h"

#include <modelnode.h>

#include <QDebug>
#include <QPainter>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

QT_BEGIN_NAMESPACE
void qt_blurImage(QPainter *painter, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

namespace QmlDesigner {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

class ProxyNodeInstanceData
{
public:
    ProxyNodeInstanceData() = default;

    ProxyNodeInstanceData(const ModelNode &node)
        : modelNode{node}
    {}

    qint32 parentInstanceId{-1};
    ModelNode modelNode;
    QRectF boundingRect;
    QRectF boundingRectPixmap;
    QRectF contentItemBoundingRect;
    QPointF position;
    QSizeF size;
    QSizeF implicitSize;
    QTransform transform;
    QTransform contentTransform;
    QTransform contentItemTransform;
    QTransform sceneTransform;
    int penWidth{1};
    bool isAnchoredBySibling{false};
    bool isAnchoredByChildren{false};
    bool hasContent{false};
    bool isMovable{false};
    bool isResizable{false};
    bool isInLayoutable{false};
    bool directUpdates{false};

    std::map<Utils::SmallString, QVariant, std::less<>> propertyValues;
    std::map<Utils::SmallString, bool, std::less<>> hasBindingForProperty;
    std::map<Utils::SmallString, bool, std::less<>> hasAnchors;
    std::map<Utils::SmallString, TypeName, std::less<>> instanceTypes;

    QPixmap renderPixmap;
    QPixmap blurredRenderPixmap;

    QString errorMessage;

    std::map<Utils::SmallString, std::pair<PropertyName, qint32>, std::less<>> anchors;
    QStringList allStates;
};

NodeInstance::NodeInstance(const ModelNode &node)
    : d(std::make_shared<ProxyNodeInstanceData>(node))
{
    NanotraceHR::Tracer tracer{"node instance constructor", category(), keyValue("node", node)};
}

NodeInstance NodeInstance::create(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"node instance create", category()};

    return NodeInstance{node};
}

NodeInstance::~NodeInstance()
{
    NanotraceHR::Tracer tracer{"node instance destructor", category()};
}

NodeInstance::NodeInstance(const NodeInstance &other) = default;
NodeInstance &NodeInstance::operator=(const NodeInstance &other) = default;
NodeInstance::NodeInstance(NodeInstance &&other) noexcept = default;
NodeInstance &NodeInstance::operator=(NodeInstance &&other) noexcept = default;

ModelNode NodeInstance::modelNode() const
{
    NanotraceHR::Tracer tracer{"node instance model node", category()};

    if (d)
        return d->modelNode;
    else
        return ModelNode();
}

qint32 NodeInstance::instanceId() const
{
    NanotraceHR::Tracer tracer{"node instance id", category()};

    if (d)
        return d->modelNode.internalId();
    else
        return -1;
}

void NodeInstance::setDirectUpdate(bool directUpdates)
{
    NanotraceHR::Tracer tracer{"node instance set direct update",
                               category(),
                               keyValue("direct updates", directUpdates)};

    if (d)
        d->directUpdates = directUpdates;
}

bool NodeInstance::directUpdates() const
{
    NanotraceHR::Tracer tracer{"node instance direct updates", category()};

    if (d)
        return d->directUpdates && !(d->transform.isRotating() || d->transform.isScaling() || hasAnchors());
    else
        return true;
}

void NodeInstance::setX(double x)
{
    NanotraceHR::Tracer tracer{"node instance set x", category(), keyValue("x", x)};

    if (d && directUpdates()) {
        double dx = x - d->transform.dx();
        d->transform.translate(dx, 0.0);
    }
}

void NodeInstance::setY(double y)
{
    NanotraceHR::Tracer tracer{"node instance set y", category(), keyValue("y", y)};

    if (d && directUpdates()) {
        double dy = y - d->transform.dy();
        d->transform.translate(0.0, dy);
    }
}

bool NodeInstance::hasAnchors() const
{
    NanotraceHR::Tracer tracer{"node instance has anchors", category()};

    return hasAnchor("anchors.fill") || hasAnchor("anchors.centerIn") || hasAnchor("anchors.top")
           || hasAnchor("anchors.left") || hasAnchor("anchors.right") || hasAnchor("anchors.bottom")
           || hasAnchor("anchors.horizontalCenter") || hasAnchor("anchors.verticalCenter")
           || hasAnchor("anchors.baseline");
}

QString NodeInstance::error() const
{
    NanotraceHR::Tracer tracer{"node instance error", category()};

    if (d)
        return d->errorMessage;

    return {};
}

bool NodeInstance::hasError() const
{
    NanotraceHR::Tracer tracer{"node instance has error", category()};

    if (d)
        return !d->errorMessage.isEmpty();

    return false;
}

QStringList NodeInstance::allStateNames() const
{
    NanotraceHR::Tracer tracer{"node instance all state names", category()};

    if (d)
        return d->allStates;

    return {};
}

static constinit NodeInstance nullInstance;

NodeInstance &NodeInstance::null()
{
    NanotraceHR::Tracer tracer{"node instance null", category()};

    return nullInstance;
}

bool NodeInstance::isValid() const
{
    NanotraceHR::Tracer tracer{"node instance is valid", category()};

    return instanceId() >= 0 && modelNode().isValid();
}

void NodeInstance::makeInvalid()
{
    NanotraceHR::Tracer tracer{"node instance make invalid", category()};

    if (d)
        d->modelNode = ModelNode();
}

QRectF NodeInstance::boundingRect() const
{
    NanotraceHR::Tracer tracer{"node instance bounding rect", category()};

    if (isValid()) {
        if (d->boundingRectPixmap.isValid())
            return d->boundingRectPixmap;
        return d->boundingRect;
    } else
        return QRectF();
}

QRectF NodeInstance::contentItemBoundingRect() const
{
    NanotraceHR::Tracer tracer{"node instance content item bounding rect", category()};

    if (isValid())
        return d->contentItemBoundingRect;
    else
        return QRectF();
}

bool NodeInstance::hasContent() const
{
    NanotraceHR::Tracer tracer{"node instance has content", category()};

    if (isValid())
        return d->hasContent;
    else
        return false;
}

bool NodeInstance::isAnchoredBySibling() const
{
    NanotraceHR::Tracer tracer{"node instance is anchored by sibling", category()};

    if (isValid())
        return d->isAnchoredBySibling;
    else
        return false;
}

bool NodeInstance::isAnchoredByChildren() const
{
    NanotraceHR::Tracer tracer{"node instance is anchored by children", category()};

    if (isValid())
        return d->isAnchoredByChildren;
    else
        return false;
}

bool NodeInstance::isMovable() const
{
    NanotraceHR::Tracer tracer{"node instance is movable", category()};

    if (isValid())
        return d->isMovable;
    else
        return false;
}

bool NodeInstance::isResizable() const
{
    NanotraceHR::Tracer tracer{"node instance is resizable", category()};

    if (isValid())
        return d->isResizable;
    else
        return false;
}

QTransform NodeInstance::transform() const
{
    NanotraceHR::Tracer tracer{"node instance transform", category()};

    if (isValid())
        return d->transform;
    else
        return QTransform();
}

QTransform NodeInstance::contentTransform() const
{
    NanotraceHR::Tracer tracer{"node instance content transform", category()};

    if (isValid())
        return d->contentTransform;
    else
        return QTransform();
}

QTransform NodeInstance::contentItemTransform() const
{
    NanotraceHR::Tracer tracer{"node instance content item transform", category()};

    if (isValid())
        return d->contentItemTransform;
    else
        return QTransform();
}
QTransform NodeInstance::sceneTransform() const
{
    NanotraceHR::Tracer tracer{"node instance scene transform", category()};

    if (isValid())
        return d->sceneTransform;
    else
        return QTransform();
}
bool NodeInstance::isInLayoutable() const
{
    NanotraceHR::Tracer tracer{"node instance is in layoutable", category()};

    if (isValid())
        return d->isInLayoutable;
    else
        return false;
}

QPointF NodeInstance::position() const
{
    NanotraceHR::Tracer tracer{"node instance position", category()};

    if (isValid())
        return d->position;
    else
        return QPointF();
}

QSizeF NodeInstance::size() const
{
    NanotraceHR::Tracer tracer{"node instance size", category()};

    if (isValid())
        return d->size;
    else
        return QSizeF();
}

int NodeInstance::penWidth() const
{
    NanotraceHR::Tracer tracer{"node instance pen width", category()};

    if (isValid())
        return d->penWidth;
    else
        return 1;
}

namespace {

template<typename... Arguments>
auto value(const std::map<Arguments...> &dict,
           PropertyNameView key,
           typename std::map<Arguments...>::mapped_type defaultValue = {})
{
    if (auto found = dict.find(key); found != dict.end())
        return found->second;

    return defaultValue;
}

} // namespace

QVariant NodeInstance::property(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance property", category(), keyValue("name", name)};

    if (isValid()) {
        if (auto found = d->propertyValues.find(name); found != d->propertyValues.end()) {
            return found->second;
        } else {
            // Query may be for a subproperty, e.g. scale.x
            const auto index = name.indexOf('.');
            if (index != -1) {
                PropertyNameView parentPropName = name.left(index);
                QVariant varValue = value(d->propertyValues, parentPropName);
                if (varValue.typeId() == QMetaType::QVector2D) {
                    auto value = varValue.value<QVector2D>();
                    char subProp = name.right(1)[0];
                    float subValue = 0.f;
                    switch (subProp) {
                    case 'x':
                        subValue = value.x();
                        break;
                    case 'y':
                        subValue = value.y();
                        break;
                    default:
                        subValue = 0.f;
                        break;
                    }
                    return QVariant(subValue);
                } else if (varValue.typeId() == QMetaType::QVector3D) {
                    auto value = varValue.value<QVector3D>();
                    char subProp = name.right(1)[0];
                    float subValue = 0.f;
                    switch (subProp) {
                    case 'x':
                        subValue = value.x();
                        break;
                    case 'y':
                        subValue = value.y();
                        break;
                    case 'z':
                        subValue = value.z();
                        break;
                    default:
                        subValue = 0.f;
                        break;
                    }
                    return QVariant(subValue);
                } else if (varValue.typeId() == QMetaType::QVector4D) {
                    auto value = varValue.value<QVector4D>();
                    char subProp = name.right(1)[0];
                    float subValue = 0.f;
                    switch (subProp) {
                    case 'x':
                        subValue = value.x();
                        break;
                    case 'y':
                        subValue = value.y();
                        break;
                    case 'z':
                        subValue = value.z();
                        break;
                    case 'w':
                        subValue = value.w();
                        break;
                    default:
                        subValue = 0.f;
                        break;
                    }
                    return QVariant(subValue);
                }
            }
        }
    }

    return QVariant();
}

bool NodeInstance::hasProperty(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance has property", category(), keyValue("name", name)};

    if (isValid())
        return d->propertyValues.contains(name);

    return false;
}

bool NodeInstance::hasBindingForProperty(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance has binding for property",
                               category(),
                               keyValue("name", name)};

    if (isValid())
        return value(d->hasBindingForProperty, name, false);

    return false;
}

TypeName NodeInstance::instanceType(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance instance type", category(), keyValue("name", name)};

    if (isValid())
        return value(d->instanceTypes, name);

    return TypeName();
}

qint32 NodeInstance::parentId() const
{
    NanotraceHR::Tracer tracer{"node instance parent id", category()};

    if (isValid())
        return d->parentInstanceId;
    else
        return -1;
}

bool NodeInstance::hasAnchor(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance has anchor", category(), keyValue("name", name)};

    if (isValid())
        return value(d->hasAnchors, name, false);

    return false;
}

QPair<PropertyName, qint32> NodeInstance::anchor(PropertyNameView name) const
{
    NanotraceHR::Tracer tracer{"node instance anchor", category(), keyValue("name", name)};

    if (isValid())
        return value(d->anchors, name, QPair<PropertyName, qint32>(PropertyName(), qint32(-1)));

    return QPair<PropertyName, qint32>(PropertyName(), -1);
}

void NodeInstance::setProperty(PropertyNameView name, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"node instance set property", category(), keyValue("name", name)};

    const auto index = name.indexOf('.');
    if (index != -1) {
        PropertyNameView parentPropName = name.left(index);
        QVariant oldValue = QmlDesigner::value(d->propertyValues, parentPropName);
        QVariant newValueVar;
        bool update = false;
        if (oldValue.typeId() == QMetaType::QVector2D) {
            QVector2D newValue;
            if (oldValue.typeId() == QMetaType::QVector2D)
                newValue = oldValue.value<QVector2D>();
            if (name.endsWith(".x")) {
                newValue.setX(value.toFloat());
                update = true;
            } else if (name.endsWith(".y")) {
                newValue.setY(value.toFloat());
                update = true;
            }
            newValueVar = newValue;
        } else if (oldValue.typeId() == QMetaType::QVector3D) {
            QVector3D newValue;
            if (oldValue.typeId() == QMetaType::QVector3D)
                newValue = oldValue.value<QVector3D>();
            if (name.endsWith(".x")) {
                newValue.setX(value.toFloat());
                update = true;
            } else if (name.endsWith(".y")) {
                newValue.setY(value.toFloat());
                update = true;
            } else if (name.endsWith(".z")) {
                newValue.setZ(value.toFloat());
                update = true;
            }
            newValueVar = newValue;
        } else if (oldValue.typeId() == QMetaType::QVector4D) {
            QVector4D newValue;
            if (oldValue.typeId() == QMetaType::QVector4D)
                newValue = oldValue.value<QVector4D>();
            if (name.endsWith(".x")) {
                newValue.setX(value.toFloat());
                update = true;
            } else if (name.endsWith(".y")) {
                newValue.setY(value.toFloat());
                update = true;
            } else if (name.endsWith(".z")) {
                newValue.setZ(value.toFloat());
                update = true;
            } else if (name.endsWith(".w")) {
                newValue.setW(value.toFloat());
                update = true;
            }
            newValueVar = newValue;
        }
        if (update) {
            d->propertyValues.insert_or_assign(parentPropName, newValueVar);
            return;
        }
    }

    d->propertyValues.insert_or_assign(name, value);
}

QPixmap NodeInstance::renderPixmap() const
{
    NanotraceHR::Tracer tracer{"node instance render pixmap", category()};

    if (isValid())
        return d->renderPixmap;

    return {};
}

QPixmap NodeInstance::blurredRenderPixmap() const
{
    NanotraceHR::Tracer tracer{"node instance blurred render pixmap", category()};

    if (!isValid())
        return {};

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
    NanotraceHR::Tracer tracer{"node instance set render pixmap", category(), keyValue("image", image)};

    d->renderPixmap = QPixmap::fromImage(image);

    d->blurredRenderPixmap = QPixmap();
}

bool NodeInstance::setError(const QString &errorMessage)
{
    NanotraceHR::Tracer tracer{"node instance set error",
                               category(),
                               keyValue("error message", errorMessage)};

    if (d->errorMessage != errorMessage) {
        d->errorMessage = errorMessage;
        return true;
    }
    return false;
}

void NodeInstance::setParentId(qint32 instanceId)
{
    NanotraceHR::Tracer tracer{"node instance set parent id",
                               category(),
                               keyValue("instance id", instanceId)};

    d->parentInstanceId = instanceId;
}

InformationName NodeInstance::setInformationSize(const QSizeF &size)
{
    NanotraceHR::Tracer tracer{"node instance set information size", category()};
    if (d->size != size) {
        d->size = size;
        return Size;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationImplicitSize(const QSizeF &implicitSize)
{
    NanotraceHR::Tracer tracer{"node instance set information implicit size", category()};

    if (d->implicitSize != implicitSize) {
        d->implicitSize = implicitSize;
        return ImplicitSize;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationBoundingRect(const QRectF &rectangle)
{
    NanotraceHR::Tracer tracer{"node instance set information bounding rect", category()};

    if (d->boundingRect != rectangle) {
        d->boundingRect = rectangle;
        return BoundingRect;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationBoundingRectPixmap(const QRectF &rectangle)
{
    NanotraceHR::Tracer tracer{"node instance set information bounding rect pixmap", category()};

    if (d->boundingRectPixmap != rectangle) {
        d->boundingRectPixmap = rectangle;
        return BoundingRectPixmap;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentItemBoundingRect(const QRectF &rectangle)
{
    NanotraceHR::Tracer tracer{"node instance set information content item bounding rect", category()};

    if (d->contentItemBoundingRect != rectangle) {
        d->contentItemBoundingRect = rectangle;
        return ContentItemBoundingRect;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationTransform(const QTransform &transform)
{
    NanotraceHR::Tracer tracer{"node instance set information transform", category()};

    if (!directUpdates() && d->transform != transform) {
        d->transform = transform;
        return Transform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentTransform(const QTransform &contentTransform)
{
    NanotraceHR::Tracer tracer{"node instance set information content transform", category()};

    if (d->contentTransform != contentTransform) {
        d->contentTransform = contentTransform;
        return ContentTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationContentItemTransform(const QTransform &contentItemTransform)
{
    NanotraceHR::Tracer tracer{"node instance set information content item transform", category()};

    if (d->contentItemTransform != contentItemTransform) {
        d->contentItemTransform = contentItemTransform;
        return ContentItemTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationPenWith(int penWidth)
{
    NanotraceHR::Tracer tracer{"node instance set information pen width", category()};

    if (d->penWidth != penWidth) {
        d->penWidth = penWidth;
        return PenWidth;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationPosition(const QPointF &position)
{
    NanotraceHR::Tracer tracer{"node instance set information position", category()};

    if (d->position != position) {
        d->position = position;
        return Position;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsInLayoutable(bool isInLayoutable)
{
    NanotraceHR::Tracer tracer{"node instance set information is in layoutable", category()};

    if (d->isInLayoutable != isInLayoutable) {
        d->isInLayoutable = isInLayoutable;
        return IsInLayoutable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationSceneTransform(const QTransform &sceneTransform)
{
    NanotraceHR::Tracer tracer{"node instance set information scene transform", category()};

    if (d->sceneTransform != sceneTransform) {
        d->sceneTransform = sceneTransform;
        if (!directUpdates())
            return SceneTransform;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsResizable(bool isResizable)
{
    NanotraceHR::Tracer tracer{"node instance set information is resizable", category()};

    if (d->isResizable != isResizable) {
        d->isResizable = isResizable;
        return IsResizable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsMovable(bool isMovable)
{
    NanotraceHR::Tracer tracer{"node instance set information is movable", category()};

    if (d->isMovable != isMovable) {
        d->isMovable = isMovable;
        return IsMovable;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsAnchoredByChildren(bool isAnchoredByChildren)
{
    NanotraceHR::Tracer tracer{"node instance set information is anchored by children", category()};

    if (d->isAnchoredByChildren != isAnchoredByChildren) {
        d->isAnchoredByChildren = isAnchoredByChildren;
        return IsAnchoredByChildren;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationIsAnchoredBySibling(bool isAnchoredBySibling)
{
    NanotraceHR::Tracer tracer{"node instance set information is anchored by sibling", category()};

    if (d->isAnchoredBySibling != isAnchoredBySibling) {
        d->isAnchoredBySibling = isAnchoredBySibling;
        return IsAnchoredBySibling;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasContent(bool hasContent)
{
    NanotraceHR::Tracer tracer{"node instance set information has content", category()};

    if (d->hasContent != hasContent) {
        d->hasContent = hasContent;
        return HasContent;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasAnchor(PropertyNameView sourceAnchorLine, bool hasAnchor)
{
    NanotraceHR::Tracer tracer{"node instance set information has anchor", category()};

    if (auto found = d->hasAnchors.find(sourceAnchorLine);
        found == d->hasAnchors.end() || found->second != hasAnchor) {
        d->hasAnchors.insert_or_assign(found, sourceAnchorLine, hasAnchor);
        return HasAnchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationAnchor(PropertyNameView sourceAnchorLine,
                                                   const PropertyName &targetAnchorLine,
                                                   qint32 targetInstanceId)
{
    NanotraceHR::Tracer tracer{"node instance set information anchor", category()};

    std::pair<PropertyName, qint32> anchorPair = std::pair<PropertyName, qint32>(targetAnchorLine,
                                                                                 targetInstanceId);
    if (auto found = d->anchors.find(sourceAnchorLine);
        found == d->anchors.end() || found->second != anchorPair) {
        d->anchors.insert_or_assign(found, sourceAnchorLine, anchorPair);
        return Anchor;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationInstanceTypeForProperty(PropertyNameView property,
                                                                    const TypeName &type)
{
    NanotraceHR::Tracer tracer{"node instance set information instance type for property", category()};

    if (auto found = d->instanceTypes.find(property);
        found == d->instanceTypes.end() || found->second != type) {
        d->instanceTypes.insert_or_assign(found, property, type);
        return InstanceTypeForProperty;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformationHasBindingForProperty(PropertyNameView property,
                                                                  bool hasProperty)
{
    NanotraceHR::Tracer tracer{"node instance set information has binding for property", category()};

    if (auto found = d->hasBindingForProperty.find(property);
        found == d->hasBindingForProperty.end() || found->second != hasProperty) {
        d->hasBindingForProperty.insert_or_assign(found, property, hasProperty);
        return HasBindingForProperty;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setAllStates(const QStringList &states)
{
    NanotraceHR::Tracer tracer{"node instance set all states", category()};

    if (d->allStates != states) {
        d->allStates = states;
        return AllStates;
    }

    return NoInformationChange;
}

InformationName NodeInstance::setInformation(InformationName name, const QVariant &information, const QVariant &secondInformation, const QVariant &thirdInformation)
{
    NanotraceHR::Tracer tracer{"node instance set information", category(), keyValue("name", name)};

    switch (name) {
    case Size: return setInformationSize(information.toSizeF());
    case ImplicitSize:
        return setInformationImplicitSize(information.toSizeF());
    case BoundingRect:
        return setInformationBoundingRect(information.toRectF());
    case BoundingRectPixmap:
        return setInformationBoundingRectPixmap(information.toRectF());
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
    case AllStates: return setAllStates(information.toStringList());
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
