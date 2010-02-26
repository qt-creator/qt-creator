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

#include "qmlstatenodeinstance.h"
#include <private/qdeclarativestategroup_p.h>

namespace QmlDesigner {
namespace Internal {

/**
  \class QmlStateNodeInstance

  QmlStateNodeInstance manages a QDeclarativeState object. One can activate / deactivate a state
  by setting/unsetting the special "__activateState" boolean property.
  */

QmlStateNodeInstance::QmlStateNodeInstance(QDeclarativeState *object) :
        ObjectNodeInstance(object)
{
}

QmlStateNodeInstance::Pointer
        QmlStateNodeInstance::create(const NodeMetaInfo &metaInfo,
                                               QDeclarativeContext *context,
                                               QObject *objectToBeWrapped)
{
    Q_ASSERT(!objectToBeWrapped);
    QObject *object = createObject(metaInfo, context);
    QDeclarativeState *stateObject = qobject_cast<QDeclarativeState*>(object);
    Q_ASSERT(stateObject);

    Pointer instance(new QmlStateNodeInstance(stateObject));

    instance->populateResetValueHash();

    return instance;
}

void QmlStateNodeInstance::activateState()
{
    if (stateGroup()) {
        if (!isStateActive())
            stateGroup()->setState(property("name").toString());
    }
}

void QmlStateNodeInstance::deactivateState()
{
    if (stateGroup()) {
        if (isStateActive())
            stateGroup()->setState(QString());
    }
}

void QmlStateNodeInstance::refreshState()
{
    nodeInstanceView()->setBlockStatePropertyChanges(true);
    deactivateState();
    activateState();
    nodeInstanceView()->setBlockStatePropertyChanges(false);
}

QDeclarativeState *QmlStateNodeInstance::stateObject() const
{
    Q_ASSERT(object());
    Q_ASSERT(qobject_cast<QDeclarativeState*>(object()));
    return static_cast<QDeclarativeState*>(object());
}

QDeclarativeStateGroup *QmlStateNodeInstance::stateGroup() const
{
    return stateObject()->stateGroup();
}

bool QmlStateNodeInstance::isStateActive() const
{
    Q_ASSERT(stateObject());
    Q_ASSERT(stateGroup());
    return (stateGroup()->state() == property("name"));
}

} // namespace Internal
} // namespace QmlDesigner
