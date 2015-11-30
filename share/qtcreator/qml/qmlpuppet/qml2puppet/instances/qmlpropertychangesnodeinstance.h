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
#ifndef QMLPROPERTYCHANGESNODEINSTANCE_H
#define QMLPROPERTYCHANGESNODEINSTANCE_H

#include "objectnodeinstance.h"

#include <QPair>
#include <QWeakPointer>

namespace QmlDesigner {

namespace Internal {

class QmlPropertyChangesNodeInstance;

class QmlPropertyChangesNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlPropertyChangesNodeInstance> Pointer;
    typedef QWeakPointer<QmlPropertyChangesNodeInstance> WeakPointer;

    static Pointer create(QObject *objectToBeWrapped);

    virtual void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    virtual void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    virtual QVariant property(const PropertyName &name) const override;
    virtual void resetProperty(const PropertyName &name) override;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty) override;

protected:
    QmlPropertyChangesNodeInstance(QObject *object);
};

} // namespace Internal
} // namespace QmlDesigner

//QML_DECLARE_TYPE(QmlDesigner::Internal::QmlPropertyChangesObject)

#endif // QMLPROPERTYCHANGESNODEINSTANCE_H
