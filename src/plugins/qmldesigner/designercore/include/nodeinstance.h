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

#ifndef PROXYNODEINSTANCE_H
#define PROXYNODEINSTANCE_H

#include <QSharedPointer>
#include <QTransform>
#include <QPointF>
#include <QSizeF>
#include <QPair>

#include "commondefines.h"

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
    bool hasContent() const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    bool isMovable() const;
    bool isResizable() const;
    QTransform transform() const;
    QTransform sceneTransform() const;
    bool isInPositioner() const;
    QPointF position() const;
    QSizeF size() const;
    int penWidth() const;
    void paint(QPainter *painter);

    QVariant property(const QString &name) const;
    bool hasBindingForProperty(const QString &name) const;
    QPair<QString, qint32> anchor(const QString &name) const;
    bool hasAnchor(const QString &name) const;
    QString instanceType(const QString &name) const;

    qint32 parentId() const;
    qint32 instanceId() const;

    QPixmap renderPixmap() const;

protected:
    void setProperty(const QString &name, const QVariant &value);
    InformationName setInformation(InformationName name,
                        const QVariant &information,
                        const QVariant &secondInformation,
                        const QVariant &thirdInformation);

    InformationName setInformationSize(const QSizeF &size);
    InformationName setInformationBoundingRect(const QRectF &rectangle);
    InformationName setInformationTransform(const QTransform &transform);
    InformationName setInformationPenWith(int penWidth);
    InformationName setInformationPosition(const QPointF &position);
    InformationName setInformationIsInPositioner(bool isInPositioner);
    InformationName setInformationSceneTransform(const QTransform &sceneTransform);
    InformationName setInformationIsResizable(bool isResizable);
    InformationName setInformationIsMovable(bool isMovable);
    InformationName setInformationIsAnchoredByChildren(bool isAnchoredByChildren);
    InformationName setInformationIsAnchoredBySibling(bool isAnchoredBySibling);
    InformationName setInformationHasContent(bool hasContent);
    InformationName setInformationHasAnchor(const QString &sourceAnchorLine, bool hasAnchor);
    InformationName setInformationAnchor(const QString &sourceAnchorLine, const QString &targetAnchorLine, qint32 targetInstanceId);
    InformationName setInformationInstanceTypeForProperty(const QString &property, const QString &type);
    InformationName setInformationHasBindingForProperty(const QString &property, bool hasProperty);

    void setParentId(qint32 instanceId);
    void setRenderPixmap(const QImage &image);
    NodeInstance(ProxyNodeInstanceData *d);

private:
    QSharedPointer<ProxyNodeInstanceData> d;
};

bool operator ==(const NodeInstance &first, const NodeInstance &second);

}

#endif // PROXYNODEINSTANCE_H
