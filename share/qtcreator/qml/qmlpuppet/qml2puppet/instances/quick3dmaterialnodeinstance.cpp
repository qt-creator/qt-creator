// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "quick3dmaterialnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

Quick3DMaterialNodeInstance::Quick3DMaterialNodeInstance(QObject *node)
   : Quick3DRenderableNodeInstance(node)
{
}

Quick3DMaterialNodeInstance::~Quick3DMaterialNodeInstance()
{
}

void Quick3DMaterialNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                             InstanceContainer::NodeFlags flags)
{
    m_dummyRootViewCreateFunction = "createViewForMaterial";

    Quick3DRenderableNodeInstance::initialize(objectNodeInstance, flags);
}

Quick3DMaterialNodeInstance::Pointer Quick3DMaterialNodeInstance::create(QObject *object)
{
    Pointer instance(new Quick3DMaterialNodeInstance(object));
    instance->populateResetHashes();
    return instance;
}

} // namespace Internal
} // namespace QmlDesigner

