// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils3d.h"
#include "componentcoretracing.h"

#include <auxiliarydataproperties.h>
#include <designmodewidget.h>
#include <indentingtexteditormodifier.h>
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
#include <rewriterview.h>
#include <uniquename.h>
#include <variantproperty.h>

#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QTextDocument>

namespace QmlDesigner {
namespace Utils3D {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

namespace {

std::tuple<QString, Storage::ModuleKind> nodeModuleInfo(const ModelNode &node, Model *model)
{
    NanotraceHR::Tracer tracer{"utils 3d node module info", category(), keyValue("node", node)};

    using Storage::Info::ExportedTypeName;
    using Storage::Module;
    using Storage::ModuleKind;

    ExportedTypeName exportedName = node.exportedTypeName();
    if (exportedName.moduleId) {
        Module module = model->projectStorageDependencies().modulesStorage.module(
            exportedName.moduleId);
        return {module.name.toQString(), module.kind};
    }
    return {QString{}, ModuleKind::QmlLibrary};
}

} // namespace

ModelNode active3DSceneNode(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d active 3d scene node", category()};

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
    NanotraceHR::Tracer tracer{"utils 3d active 3d scene id", category()};

    auto sceneId = model->rootModelNode().auxiliaryData(active3dSceneProperty);
    if (sceneId)
        return sceneId->toInt();
    return -1;
}

ModelNode materialLibraryNode(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d material library node", category()};

    return view->modelNodeForId(Constants::MATERIAL_LIB_ID);
}

// Creates material library if it doesn't exist and moves any existing materials and textures into it.
void ensureMaterialLibraryNode(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d ensure material library node", category()};

    ModelNode matLib = view->modelNodeForId(Constants::MATERIAL_LIB_ID);
    if (matLib.isValid()
        || (!view->rootModelNode().metaInfo().isQtQuick3DNode()
            && !view->rootModelNode().metaInfo().isQtQuickItem())) {
        return;
    }

    bool resetPuppet = false;

    view->executeInTransaction(__FUNCTION__, [&] {
    // Create material library node
        TypeName nodeTypeName = view->rootModelNode().metaInfo().isQtQuick3DNode() ? "Node" : "Item";
        matLib = view->createModelNode(nodeTypeName);
        matLib.setIdWithoutRefactoring(Constants::MATERIAL_LIB_ID);
        view->rootModelNode().defaultNodeListProperty().reparentHere(matLib);
    });

    // Do the material reparentings in different transaction to work around issue QDS-8094
    view->executeInTransaction(__FUNCTION__, [&] {
        const NodeMetaInfo matInfo = view->model()->qtQuick3DMaterialMetaInfo();
        const NodeMetaInfo texInfo = view->model()->qtQuick3DTextureMetaInfo();
        const QList<ModelNode> materialsAndTextures
            = Utils::filtered(view->rootModelNode().allSubModelNodes(), [&](const ModelNode &node) {
            return node.metaInfo().isBasedOn(matInfo, texInfo);
        });

        if (!materialsAndTextures.isEmpty()) {
            resetPuppet = true;
            // Move all matching nodes to under material library node
            for (const ModelNode &node : materialsAndTextures) {
                // If node has no name, set name to id
                QString name = node.variantProperty("objectName").value().toString();
                if (name.isEmpty()) {
                    VariantProperty objNameProp = node.variantProperty("objectName");
                    objNameProp.setValue(node.id());
                }

                matLib.defaultNodeListProperty().reparentHere(node);
            }
        }
    });

    // Reparenting multiple materials/textures at once can cause rendering issues in
    // quick3d, so reset puppet
    if (resetPuppet)
        view->resetPuppet();
}

bool isPartOfMaterialLibrary(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"utils 3d is part of material library",
                               category(),
                               keyValue("node", node)};

    if (!node.isValid())
        return false;

    ModelNode matLib = materialLibraryNode(node.view());

    return matLib.isValid()
           && (node == matLib
               || (node.hasParentProperty() && node.parentProperty().parentModelNode() == matLib));
}

QHash<ModelNode, QSet<ModelNode>> allBoundMaterialsAndTextures(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"utils 3d all bound materials and textures",
                               category(),
                               keyValue("node", node)};

    auto model = node.model();
    const QList<ModelNode> allNodes = node.allSubModelNodesAndThisNode();

    auto matInfo = model->qtQuick3DMaterialMetaInfo();
    auto texInfo = model->qtQuick3DTextureMetaInfo();

    QSet<ModelNode> matTexNodes;
    for (const auto &checkNode : allNodes) {
        const QList<BindingProperty> bindProps = checkNode.bindingProperties();
        for (const auto &bindProp : bindProps) {
            const QList<ModelNode> boundNodes = bindProp.resolveToModelNodes();
            for (const auto &boundNode : boundNodes) {
                if (boundNode.metaInfo().isBasedOn(matInfo, texInfo))
                    matTexNodes.insert(boundNode);
            }
        }
    }

    QHash<ModelNode, QSet<ModelNode>> finalHash;
    for (const auto &boundNode : std::as_const(matTexNodes)) {
        QSet<ModelNode> boundTex;
        if (boundNode.metaInfo().isBasedOn(matInfo)) {
            const QList<BindingProperty> bindProps = boundNode.bindingProperties();
            for (const auto &prop : bindProps) {
                ModelNode boundNode = prop.resolveToModelNode();
                if (boundNode.metaInfo().isBasedOn(texInfo))
                    boundTex.insert(boundNode);
            }
        }
        finalHash.insert(boundNode, boundTex);
    }

    return finalHash;
}

void createMatLibForFile(const QString &fileName,
                         const QHash<ModelNode, QSet<ModelNode>> &matAndTexNodes,
                         AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d create mat lib for file",
                               category(),
                               keyValue("file name", fileName)};

    if (!view || !view->model() || fileName.isEmpty() || matAndTexNodes.isEmpty())
        return;

    Utils::FilePath file = Utils::FilePath::fromString(fileName);
    auto fileContents = file.fileContents();
    if (!fileContents.has_value())
        return;

    const QString qmlStr = QString::fromUtf8(fileContents.value());
    QString cleanQmlStr = qmlStr;
    static QRegularExpression cppCommentsRE(R"(//[^\n]*)", QRegularExpression::MultilineOption);
    static QRegularExpression cCommentsRE(R"(/\*.*?\*/)", QRegularExpression::DotMatchesEverythingOption);
    cleanQmlStr.remove(cppCommentsRE);
    cleanQmlStr.remove(cCommentsRE);

    QSet<ModelNode> allNodes;
    const QList<ModelNode> matNodes = matAndTexNodes.keys();
    for (const auto &matNode : matNodes) {
        allNodes.insert(matNode);
        const QSet<ModelNode> texNodes = matAndTexNodes[matNode];
        for (const auto &texNode : texNodes)
            allNodes.insert(texNode);
    }

    // Find all nodes for which there are direct bindings
    QList<ModelNode> directlyUsedNodes;
    QString regExTemplate(R"(:.*\b%1\b.*\n)");
    for (const auto &checkNode : allNodes) {
        QRegularExpression re(regExTemplate.arg(checkNode.id()));
        if (re.match(cleanQmlStr).hasMatch())
            directlyUsedNodes.append(checkNode);
    }

    QSet<ModelNode> matLibNodes;
    for (const auto &usedNode : std::as_const(directlyUsedNodes)) {
        matLibNodes.insert(usedNode);
        const QSet<ModelNode> texNodes = matAndTexNodes.value(usedNode);
        matLibNodes.unite(texNodes);
    }

    if (matLibNodes.isEmpty())
        return;

    ModelPointer fileModel = view->model()->createModel({"Item"});

    fileModel->setFileUrl(QUrl::fromUserInput(fileName));
    QTextDocument textDoc(qmlStr);
    IndentingTextEditModifier modifier(&textDoc);
    RewriterView rewriterView(view->externalDependencies(),
                              view->model()->projectStorageDependencies().modulesStorage,
                              RewriterView::Amend);
    rewriterView.setTextModifier(&modifier);
    fileModel->setRewriterView(&rewriterView);

    using Storage::ModuleKind;

    QHash<QString, ModuleKind> moduleInfos;
    for (const auto &oldNode : std::as_const(matLibNodes)) {
        auto [moduleName, moduleKind] = nodeModuleInfo(oldNode, view->model());
        if (!moduleName.isEmpty())
            moduleInfos.insert(moduleName, moduleKind);
    }

    Imports imports;
    for (const auto &[path, kind] : moduleInfos.asKeyValueRange()) {
        if (kind == ModuleKind::PathLibrary) {
            imports.append(Import::createFileImport(
                Utils::FilePath::fromString(path).relativePathFromDir(file.parentDir()).path()));
        } else {
            imports.append(Import::createLibraryImport(path));
        }
    }

    rewriterView.model()->changeImports(imports, {});

    ensureMaterialLibraryNode(&rewriterView);
    ModelNode matLib = materialLibraryNode(&rewriterView);
    for (const auto &oldNode : std::as_const(matLibNodes)) {
        if (fileModel->hasId(oldNode.id()))
            continue;

        const QList<VariantProperty> oldNodeVarProps = oldNode.variantProperties();
        const QList<BindingProperty> oldNodeBindProps = oldNode.bindingProperties();

        ModelNode newNode = fileModel->createModelNode(oldNode.documentTypeRepresentation());

        for (const VariantProperty &prop : oldNodeVarProps)
            newNode.variantProperty(prop.name()).setValue(prop.value());
        for (const BindingProperty &prop : oldNodeBindProps)
            newNode.bindingProperty(prop.name()).setExpression(prop.expression());
        newNode.setIdWithoutRefactoring(oldNode.id());
        matLib.defaultNodeAbstractProperty().reparentHere(newNode);
    }

    rewriterView.forceAmend();

    QString newText = modifier.text();

    if (newText != qmlStr && !file.writeFileContents(newText.toUtf8()))
        qWarning() << __FUNCTION__ << "Failed to save changes to:" << fileName;
}

ModelNode getTextureDefaultInstance(const QString &source, AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d get texture default instance", category()};

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
    NanotraceHR::Tracer tracer{"utils 3d active view 3d node", category()};

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
    NanotraceHR::Tracer tracer{"utils 3d active view 3d id", category()};

    ModelNode activeView3D = activeView3dNode(view);

    if (activeView3D.isValid())
        return activeView3D.id();

    return {};
}

ModelNode getMaterialOfModel(const ModelNode &model, int idx)
{
    NanotraceHR::Tracer tracer{"utils 3d get material of model",
                               category(),
                               keyValue("node", model),
                               keyValue("index", idx)};

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

QList<ModelNode> getSelectedModels(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d get selected models", category()};

    if (!view || !view->model())
        return {};

    return Utils::filtered(view->selectedModelNodes(), [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DModel();
    });
}

QList<ModelNode> getSelectedTextures(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d get selected textures", category()};

    if (!view || !view->model())
        return {};

    return Utils::filtered(view->selectedModelNodes(), [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DTexture();
    });
}

QList<ModelNode> getSelectedMaterials(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d get selected materials", category()};

    if (!view || !view->model())
        return {};

    return Utils::filtered(view->selectedModelNodes(), [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DMaterial();
    });
}

void applyMaterialToModels(AbstractView *view, const ModelNode &material,
                           const QList<ModelNode> &models, bool add)
{
    NanotraceHR::Tracer tracer{"utils 3d apply material to models", category()};

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
    NanotraceHR::Tracer tracer{"utils 3d resolve scene env", category()};

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
    NanotraceHR::Tracer tracer{"utils 3d assign texture as light probe",
                               category(),
                               keyValue("texture", texture),
                               keyValue("sceneId", sceneId)};

    ModelNode sceneEnvNode = resolveSceneEnv(view, sceneId);
    QmlObjectNode sceneEnv = sceneEnvNode;
    if (sceneEnv.isValid()) {
        sceneEnv.setBindingProperty("lightProbe", texture.id());
        sceneEnv.setVariantProperty("backgroundMode",
                                    QVariant::fromValue(Enumeration("SceneEnvironment", "SkyBox")));
    }
}

// This method should be executed within a transaction as it performs multiple modifications to the model
ModelNode createMaterial(AbstractView *view, const TypeName &typeName)
{
    NanotraceHR::Tracer tracer{"utils 3d create material", category(), keyValue("type name", typeName)};

    ModelNode matLib = Utils3D::materialLibraryNode(view);
    if (!matLib.isValid() || !typeName.size())
        return {};

    ModelNode newMatNode = view->createModelNode(typeName);
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

bool addQuick3DImportAndView3D(AbstractView *view, bool suppressWarningDialog)
{
    NanotraceHR::Tracer tracer{"utils 3d add quick 3d import and view 3d",
                               category(),
                               keyValue("suppress warning dialog", suppressWarningDialog)};

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (!view || !view->model() || !document || document->inFileComponentModelActive()) {
        if (!suppressWarningDialog) {
            Core::AsynchronousMessageBox::warning(
                Tr::tr("Failed to Add Import"),
                Tr::tr("Could not add QtQuick3D import to the document."));
        }
        return false;
    }

    QString importName{"QtQuick3D"};
    if (view->model()->hasImport(importName))
        return true;

    view->executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(importName);
        view->model()->changeImports({import}, {});

        if (!view->rootModelNode().metaInfo().isQtQuickItem())
            return;

        ensureMaterialLibraryNode(view);

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
        QmlObjectNode newQmlObjectNode = QmlItemNode::createQmlObjectNode(view,
                                                                          *iter,
                                                                          QPointF(),
                                                                          targetProp,
                                                                          false);

        const QList<ModelNode> models = newQmlObjectNode.modelNode().subModelNodesOfType(
            view->model()->qtQuick3DModelMetaInfo());
        if (!models.isEmpty())
            assignMaterialTo3dModel(view, models.at(0));
    });
    return true;
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
    NanotraceHR::Tracer tracer{"utils 3d assign material to 3d model",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("material node", materialNode)};

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
            newMaterialNode = view->createModelNode("PrincipledMaterial");

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

ModelNode createMaterial(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"utils 3d create material", category()};

    QTC_ASSERT(view && view->model(), return {});

    ModelNode newMatNode;
    view->executeInTransaction(__FUNCTION__, [&] {
        ModelNode matLib = materialLibraryNode(view);
        if (!matLib.isValid())
            return;

        newMatNode = view->createModelNode("PrincipledMaterial");
        QmlObjectNode(newMatNode).setNameAndId("New Material", "material");
        matLib.defaultNodeListProperty().reparentHere(newMatNode);

        QTimer::singleShot(0, view, [newMatNode]() mutable {
            newMatNode.selectNode();
        });
    });

    return newMatNode;
}

void renameMaterial(const ModelNode &material, const QString &newName)
{
    NanotraceHR::Tracer tracer{"utils 3d rename material",
                               category(),
                               keyValue("material", material),
                               keyValue("new name", newName)};

    QTC_ASSERT(material, return);
    QmlObjectNode(material).setNameAndId(newName, "material");
}

void duplicateMaterial(AbstractView *view, const ModelNode &material)
{
    NanotraceHR::Tracer tracer{"utils 3d duplicate material",
                               category(),
                               keyValue("material", material)};

    QTC_ASSERT(view && view->model() && material, return);

    TypeName matType = material.documentTypeRepresentation();
    QmlObjectNode sourceMat(material);
    ModelNode duplicateMatNode;
    QList<AbstractProperty> dynamicProps;

    view->executeInTransaction(__FUNCTION__, [&] {
        ModelNode matLib = Utils3D::materialLibraryNode(view);
        QTC_ASSERT(matLib.isValid(), return);

        // create the duplicate material
        QmlObjectNode duplicateMat = view->createModelNode(matType);

        duplicateMatNode = duplicateMat.modelNode();

        // generate and set a unique name
        QString newName = sourceMat.modelNode().variantProperty("objectName").value().toString();
        if (!newName.contains("copy", Qt::CaseInsensitive))
            newName.append(" copy");

        const QList<ModelNode> mats = matLib.directSubModelNodesOfType(
            view->model()->qtQuick3DMaterialMetaInfo());
        QStringList matNames;
        for (const ModelNode &mat : mats)
            matNames.append(mat.variantProperty("objectName").value().toString());

        newName = UniqueName::generate(newName,
                                       [&](const QString &name) { return matNames.contains(name); });

        VariantProperty objNameProp = duplicateMatNode.variantProperty("objectName");
        objNameProp.setValue(newName);

        // generate and set an id
        duplicateMatNode.setIdWithoutRefactoring(view->model()->generateNewId(newName, "material"));

        // sync properties. Only the base state is duplicated.
        const QList<AbstractProperty> props = material.properties();
        for (const AbstractProperty &prop : props) {
            if (prop.name() == "objectName" || prop.name() == "data")
                continue;

            if (prop.isVariantProperty()) {
                if (prop.isDynamic()) {
                    dynamicProps.append(prop);
                } else {
                    VariantProperty variantProp = duplicateMatNode.variantProperty(prop.name());
                    variantProp.setValue(prop.toVariantProperty().value());
                }
            } else if (prop.isBindingProperty()) {
                if (prop.isDynamic()) {
                    dynamicProps.append(prop);
                } else {
                    BindingProperty bindingProp = duplicateMatNode.bindingProperty(prop.name());
                    bindingProp.setExpression(prop.toBindingProperty().expression());
                }
            }
        }

        matLib.defaultNodeListProperty().reparentHere(duplicateMat);
        duplicateMat.modelNode().selectNode();
    });

    // For some reason, creating dynamic properties in the same transaction doesn't work, so
    // let's do it in separate transaction.
    // TODO: Fix the issue and merge transactions (QDS-8094)
    if (!dynamicProps.isEmpty()) {
        view->executeInTransaction(__FUNCTION__, [&] {
            for (const AbstractProperty &prop : std::as_const(dynamicProps)) {
                if (prop.isVariantProperty()) {
                    VariantProperty variantProp = duplicateMatNode.variantProperty(prop.name());
                    variantProp.setDynamicTypeNameAndValue(prop.dynamicTypeName(),
                                                           prop.toVariantProperty().value());
                } else if (prop.isBindingProperty()) {
                    BindingProperty bindingProp = duplicateMatNode.bindingProperty(prop.name());
                    bindingProp.setDynamicTypeNameAndExpression(prop.dynamicTypeName(),
                                                                prop.toBindingProperty().expression());
                }
            }
        });
    }
}

void openNodeInPropertyEditor(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"utils 3d open node in property editor",
                               category(),
                               keyValue("node", node)};

    using namespace Qt::StringLiterals;
    QTC_ASSERT(node, return);
    const auto mainWidget = QmlDesignerPlugin::instance()->mainWidget();
    AbstractView *mainPropertyEditor = QmlDesignerPlugin::instance()->viewManager().findView(
        "Properties"_L1);

    if (!mainPropertyEditor->isAttached())
        mainWidget->showDockWidget("Properties"_L1);
    mainWidget->viewManager().emitCustomNotification("set_property_editor_target_node", {node}, {});
}

bool hasImported3dType(AbstractView *view,
                       const AbstractView::ExportedTypeNames &added,
                       const AbstractView::ExportedTypeNames &removed)
{
    NanotraceHR::Tracer tracer{"utils 3d has imported 3d type", category()};

    QTC_ASSERT(view && view->model(), return false);

    using Storage::Info::ExportedTypeName;

    const QByteArray import3dPrefix = QmlDesignerPlugin::instance()->documentManager()
                                          .generatedComponentUtils().import3dTypePrefix().toUtf8();

    auto generatedModuleIds = view->model()->moduleIdsStartsWith(import3dPrefix,
                                                                 Storage::ModuleKind::QmlLibrary);
    std::ranges::sort(generatedModuleIds);

    return Utils::set_has_common_element(added, generatedModuleIds, {}, &ExportedTypeName::moduleId)
           || Utils::set_has_common_element(removed, generatedModuleIds, {}, &ExportedTypeName::moduleId);
}

void handle3DDrop(AbstractView *view, const ModelNode &dropNode)
{
    NanotraceHR::Tracer tracer{"utils 3d handle 3d drop", category(), keyValue("node", dropNode)};

    // If a View3D or 3D Model is dropped, we need to assign material to the model

    NodeMetaInfo model3DInfo = view->model()->qtQuick3DModelMetaInfo();

    if (dropNode.metaInfo().isBasedOn(model3DInfo)) {
        Utils3D::assignMaterialTo3dModel(view, dropNode);
        return;
    }

    NodeMetaInfo view3DInfo = view->model()->qtQuick3DView3DMetaInfo();

    if (dropNode.metaInfo().isBasedOn(view3DInfo)) {
        const QList<ModelNode> models = dropNode.subModelNodesOfType(model3DInfo);
        QTC_ASSERT(models.size() == 1, return);
        Utils3D::assignMaterialTo3dModel(view, models.at(0));

        // When the first View3D is added to the scene, we need to reset the puppet or
        // materials won't render correctly
        const ModelNodes allView3Ds = view->allModelNodesOfType(view3DInfo);
        if (allView3Ds.size() == 1)
            view->resetPuppet();
    }
}

} // namespace Utils3D
} // namespace QmlDesigner
