// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils3d.h"

namespace QmlDesigner {
namespace Utils3D {

ModelNode active3DSceneNode(AbstractView *view)
{
    if (!view)
        return {};

    auto activeSceneAux = view->rootModelNode().auxiliaryData(active3dSceneProperty);
    if (activeSceneAux) {
        int activeScene = activeSceneAux->toInt();

        if (view->hasModelNodeForInternalId(activeScene))
            return view->modelNodeForInternalId(activeScene);
    }

    return {};
}

qint32 active3DSceneId(Model *model)
{
    auto sceneId = model->rootModelNode().auxiliaryData(active3dSceneProperty);
    if (sceneId)
        return sceneId->toInt();
    return -1;
}

} // namespace Utils3D
} // namespace QmlDesigner
