/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmltransitionnodeinstance.h"
#include <private/qdeclarativetransition_p.h>
#include <nodemetainfo.h>
#include "invalidnodeinstanceexception.h"

namespace QmlDesigner {
namespace Internal {

QmlTransitionNodeInstance::QmlTransitionNodeInstance(QDeclarativeTransition *transition)
    : ObjectNodeInstance(transition)
{
}

QmlTransitionNodeInstance::Pointer QmlTransitionNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped)
{
     QObject *object = 0;
     if (objectToBeWrapped)
         object = objectToBeWrapped;
     else
         object = createObject(nodeMetaInfo, context);

     QDeclarativeTransition *transition = qobject_cast<QDeclarativeTransition*>(object);
     if (transition == 0)
         throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

     Pointer instance(new QmlTransitionNodeInstance(transition));

     if (objectToBeWrapped)
         instance->setDeleteHeldInstance(false); // the object isn't owned

     instance->populateResetValueHash();

     transition->setToState("invalidState");
     transition->setFromState("invalidState");

     return instance;
}

bool QmlTransitionNodeInstance::isTransition() const
{
    return true;
}

void QmlTransitionNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "from" || name == "to")
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

QDeclarativeTransition *QmlTransitionNodeInstance::qmlTransition() const
{
    Q_ASSERT(qobject_cast<QDeclarativeTransition*>(object()));
    return static_cast<QDeclarativeTransition*>(object());
}
}
}
