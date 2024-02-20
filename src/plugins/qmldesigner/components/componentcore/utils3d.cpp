// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils3d.h"

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <variantproperty.h>

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

ModelNode materialLibraryNode(AbstractView *view)
{
    return view->modelNodeForId(Constants::MATERIAL_LIB_ID);
}

// Creates material library if it doesn't exist and moves any existing materials into it.
void ensureMaterialLibraryNode(AbstractView *view)
{
    ModelNode matLib = view->modelNodeForId(Constants::MATERIAL_LIB_ID);
    if (matLib.isValid()
        || (!view->rootModelNode().metaInfo().isQtQuick3DNode()
            && !view->rootModelNode().metaInfo().isQtQuickItem())) {
        return;
    }

    view->executeInTransaction(__FUNCTION__, [&] {
    // Create material library node
#ifdef QDS_USE_PROJECTSTORAGE
        TypeName nodeTypeName = view->rootModelNode().metaInfo().isQtQuick3DNode() ? "Node" : "Item";
        matLib = view->createModelNode(nodeTypeName, -1, -1);
#else
            auto nodeType = view->rootModelNode().metaInfo().isQtQuick3DNode()
                                ? view->model()->qtQuick3DNodeMetaInfo()
                                : view->model()->qtQuickItemMetaInfo();
            matLib = view->createModelNode(nodeType.typeName(), nodeType.majorVersion(), nodeType.minorVersion());
#endif
        matLib.setIdWithoutRefactoring(Constants::MATERIAL_LIB_ID);
        view->rootModelNode().defaultNodeListProperty().reparentHere(matLib);
    });

    // Do the material reparentings in different transaction to work around issue QDS-8094
    view->executeInTransaction(__FUNCTION__, [&] {
        const QList<ModelNode> materials = view->rootModelNode().subModelNodesOfType(
            view->model()->qtQuick3DMaterialMetaInfo());
        if (!materials.isEmpty()) {
            // Move all materials to under material library node
            for (const ModelNode &node : materials) {
                // If material has no name, set name to id
                QString matName = node.variantProperty("objectName").value().toString();
                if (matName.isEmpty()) {
                    VariantProperty objNameProp = node.variantProperty("objectName");
                    objNameProp.setValue(node.id());
                }

                matLib.defaultNodeListProperty().reparentHere(node);
            }
        }
    });
}

bool isPartOfMaterialLibrary(const ModelNode &node)
{
    if (!node.isValid())
        return false;

    ModelNode matLib = materialLibraryNode(node.view());

    return matLib.isValid()
           && (node == matLib
               || (node.hasParentProperty() && node.parentProperty().parentModelNode() == matLib));
}

ModelNode getTextureDefaultInstance(const QString &source, AbstractView *view)
{
    ModelNode matLib = materialLibraryNode(view);
    if (!matLib.isValid())
        return {};

    const QList<ModelNode> matLibNodes = matLib.directSubModelNodes();
    for (const ModelNode &tex : matLibNodes) {
        if (tex.isValid() && tex.metaInfo().isQtQuick3DTexture()) {
            const QList<AbstractProperty> props = tex.properties();
            if (props.size() != 1)
                continue;
            const AbstractProperty &prop = props[0];
            if (prop.name() == "source" && prop.isVariantProperty()
                && prop.toVariantProperty().value().toString() == source) {
                return tex;
            }
        }
    }

    return {};
}

} // namespace Utils3D
} // namespace QmlDesigner
