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

#ifndef INTERNALPROPERTY_H
#define INTERNALPROPERTY_H

#include "qmldesignercorelib_global.h"

#include <QVariant>
#include <QSharedPointer>

namespace QmlDesigner {

namespace Internal {

class InternalBindingProperty;
class InternalSignalHandlerProperty;
class InternalVariantProperty;
class InternalNodeListProperty;
class InternalNodeProperty;
class InternalNodeAbstractProperty;
class InternalNode;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;

class QMLDESIGNERCORE_EXPORT InternalProperty
{
public:
    typedef QSharedPointer<InternalProperty> Pointer;
    typedef QWeakPointer<InternalProperty> WeakPointer;


    InternalProperty();
    virtual ~InternalProperty();

    virtual bool isValid() const;

    PropertyName name() const;

    virtual bool isBindingProperty() const;
    virtual bool isVariantProperty() const;
    virtual bool isNodeListProperty() const;
    virtual bool isNodeProperty() const;
    virtual bool isNodeAbstractProperty() const;
    virtual bool isSignalHandlerProperty() const;

    QSharedPointer<InternalBindingProperty> toBindingProperty() const;
    QSharedPointer<InternalVariantProperty> toVariantProperty() const;
    QSharedPointer<InternalNodeListProperty> toNodeListProperty() const;
    QSharedPointer<InternalNodeProperty> toNodeProperty() const;
    QSharedPointer<InternalNodeAbstractProperty> toNodeAbstractProperty() const;
    QSharedPointer<InternalSignalHandlerProperty> toSignalHandlerProperty() const;

    InternalNodePointer propertyOwner() const;

    virtual void remove();

    TypeName dynamicTypeName() const;

    void resetDynamicTypeName();

protected: // functions
    InternalProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void setDynamicTypeName(const TypeName &name);
private:
    WeakPointer m_internalPointer;
    PropertyName m_name;
    TypeName m_dynamicType;
    InternalNodeWeakPointer m_propertyOwner;

};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALPROPERTY_H
