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

#include "qmltransitionnodeinstance.h"
#include <private/qdeclarativetransition_p.h>

namespace QmlDesigner {
namespace Internal {

QmlTransitionNodeInstance::QmlTransitionNodeInstance(QDeclarativeTransition *transition)
    : ObjectNodeInstance(transition)
{
}

QmlTransitionNodeInstance::Pointer QmlTransitionNodeInstance::create(QObject *object)
{
     QDeclarativeTransition *transition = qobject_cast<QDeclarativeTransition*>(object);

     Q_ASSERT(transition);

     Pointer instance(new QmlTransitionNodeInstance(transition));

     instance->populateResetHashes();

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
