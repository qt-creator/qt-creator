// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick3dnodeinstance.h"
#include "qt5nodeinstanceserver.h"
#include "qt5informationnodeinstanceserver.h"

#ifdef QUICK3D_MODULE
#include <private/qquick3dobject_p.h>
#include <private/qquick3dnode_p.h>
#include <private/qquick3dnode_p_p.h>
#include <private/qquick3drepeater_p.h>
#include <private/qquick3dloader_p.h>
#if defined(QUICK3D_ASSET_UTILS_MODULE)
#include <private/qquick3druntimeloader_p.h>
#endif
#endif

namespace QmlDesigner {
namespace Internal {

Quick3DNodeInstance::Quick3DNodeInstance(QObject *node)
   : Quick3DRenderableNodeInstance(node)
{
}

void Quick3DNodeInstance::invokeDummyViewCreate() const
{
    QMetaObject::invokeMethod(m_dummyRootView, "createViewForNode",
                              Q_ARG(QVariant, QVariant::fromValue(object())));
}

Quick3DNodeInstance::~Quick3DNodeInstance()
{
}

void Quick3DNodeInstance::initialize(
    [[maybe_unused]] const ObjectNodeInstance::Pointer &objectNodeInstance,
    [[maybe_unused]] InstanceContainer::NodeFlags flags)
{
#ifdef QUICK3D_MODULE
    QObject *obj = object();
    auto repObj = qobject_cast<QQuick3DRepeater *>(obj);
    auto loadObj = qobject_cast<QQuick3DLoader *>(obj);
#if defined(QUICK3D_ASSET_UTILS_MODULE)
    auto runLoadObj = qobject_cast<QQuick3DRuntimeLoader *>(obj);
    if (repObj || loadObj || runLoadObj) {
#else
    if (repObj || loadObj) {
#endif
        if (auto infoServer = qobject_cast<Qt5InformationNodeInstanceServer *>(nodeInstanceServer())) {
            if (repObj) {
                QObject::connect(repObj, &QQuick3DRepeater::objectAdded,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
#if defined(QUICK3D_ASSET_UTILS_MODULE)
            } else if (runLoadObj) {
                QObject::connect(runLoadObj, &QQuick3DRuntimeLoader::statusChanged,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
#endif
            } else {
                QObject::connect(loadObj, &QQuick3DLoader::loaded,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
            }
        }
    }

    Quick3DRenderableNodeInstance::initialize(objectNodeInstance, flags);
#endif // QUICK3D_MODULE
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

