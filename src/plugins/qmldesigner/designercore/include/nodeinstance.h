// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSharedPointer>
#include <QTransform>
#include <QPointF>
#include <QSizeF>
#include <QPair>

#include "commondefines.h"
#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class ModelNode;
class NodeInstanceView;
class ProxyNodeInstanceData;

class NodeInstance
{
    friend NodeInstanceView;

public:
    static NodeInstance create(const ModelNode &node);
    NodeInstance();
    ~NodeInstance();
    NodeInstance(const NodeInstance &other);
    NodeInstance& operator=(const NodeInstance &other);

    ModelNode modelNode() const;
    bool isValid() const;
    void makeInvalid();
    QRectF boundingRect() const;
    QRectF contentItemBoundingRect() const;
    bool hasContent() const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    bool isMovable() const;
    bool isResizable() const;
    QTransform transform() const;
    QTransform contentTransform() const;
    QTransform contentItemTransform() const;
    QTransform sceneTransform() const;
    bool isInLayoutable() const;
    QPointF position() const;
    QSizeF size() const;
    int penWidth() const;

    QPixmap renderPixmap() const;
    QPixmap blurredRenderPixmap() const;

    QVariant property(const PropertyName &name) const;
    bool hasProperty(const PropertyName &name) const;
    bool hasBindingForProperty(const PropertyName &name) const;
    QPair<PropertyName, qint32> anchor(const PropertyName &name) const;
    bool hasAnchor(const PropertyName &name) const;
    TypeName instanceType(const PropertyName &name) const;

    qint32 parentId() const;
    qint32 instanceId() const;

    void setDirectUpdate(bool directUpdates);
    bool directUpdates() const;
    void setX(double x);
    void setY(double y);

    bool hasAnchors() const;
    QString error() const;
    bool hasError() const;
    QStringList allStateNames() const;

protected:
    void setProperty(const PropertyName &name, const QVariant &value);
    InformationName setInformation(InformationName name,
                        const QVariant &information,
                        const QVariant &secondInformation,
                        const QVariant &thirdInformation);

    InformationName setInformationSize(const QSizeF &size);
    InformationName setInformationBoundingRect(const QRectF &rectangle);
    InformationName setInformationBoundingRectPixmap(const QRectF &rectangle);
    InformationName setInformationContentItemBoundingRect(const QRectF &rectangle);
    InformationName setInformationTransform(const QTransform &transform);
    InformationName setInformationContentTransform(const QTransform &contentTransform);
    InformationName setInformationContentItemTransform(const QTransform &contentItemTransform);
    InformationName setInformationPenWith(int penWidth);
    InformationName setInformationPosition(const QPointF &position);
    InformationName setInformationIsInLayoutable(bool isInLayoutable);
    InformationName setInformationSceneTransform(const QTransform &sceneTransform);
    InformationName setInformationIsResizable(bool isResizable);
    InformationName setInformationIsMovable(bool isMovable);
    InformationName setInformationIsAnchoredByChildren(bool isAnchoredByChildren);
    InformationName setInformationIsAnchoredBySibling(bool isAnchoredBySibling);
    InformationName setInformationHasContent(bool hasContent);
    InformationName setInformationHasAnchor(const PropertyName &sourceAnchorLine, bool hasAnchor);
    InformationName setInformationAnchor(const PropertyName &sourceAnchorLine, const PropertyName &targetAnchorLine, qint32 targetInstanceId);
    InformationName setInformationInstanceTypeForProperty(const PropertyName &property, const TypeName &type);
    InformationName setInformationHasBindingForProperty(const PropertyName &property, bool hasProperty);
    InformationName setAllStates(const QStringList &states);

    void setParentId(qint32 instanceId);
    void setRenderPixmap(const QImage &image);
    bool setError(const QString &errorMessage);
    NodeInstance(ProxyNodeInstanceData *d);

private:
    QSharedPointer<ProxyNodeInstanceData> d;
};

bool operator ==(const NodeInstance &first, const NodeInstance &second);

}
