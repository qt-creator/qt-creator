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

#ifndef QMLPROPERTYCHANGESNODEINSTANCE_H
#define QMLPROPERTYCHANGESNODEINSTANCE_H

#include "objectnodeinstance.h"
#include <private/qdeclarativestateoperations_p.h>
#include <private/qdeclarativepropertychanges_p.h>

#include <QPair>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QDeclarativeProperty;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class QmlPropertyChangesNodeInstance;

class QmlPropertyChangesNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlPropertyChangesNodeInstance> Pointer;
    typedef QWeakPointer<QmlPropertyChangesNodeInstance> WeakPointer;

    static Pointer create(QObject *objectToBeWrapped);

    virtual void setPropertyVariant(const QString &name, const QVariant &value);
    virtual void setPropertyBinding(const QString &name, const QString &expression);
    virtual QVariant property(const QString &name) const;
    virtual void resetProperty(const QString &name);

    using ObjectNodeInstance::reparent; // keep the virtual reparent(...) method around
    void reparent(const ServerNodeInstance &oldParentInstance, const QString &oldParentProperty, const ServerNodeInstance &newParentInstance, const QString &newParentProperty);

protected:
    QmlPropertyChangesNodeInstance(QDeclarativePropertyChanges *object);
    QDeclarativePropertyChanges *changesObject() const;
};

} // namespace Internal
} // namespace QmlDesigner

//QML_DECLARE_TYPE(QmlDesigner::Internal::QmlPropertyChangesObject)

#endif // QMLPROPERTYCHANGESNODEINSTANCE_H
