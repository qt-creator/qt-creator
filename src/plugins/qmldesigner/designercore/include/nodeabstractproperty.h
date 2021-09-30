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

#pragma once

#include "abstractproperty.h"

namespace QmlDesigner {

namespace Internal {
    class InternalNodeAbstractProperty;
    using InternalNodeAbstractPropertyPointer = QSharedPointer<InternalNodeAbstractProperty>;
}

class QMLDESIGNERCORE_EXPORT NodeAbstractProperty : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

    friend QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);

public:
    NodeAbstractProperty();
    NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view);
    void reparentHere(const ModelNode &modelNode);
    bool isEmpty() const;
    int count() const;
    int indexOf(const ModelNode &node) const;
    NodeAbstractProperty parentProperty() const;

    QList<ModelNode> allSubNodes();
    QList<ModelNode> directSubNodes() const;

    friend auto qHash(const NodeAbstractProperty &property) { qHash(AbstractProperty(property)); }

protected:
    NodeAbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view);
    NodeAbstractProperty(const Internal::InternalNodeAbstractPropertyPointer &property, Model *model, AbstractView *view);
    void reparentHere(const ModelNode &modelNode, bool isNodeList, const TypeName &typeName = TypeName());
};


QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const NodeAbstractProperty &property);

} // namespace QmlDesigner
