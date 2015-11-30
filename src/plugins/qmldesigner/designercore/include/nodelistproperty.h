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

#ifndef NODELISTPROPERTY_H
#define NODELISTPROPERTY_H


#include "qmldesignercorelib_global.h"
#include "nodeabstractproperty.h"
#include <QList>


namespace QmlDesigner {

namespace Internal {
    class ModelPrivate;
    class InternalNodeListProperty;
    typedef QSharedPointer<InternalNodeListProperty> InternalNodeListPropertyPointer;
}


class QMLDESIGNERCORE_EXPORT NodeListProperty : public NodeAbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::Internal::ModelPrivate;
public:
    NodeListProperty();
    NodeListProperty(const NodeListProperty &nodeListProperty, AbstractView *view);
    const QList<ModelNode> toModelNodeList() const;
    const QList<QmlObjectNode> toQmlObjectNodeList() const;
    void slide(int, int) const;
    void reparentHere(const ModelNode &modelNode);
    ModelNode at(int index) const;

protected:
    NodeListProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    NodeListProperty(const Internal::InternalNodeListPropertyPointer &internalNodeListProperty, Model* model, AbstractView *view);
};
}

#endif //NODELISTROPERTY_H
