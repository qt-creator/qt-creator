/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef NODELISTPROPERTY_H
#define NODELISTPROPERTY_H


#include "corelib_global.h"
#include "nodeabstractproperty.h"
#include <QList>


namespace QmlDesigner {

namespace Internal {
    class ModelPrivate;
    class InternalNodeListProperty;
    typedef QSharedPointer<InternalNodeListProperty> InternalNodeListPropertyPointer;
}


class CORESHARED_EXPORT NodeListProperty : public NodeAbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::Internal::ModelPrivate;
public:
    NodeListProperty& operator= (const QVariant &value);

    NodeListProperty();
    NodeListProperty(const NodeListProperty &nodeListProperty, AbstractView *view);
    const QList<ModelNode> toModelNodeList() const;
    const QList<QmlObjectNode> toQmlObjectNodeList() const;
    void slide(int, int) const;
    void reparentHere(const ModelNode &modelNode);

    bool isEmpty() const;

protected:
    NodeListProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    NodeListProperty(const Internal::InternalNodeListPropertyPointer &internalNodeListProperty, Model* model, AbstractView *view);
};
}

#endif //NODELISTROPERTY_H
