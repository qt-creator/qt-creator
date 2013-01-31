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

#ifndef VARIANTROPERTY_H
#define VARIANTROPERTY_H

#include "qmldesignercorelib_global.h"
#include "abstractproperty.h"

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;

namespace Internal {
    class ModelPrivate;
}

class QMLDESIGNERCORE_EXPORT VariantProperty : public AbstractProperty
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::AbstractProperty;

public:
    void setValue(const QVariant &value);
    QVariant value() const;
    VariantProperty& operator= (const QVariant &value);

    void setDynamicTypeNameAndValue(const QString &type, const QVariant &value);
    Q_DECL_DEPRECATED VariantProperty& operator= (const QPair<QString, QVariant> &typeValuePair);

    VariantProperty();
    VariantProperty(const VariantProperty &property, AbstractView *view);
protected:
    VariantProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
};

QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const VariantProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const VariantProperty &VariantProperty);

}

#endif //VARIANTPROPERTY_H
