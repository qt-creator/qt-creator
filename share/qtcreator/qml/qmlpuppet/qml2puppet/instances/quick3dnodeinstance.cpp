/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "quick3dnodeinstance.h"
#include "qt5nodeinstanceserver.h"

#include <qmlprivategate.h>

#include <QDebug>
#include <QHash>
#include <QQmlExpression>
#include <QQmlProperty>

#include <cmath>

#ifdef QUICK3D_MODULE
#include <private/qquick3dnode_p.h>
#include <private/qquick3dmodel_p.h>
#include <private/qquick3dnode_p_p.h>
#endif

namespace QmlDesigner {
namespace Internal {

Quick3DNodeInstance::Quick3DNodeInstance(QObject *node)
   : ObjectNodeInstance(node)
{
}

Quick3DNodeInstance::~Quick3DNodeInstance()
{
}

void Quick3DNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                     InstanceContainer::NodeFlags flags)
{
    ObjectNodeInstance::initialize(objectNodeInstance, flags);
}

Qt5NodeInstanceServer *Quick3DNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer *>(nodeInstanceServer());
}

QQuick3DNode *Quick3DNodeInstance::quick3DNode() const
{
#ifdef QUICK3D_MODULE
    return qobject_cast<QQuick3DNode *>(object());
#else
    return nullptr;
#endif
}

Quick3DNodeInstance::Pointer Quick3DNodeInstance::create(QObject *object)
{
    Pointer instance(new Quick3DNodeInstance(object));
    instance->populateResetHashes();
    return instance;
}

void Quick3DNodeInstance::setHiddenInEditor(bool b)
{
    ObjectNodeInstance::setHiddenInEditor(b);
#ifdef QUICK3D_MODULE
    QQuick3DNodePrivate *privateNode = QQuick3DNodePrivate::get(quick3DNode());
    if (privateNode)
        privateNode->setIsHiddenInEditor(b);
#endif
}

} // namespace Internal
} // namespace QmlDesigner

