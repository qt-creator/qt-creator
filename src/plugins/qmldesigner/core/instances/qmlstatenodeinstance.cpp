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
#include <private/qmlstategroup_p.h>

namespace QmlDesigner {
namespace Internal {

/**
  \class QmlStateNodeInstance

  QmlStateNodeInstance manages a QmlState object. One can activate / deactivate a state
  by setting/unsetting the special "__activateState" boolean property.
  */

QmlStateNodeInstance::QmlStateNodeInstance(QmlState *object) :
        ObjectNodeInstance(object)
{
}

QmlStateNodeInstance::Pointer
        QmlStateNodeInstance::create(const NodeMetaInfo &metaInfo,
                                               QmlContext *context,
                                               QObject *objectToBeWrapped)
{
    Q_ASSERT(!objectToBeWrapped);
    QObject *object = createObject(metaInfo, context);
    QmlState *stateObject = qobject_cast<QmlState*>(object);
    Q_ASSERT(stateObject);

    Pointer instance(new QmlStateNodeInstance(stateObject));

    instance->populateResetValueHash();

    return instance;
}

void QmlStateNodeInstance::activateState()
{
    if (!isStateActive())
        stateGroup()->setState(property("name").toString());
}

void QmlStateNodeInstance::deactivateState()
{
    if (isStateActive())
         stateGroup()->setState(QString());
}

void QmlStateNodeInstance::refreshState()
{
    deactivateState();
    activateState();
}

QmlState *QmlStateNodeInstance::stateObject() const
{
    Q_ASSERT(object());
    Q_ASSERT(qobject_cast<QmlState*>(object()));
    return static_cast<QmlState*>(object());
}

QmlStateGroup *QmlStateNodeInstance::stateGroup() const
{
    return stateObject()->stateGroup();
}

bool QmlStateNodeInstance::isStateActive() const
{
    return (stateGroup()->state() == property("name"));
}

} // namespace Internal
} // namespace QmlDesigner
