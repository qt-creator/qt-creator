/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef ABSTRACTROPERTY_H
#define ABSTRACTROPERTY_H

#include <QVariant>
#include <QWeakPointer>
#include <QSharedPointer>
#include "corelib_global.h"

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
class CORESHARED_EXPORT VariantProperty;
class CORESHARED_EXPORT NodeListProperty;
class CORESHARED_EXPORT NodeAbstractProperty;
class CORESHARED_EXPORT BindingProperty;
class CORESHARED_EXPORT NodeProperty;
class QmlObjectNode;


namespace Internal {
    class InternalNode;
    class ModelPrivate;
}

class CORESHARED_EXPORT AbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::Internal::ModelPrivate;

    friend CORESHARED_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
    friend CORESHARED_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);
    friend CORESHARED_EXPORT uint qHash(const AbstractProperty& property);

public:
    AbstractProperty();
    ~AbstractProperty();
    AbstractProperty(const AbstractProperty &other);
    AbstractProperty& operator=(const AbstractProperty &other);
    AbstractProperty(const AbstractProperty &property, AbstractView *view);

    QString name() const;

    bool isValid() const;
    ModelNode parentModelNode() const;
    QmlObjectNode parentQmlObjectNode() const;

    bool isDefaultProperty() const;
    VariantProperty toVariantProperty() const;
    NodeListProperty toNodeListProperty() const;
    NodeAbstractProperty toNodeAbstractProperty() const;
    BindingProperty toBindingProperty() const;
    NodeProperty toNodeProperty() const;

    bool isVariantProperty() const;
    bool isNodeListProperty() const;
    bool isNodeAbstractProperty() const;
    bool isBindingProperty() const;
    bool isNodeProperty() const;

    bool isDynamic() const;
    QString dynamicTypeName() const;

protected:
    AbstractProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    AbstractProperty(const Internal::InternalPropertyPointer &property, Model* model, AbstractView *view);
    Internal::InternalNodePointer internalNode() const;
    Model *model() const;
    AbstractView *view() const;

private:
    QString m_propertyName;
    Internal::InternalNodePointer m_internalNode;
    QWeakPointer<Model> m_model;
    QWeakPointer<AbstractView> m_view;
};

CORESHARED_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
CORESHARED_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);
CORESHARED_EXPORT uint qHash(const AbstractProperty& property);
CORESHARED_EXPORT QTextStream& operator<<(QTextStream &stream, const AbstractProperty &property);
CORESHARED_EXPORT QDebug operator<<(QDebug debug, const AbstractProperty &AbstractProperty);
}

#endif //ABSTRACTPROPERTY_H
