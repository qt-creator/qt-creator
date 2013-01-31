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

#ifndef INTERNALPROPERTY_H
#define INTERNALPROPERTY_H

#include "qmldesignercorelib_global.h"

#include <QVariant>
#include <QString>
#include <QRegExp>
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>
#include <QSharedPointer>

namespace QmlDesigner {

namespace Internal {

class InternalBindingProperty;
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

    QString name() const;

    virtual bool isBindingProperty() const;
    virtual bool isVariantProperty() const;
    virtual bool isNodeListProperty() const;
    virtual bool isNodeProperty() const;
    virtual bool isNodeAbstractProperty() const;

    QSharedPointer<InternalBindingProperty> toBindingProperty() const;
    QSharedPointer<InternalVariantProperty> toVariantProperty() const;
    QSharedPointer<InternalNodeListProperty> toNodeListProperty() const;
    QSharedPointer<InternalNodeProperty> toNodeProperty() const;
    QSharedPointer<InternalNodeAbstractProperty> toNodeAbstractProperty() const;

    InternalNodePointer propertyOwner() const;

    virtual void remove();

    QString dynamicTypeName() const;

    void resetDynamicTypeName();

protected: // functions
    InternalProperty(const QString &name, const InternalNodePointer &propertyOwner);
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void setDynamicTypeName(const QString &name);
private:
    WeakPointer m_internalPointer;
    QString m_name;
    QString m_dynamicType;
    InternalNodeWeakPointer m_propertyOwner;

};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALPROPERTY_H
