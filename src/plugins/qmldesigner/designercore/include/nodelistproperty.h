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
    NodeListProperty& operator= (const QVariant &value);

    NodeListProperty();
    NodeListProperty(const NodeListProperty &nodeListProperty, AbstractView *view);
    const QList<ModelNode> toModelNodeList() const;
    const QList<QmlObjectNode> toQmlObjectNodeList() const;
    void slide(int, int) const;
    void reparentHere(const ModelNode &modelNode);
    ModelNode at(int index) const;

protected:
    NodeListProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    NodeListProperty(const Internal::InternalNodeListPropertyPointer &internalNodeListProperty, Model* model, AbstractView *view);
};
}

#endif //NODELISTROPERTY_H
