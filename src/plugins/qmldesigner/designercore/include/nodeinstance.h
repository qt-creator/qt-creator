/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROXYNODEINSTANCE_H
#define PROXYNODEINSTANCE_H

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
    friend class NodeInstanceView;
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

protected:
    void setProperty(const PropertyName &name, const QVariant &value);
    InformationName setInformation(InformationName name,
                        const QVariant &information,
                        const QVariant &secondInformation,
                        const QVariant &thirdInformation);

    InformationName setInformationSize(const QSizeF &size);
    InformationName setInformationBoundingRect(const QRectF &rectangle);
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

    void setParentId(qint32 instanceId);
    void setRenderPixmap(const QImage &image);
    bool setError(const QString &errorMessage);
    NodeInstance(ProxyNodeInstanceData *d);

private:
    QSharedPointer<ProxyNodeInstanceData> d;
};

bool operator ==(const NodeInstance &first, const NodeInstance &second);

}

#endif // PROXYNODEINSTANCE_H
