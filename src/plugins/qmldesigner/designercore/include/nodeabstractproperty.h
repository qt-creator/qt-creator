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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef NODEABSTRACTPROPERTY_H
#define NODEABSTRACTPROPERTY_H

#include "abstractproperty.h"

namespace QmlDesigner {

namespace Internal {
    class InternalNodeAbstractProperty;
    typedef QSharedPointer<InternalNodeAbstractProperty> InternalNodeAbstractPropertyPointer;
}

class QMLDESIGNERCORE_EXPORT NodeAbstractProperty : public AbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::AbstractProperty;

    friend QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT uint qHash(const NodeAbstractProperty& property);

public:
    NodeAbstractProperty();
    NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view);
    void reparentHere(const ModelNode &modelNode);
    bool isEmpty() const;
    int count() const;
    int indexOf(const ModelNode &node) const;
    NodeAbstractProperty parentProperty() const;

    const QList<ModelNode> allSubNodes();
    const QList<ModelNode> directSubNodes() const;

protected:
    NodeAbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view);
    NodeAbstractProperty(const Internal::InternalNodeAbstractPropertyPointer &property, Model *model, AbstractView *view);
    void reparentHere(const ModelNode &modelNode, bool isNodeList);
};


QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT uint qHash(const NodeAbstractProperty& property);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const NodeAbstractProperty &property);

} // namespace QmlDesigner

#endif // NODEABSTRACTPROPERTY_H
