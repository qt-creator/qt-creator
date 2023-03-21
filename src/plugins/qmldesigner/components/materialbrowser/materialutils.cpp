// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "materialutils.h"

#include "abstractview.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

// Assigns given material to a 3D model.
// The assigned material is also inserted into material library if not already there.
// If given material is not valid, first existing material from material library is used,
// or if material library is empty, a new material is created.
// This function should be called only from inside a transaction, as it potentially does many
// changes to model.
void MaterialUtils::assignMaterialTo3dModel(AbstractView *view, const ModelNode &modelNode,
                                            const ModelNode &materialNode)
{
    QTC_ASSERT(modelNode.isValid() && modelNode.metaInfo().isQtQuick3DModel(), return);

    ModelNode matLib = view->materialLibraryNode();

    if (!matLib.isValid())
        return;

    ModelNode newMaterialNode;

    if (materialNode.isValid() && materialNode.metaInfo().isQtQuick3DMaterial()) {
        newMaterialNode = materialNode;
    } else {
        const QList<ModelNode> materials = matLib.directSubModelNodes();
        if (materials.size() > 0) {
            for (const ModelNode &mat : materials) {
                if (mat.metaInfo().isQtQuick3DMaterial()) {
                    newMaterialNode = mat;
                    break;
                }
            }
        }

        // if no valid material, create a new default material
        if (!newMaterialNode.isValid()) {
            NodeMetaInfo metaInfo = view->model()->qtQuick3DPrincipledMaterialMetaInfo();
            newMaterialNode = view->createModelNode("QtQuick3D.PrincipledMaterial",
                                                    metaInfo.majorVersion(),
                                                    metaInfo.minorVersion());
            newMaterialNode.validId();
        }
    }

    QTC_ASSERT(newMaterialNode.isValid(), return);

    VariantProperty matNameProp = newMaterialNode.variantProperty("objectName");
    if (matNameProp.value().isNull())
        matNameProp.setValue("New Material");

    if (!newMaterialNode.hasParentProperty()
        || newMaterialNode.parentProperty() != matLib.defaultNodeListProperty()) {
        matLib.defaultNodeListProperty().reparentHere(newMaterialNode);
    }

    QmlObjectNode(modelNode).setBindingProperty("materials", newMaterialNode.id());
}

} // namespace QmlDesigner
