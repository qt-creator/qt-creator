// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils3d.h"

#include <itemlibraryentry.h>
#include <modelutils.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <qmlitemnode.h>
#include <qmlobjectnode.h>
#include <variantproperty.h>

#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

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

ModelNode activeView3dNode(AbstractView *view)
{
    if (!view || !view->model())
        return {};

    ModelNode activeView3D;
    ModelNode activeScene = Utils3D::active3DSceneNode(view);

    if (activeScene.isValid()) {
        if (activeScene.metaInfo().isQtQuick3DView3D()) {
            activeView3D = activeScene;
        } else {
            ModelNode sceneParent = activeScene.parentProperty().parentModelNode();
            if (sceneParent.metaInfo().isQtQuick3DView3D())
                activeView3D = sceneParent;
        }
        return activeView3D;
    }

    return {};
}

QString activeView3dId(AbstractView *view)
{
    ModelNode activeView3D = activeView3dNode(view);

    if (activeView3D.isValid())
        return activeView3D.id();

    return {};
}

ModelNode getMaterialOfModel(const ModelNode &model, int idx)
{
    QTC_ASSERT(model.isValid(), return {});

    QmlObjectNode qmlObjNode(model);
    QString matExp = qmlObjNode.expression("materials");
    if (matExp.isEmpty())
        return {};

    const QStringList mats = matExp.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
    if (mats.isEmpty())
        return {};

    ModelNode mat = model.model()->modelNodeForId(mats.at(idx));

    QTC_CHECK(mat);

    return mat;
}

void selectMaterial(const ModelNode &material)
{
    if (material.metaInfo().isQtQuick3DMaterial()) {
        material.model()->rootModelNode().setAuxiliaryData(Utils3D::matLibSelectedMaterialProperty,
                                                           material.id());
    }
}

void selectTexture(const ModelNode &texture)
{
    if (texture.metaInfo().isQtQuick3DTexture()) {
        texture.model()->rootModelNode().setAuxiliaryData(Utils3D::matLibSelectedTextureProperty,
                                                          texture.id());
    }
}

ModelNode selectedMaterial(AbstractView *view)
{
    if (!view)
        return {};

    ModelNode root = view->rootModelNode();

    if (auto selectedProperty = root.auxiliaryData(Utils3D::matLibSelectedMaterialProperty))
        return view->modelNodeForId(selectedProperty->toString());
    return {};
}

ModelNode selectedTexture(AbstractView *view)
{
    if (!view)
        return {};

    ModelNode root = view->rootModelNode();
    if (auto selectedProperty = root.auxiliaryData(Utils3D::matLibSelectedTextureProperty))
        return view->modelNodeForId(selectedProperty->toString());
    return {};
}

QList<ModelNode> getSelectedModels(AbstractView *view)
{
    if (!view || !view->model())
        return {};

    return Utils::filtered(view->selectedModelNodes(), [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DModel();
    });
}

void applyMaterialToModels(AbstractView *view, const ModelNode &material,
                           const QList<ModelNode> &models, bool add)
{
    if (models.isEmpty() || !view)
        return;

    QTC_CHECK(material);

    view->executeInTransaction(__FUNCTION__, [&] {
        for (const ModelNode &node : std::as_const(models)) {
            QmlObjectNode qmlObjNode(node);
            if (add) {
                QStringList matList = ModelUtils::expressionToList(
                    qmlObjNode.expression("materials"));
                matList.append(material.id());
                QString updatedExp = ModelUtils::listToExpression(matList);
                qmlObjNode.setBindingProperty("materials", updatedExp);
            } else {
                qmlObjNode.setBindingProperty("materials", material.id());
            }
        }
    });
}

ModelNode resolveSceneEnv(AbstractView *view, int sceneId)
{
    ModelNode activeSceneEnv;
    ModelNode selectedNode = view->firstSelectedModelNode();

    if (selectedNode.metaInfo().isQtQuick3DSceneEnvironment()) {
        activeSceneEnv = selectedNode;
    } else if (sceneId != -1) {
        ModelNode activeScene = Utils3D::active3DSceneNode(view);
        if (activeScene.isValid()) {
            QmlObjectNode view3D;
            if (activeScene.metaInfo().isQtQuick3DView3D()) {
                view3D = activeScene;
            } else {
                ModelNode sceneParent = activeScene.parentProperty().parentModelNode();
                if (sceneParent.metaInfo().isQtQuick3DView3D())
                    view3D = sceneParent;
            }
            if (view3D.isValid())
                activeSceneEnv = view->modelNodeForId(view3D.expression("environment"));
        }
    }

    return activeSceneEnv;
}

void assignTextureAsLightProbe(AbstractView *view, const ModelNode &texture, int sceneId)
{
    ModelNode sceneEnvNode = resolveSceneEnv(view, sceneId);
    QmlObjectNode sceneEnv = sceneEnvNode;
    if (sceneEnv.isValid()) {
        sceneEnv.setBindingProperty("lightProbe", texture.id());
        sceneEnv.setVariantProperty("backgroundMode",
                                    QVariant::fromValue(Enumeration("SceneEnvironment", "SkyBox")));
    }
}

// This method should be executed within a transaction as it performs multiple modifications to the model
#ifdef QDS_USE_PROJECTSTORAGE
ModelNode createMaterial(AbstractView *view, const TypeName &typeName)
{
    ModelNode matLib = Utils3D::materialLibraryNode(view);
    if (!matLib.isValid() || !typeName.size())
        return {};

    ModelNode newMatNode = view->createModelNode(typeName, -1, -1);
    matLib.defaultNodeListProperty().reparentHere(newMatNode);

    static QRegularExpression rgx("([A-Z])([a-z]*)");
    QString newName = QString::fromUtf8(typeName.split('.').last()).replace(rgx, " \\1\\2").trimmed();
    if (newName.endsWith(" Material"))
        newName.chop(9); // remove trailing " Material"
    QString newId = view->model()->generateNewId(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    view->emitCustomNotification("focus_material_section", {});

    return newMatNode;
}
#else
ModelNode createMaterial(AbstractView *view, const NodeMetaInfo &metaInfo)
{
    ModelNode matLib = Utils3D::materialLibraryNode(view);
    if (!matLib.isValid() || !metaInfo.isValid())
        return {};

    ModelNode newMatNode = view->createModelNode(metaInfo.typeName(),
                                                 metaInfo.majorVersion(),
                                                 metaInfo.minorVersion());
    matLib.defaultNodeListProperty().reparentHere(newMatNode);

    static QRegularExpression rgx("([A-Z])([a-z]*)");
    QString newName = QString::fromLatin1(metaInfo.simplifiedTypeName()).replace(rgx, " \\1\\2").trimmed();
    if (newName.endsWith(" Material"))
        newName.chop(9); // remove trailing " Material"
    QString newId = view->model()->generateNewId(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    view->emitCustomNotification("focus_material_section", {});

    return newMatNode;
}
#endif

void addQuick3DImportAndView3D(AbstractView *view)
{
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (!view || !view->model() || !document || document->inFileComponentModelActive()) {
        Core::AsynchronousMessageBox::warning(Tr::tr("Failed to Add Import"),
                                              Tr::tr("Could not add QtQuick3D import to the document."));
        return;
    }

    QString importName{"QtQuick3D"};
    if (view->model()->hasImport(importName))
        return;

    view->executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(importName);
        view->model()->changeImports({import}, {});

        if (!view->rootModelNode().metaInfo().isQtQuickItem())
            return;

        ensureMaterialLibraryNode(view);
#ifndef QDS_USE_PROJECTSTORAGE
    });
    view->executeInTransaction(__FUNCTION__, [&] {
#endif
        NodeMetaInfo view3dInfo = view->model()->qtQuick3DView3DMetaInfo();
        if (!view->allModelNodesOfType(view3dInfo).isEmpty())
            return;

        const QList<ItemLibraryEntry> entries = view->model()->itemLibraryEntries();
        // Use template file to identify correct entry, as name could be localized in the future
        const QString view3dSource{"extendedview3D_template.qml"};
        auto templateMatch = [&view3dSource](const ItemLibraryEntry &entry) -> bool {
            return entry.templatePath().endsWith(view3dSource);
        };
        auto iter = std::ranges::find_if(entries, templateMatch);
        if (iter == entries.end())
            return;

        NodeAbstractProperty targetProp = view->rootModelNode().defaultNodeAbstractProperty();
        QmlObjectNode newQmlObjectNode = QmlItemNode::createQmlObjectNode(
            view, *iter, QPointF(), targetProp, false);

        const QList<ModelNode> models = newQmlObjectNode.modelNode().subModelNodesOfType(
            view->model()->qtQuick3DModelMetaInfo());
        if (!models.isEmpty())
            assignMaterialTo3dModel(view, models.at(0));
    });
}

// Assigns given material to a 3D model.
// The assigned material is also inserted into material library if not already there.
// If given material is not valid, first existing material from material library is used,
// or if material library is empty, a new material is created.
// This function should be called only from inside a transaction, as it potentially does many
// changes to model.
void assignMaterialTo3dModel(AbstractView *view, const ModelNode &modelNode,
                             const ModelNode &materialNode)
{
    QTC_ASSERT(modelNode.metaInfo().isQtQuick3DModel(), return);

    ModelNode matLib = Utils3D::materialLibraryNode(view);

    if (!matLib)
        return;

    ModelNode newMaterialNode;

    if (materialNode.metaInfo().isQtQuick3DMaterial()) {
        newMaterialNode = materialNode;
    } else {
        const QList<ModelNode> materials = matLib.directSubModelNodes();
        auto isMaterial = [](const ModelNode &node) -> bool {
            return node.metaInfo().isQtQuick3DMaterial();
        };
        if (auto iter = std::ranges::find_if(materials, isMaterial); iter != materials.end())
            newMaterialNode = *iter;

        // if no valid material, create a new default material
        if (!newMaterialNode) {
#ifdef QDS_USE_PROJECTSTORAGE
            newMaterialNode = view->createModelNode("PrincipledMaterial");
#else
            NodeMetaInfo metaInfo = view->model()->qtQuick3DPrincipledMaterialMetaInfo();
            newMaterialNode = view->createModelNode("QtQuick3D.PrincipledMaterial",
                                                    metaInfo.majorVersion(),
                                                    metaInfo.minorVersion());
#endif
            newMaterialNode.ensureIdExists();
        }
    }

    QTC_ASSERT(newMaterialNode, return);

    VariantProperty matNameProp = newMaterialNode.variantProperty("objectName");
    if (matNameProp.value().isNull())
        matNameProp.setValue("New Material");

    if (!newMaterialNode.hasParentProperty()
        || newMaterialNode.parentProperty() != matLib.defaultNodeListProperty()) {
        matLib.defaultNodeListProperty().reparentHere(newMaterialNode);
    }

    QmlObjectNode(modelNode).setBindingProperty("materials", newMaterialNode.id());
}

} // namespace Utils3D
} // namespace QmlDesigner
