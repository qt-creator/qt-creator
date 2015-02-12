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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
