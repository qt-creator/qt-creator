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

#include <QPointer>
#include <QSharedPointer>
#include "qmldesignercorelib_global.h"

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
        class InternalNode;
        class InternalProperty;

        typedef QSharedPointer<InternalNode> InternalNodePointer;
        typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
        typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;
    }

class Model;
class ModelNode;
class AbstractView;
class QMLDESIGNERCORE_EXPORT VariantProperty;
class QMLDESIGNERCORE_EXPORT NodeListProperty;
class QMLDESIGNERCORE_EXPORT NodeAbstractProperty;
class QMLDESIGNERCORE_EXPORT BindingProperty;
class QMLDESIGNERCORE_EXPORT NodeProperty;
class QMLDESIGNERCORE_EXPORT SignalHandlerProperty;
class QmlObjectNode;


namespace Internal {
    class InternalNode;
    class ModelPrivate;
}

class QMLDESIGNERCORE_EXPORT AbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::Internal::ModelPrivate;

    friend QMLDESIGNERCORE_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT uint qHash(const AbstractProperty& property);

public:
    AbstractProperty();
    ~AbstractProperty();
    AbstractProperty(const AbstractProperty &other);
    AbstractProperty& operator=(const AbstractProperty &other);
    AbstractProperty(const AbstractProperty &property, AbstractView *view);

    PropertyName name() const;

    bool isValid() const;
    bool exists() const;
    ModelNode parentModelNode() const;
    QmlObjectNode parentQmlObjectNode() const;

    bool isDefaultProperty() const;
    VariantProperty toVariantProperty() const;
    NodeListProperty toNodeListProperty() const;
    NodeAbstractProperty toNodeAbstractProperty() const;
    BindingProperty toBindingProperty() const;
    NodeProperty toNodeProperty() const;
    SignalHandlerProperty toSignalHandlerProperty() const;

    bool isVariantProperty() const;
    bool isNodeListProperty() const;
    bool isNodeAbstractProperty() const;
    bool isBindingProperty() const;
    bool isNodeProperty() const;
    bool isSignalHandlerProperty() const;

    bool isDynamic() const;
    TypeName dynamicTypeName() const;

    Model *model() const;
    AbstractView *view() const;

protected:
    AbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    AbstractProperty(const Internal::InternalPropertyPointer &property, Model* model, AbstractView *view);
    Internal::InternalNodePointer internalNode() const;

private:
    PropertyName m_propertyName;
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
QMLDESIGNERCORE_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);
QMLDESIGNERCORE_EXPORT uint qHash(const AbstractProperty& property);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const AbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const AbstractProperty &AbstractProperty);
}
