// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltransitionnodeinstance.h"

#include <qmlprivategate.h>

namespace QmlDesigner {
namespace Internal {

QmlTransitionNodeInstance::QmlTransitionNodeInstance(QObject *transition)
    : ObjectNodeInstance(transition)
{
}

QmlTransitionNodeInstance::Pointer QmlTransitionNodeInstance::create(QObject *object)
{
     Pointer instance(new QmlTransitionNodeInstance(object));

     instance->populateResetHashes();
     QmlPrivateGate::disableTransition(object);

     return instance;
}

bool QmlTransitionNodeInstance::isTransition() const
{
    return true;
}

PropertyNameList QmlTransitionNodeInstance::ignoredProperties() const
{
    static const PropertyNameList properties({"from", "to"});
    return properties;
}

}
}
