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

#ifndef VARIANTPROPERTY_H
#define VARIANTPROPERTY_H

#include "qmldesignercorelib_global.h"
#include "abstractproperty.h"
#include "enumeration.h"

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;

namespace Internal { class ModelPrivate; }

class QMLDESIGNERCORE_EXPORT VariantProperty : public AbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::AbstractProperty;

public:
    void setValue(const QVariant &value);
    QVariant value() const;

    void setEnumeration(const EnumerationName &enumerationName);
    Enumeration enumeration() const;
    bool holdsEnumeration() const;

    void setDynamicTypeNameAndValue(const TypeName &type, const QVariant &value);
    void setDynamicTypeNameAndEnumeration(const TypeName &type, const EnumerationName &enumerationName);

    VariantProperty();
    VariantProperty(const VariantProperty &property, AbstractView *view);
protected:
    VariantProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
};

QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const VariantProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const VariantProperty &VariantProperty);

}

#endif //VARIANTPROPERTY_H
