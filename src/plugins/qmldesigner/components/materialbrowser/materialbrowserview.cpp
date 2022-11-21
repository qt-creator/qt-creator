// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "materialbrowserview.h"

#include "bindingproperty.h"
#include "materialbrowserwidget.h"
#include "materialbrowsermodel.h"
#include "materialbrowsertexturesmodel.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <coreplugin/icore.h>
#include <designmodecontext.h>
#include <nodeinstanceview.h>
#include <nodemetainfo.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <utils/algorithm.h>

#include <QQuickItem>
#include <QRegularExpression>
#include <QTimer>

namespace QmlDesigner {

MaterialBrowserView::MaterialBrowserView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
    m_previewTimer.setSingleShot(true);
    connect(&m_previewTimer, &QTimer::timeout, this, &MaterialBrowserView::requestPreviews);
}

MaterialBrowserView::~MaterialBrowserView()
{}

bool MaterialBrowserView::hasWidget() const
{
    return true;
}

WidgetInfo MaterialBrowserView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new MaterialBrowserWidget(this);

        auto matEditorContext = new Internal::MaterialBrowserContext(m_widget.data());
        Core::ICore::addContextObject(matEditorContext);


        // custom notifications below are sent to the MaterialEditor
        MaterialBrowserModel *matBrowserModel = m_widget->materialBrowserModel().data();

        connect(matBrowserModel, &MaterialBrowserModel::selectedIndexChanged, this, [&] (int idx) {
            ModelNode matNode = m_widget->materialBrowserModel()->materialAt(idx);
            emitCustomNotification("selected_material_changed", {matNode}, {});
        });

        connect(matBrowserModel, &MaterialBrowserModel::applyToSelectedTriggered, this,
                [&] (const ModelNode &material, bool add) {
            emitCustomNotification("apply_to_selected_triggered", {material}, {add});
        });

        connect(matBrowserModel, &MaterialBrowserModel::renameMaterialTriggered, this,
                [&] (const ModelNode &material, const QString &newName) {
            emitCustomNotification("rename_material", {material}, {newName});
        });

        connect(matBrowserModel, &MaterialBrowserModel::addNewMaterialTriggered, this, [&] {
            emitCustomNotification("add_new_material");
        });

        connect(matBrowserModel, &MaterialBrowserModel::duplicateMaterialTriggered, this,
                [&] (const ModelNode &material) {
            emitCustomNotification("duplicate_material", {material});
        });

        connect(matBrowserModel, &MaterialBrowserModel::pasteMaterialPropertiesTriggered, this,
                [&] (const ModelNode &material,
                     const QList<QmlDesigner::MaterialBrowserModel::PropertyCopyData> &propDatas,
                     bool all) {
            QmlObjectNode mat(material);
            executeInTransaction(__FUNCTION__, [&] {
                if (all) { // all material properties copied
                    // remove current properties
                    PropertyNameList propNames;
                    if (mat.isInBaseState()) {
                        const QList<AbstractProperty> baseProps = material.properties();
                        for (const auto &baseProp : baseProps) {
                            if (!baseProp.isDynamic())
                                propNames.append(baseProp.name());
                        }
                    } else {
                        QmlPropertyChanges changes = mat.propertyChangeForCurrentState();
                        if (changes.isValid()) {
                            const QList<AbstractProperty> changedProps = changes.targetProperties();
                            for (const auto &changedProp : changedProps) {
                                if (!changedProp.isDynamic())
                                    propNames.append(changedProp.name());
                            }
                        }
                    }
                    for (const PropertyName &propName : std::as_const(propNames)) {
                        if (propName != "objectName" && propName != "data")
                            mat.removeProperty(propName);
                    }
                }

                // apply pasted properties
                for (const QmlDesigner::MaterialBrowserModel::PropertyCopyData &propData : propDatas) {
                    if (propData.isValid) {
                        const bool isDynamic = !propData.dynamicTypeName.isEmpty();
                        const bool isBaseState = currentState().isBaseState();
                        const bool hasProperty = mat.hasProperty(propData.name);
                        if (propData.isBinding) {
                            if (isDynamic && (!hasProperty || isBaseState)) {
                                mat.modelNode().bindingProperty(propData.name)
                                        .setDynamicTypeNameAndExpression(
                                            propData.dynamicTypeName, propData.value.toString());
                                continue;
                            }
                            mat.setBindingProperty(propData.name, propData.value.toString());
                        } else {
                            const bool isRecording = mat.timelineIsActive()
                                    && mat.currentTimeline().isRecording();
                            if (isDynamic && (!hasProperty || (isBaseState && !isRecording))) {
                                mat.modelNode().variantProperty(propData.name)
                                        .setDynamicTypeNameAndValue(
                                            propData.dynamicTypeName, propData.value);
                                continue;
                            }
                            mat.setVariantProperty(propData.name, propData.value);
                        }
                    } else {
                        mat.removeProperty(propData.name);
                    }
                }
            });
        });

        // custom notifications below are sent to the TextureEditor
        MaterialBrowserTexturesModel *texturesModel = m_widget->materialBrowserTexturesModel().data();
        connect(texturesModel, &MaterialBrowserTexturesModel::selectedIndexChanged, this, [&] (int idx) {
            ModelNode texNode = m_widget->materialBrowserTexturesModel()->textureAt(idx);
            emitCustomNotification("selected_texture_changed", {texNode}, {});
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::addNewTextureTriggered, this, [&] {
            emitCustomNotification("add_new_texture");
        });
    }

    return createWidgetInfo(m_widget.data(),
                            "MaterialBrowser",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Material Browser"));
}

void MaterialBrowserView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->materialBrowserModel()->setHasMaterialRoot(
        rootModelNode().metaInfo().isQtQuick3DMaterial());
    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    // Project load is already very busy and may even trigger puppet reset, so let's wait a moment
    // before refreshing the model
    QTimer::singleShot(1000, model, [this]() {
        refreshModel(true);
        loadPropertyGroups(); // Needs the delay because it uses metaInfo
    });
}

void MaterialBrowserView::refreshModel(bool updateImages)
{
    if (!model())
        return;

    ModelNode matLib = modelNodeForId(Constants::MATERIAL_LIB_ID);
    QList<ModelNode> materials;
    QList<ModelNode> textures;

    if (m_hasQuick3DImport && matLib.isValid()) {
        const QList <ModelNode> matLibNodes = matLib.directSubModelNodes();
        for (const ModelNode &node : matLibNodes) {
            if (isMaterial(node))
                materials.append(node);
            else if (isTexture(node))
                textures.append(node);
        }
    }

    m_widget->clearSearchFilter();
    m_widget->materialBrowserModel()->setMaterials(materials, m_hasQuick3DImport);
    m_widget->materialBrowserTexturesModel()->setTextures(textures);

    if (updateImages) {
        for (const ModelNode &node : std::as_const(materials))
            m_previewRequests.insert(node);
        if (!m_previewRequests.isEmpty())
            m_previewTimer.start(0);
    }
}

bool MaterialBrowserView::isMaterial(const ModelNode &node) const
{
    return node.metaInfo().isQtQuick3DMaterial();
}

bool MaterialBrowserView::isTexture(const ModelNode &node) const
{
    if (!node.isValid())
        return false;

    return node.metaInfo().isQtQuick3DTexture();
}

void MaterialBrowserView::modelAboutToBeDetached(Model *model)
{
    m_widget->materialBrowserModel()->setMaterials({}, m_hasQuick3DImport);
    m_widget->clearPreviewCache();

    if (m_propertyGroupsLoaded) {
        m_propertyGroupsLoaded = false;
        m_widget->materialBrowserModel()->unloadPropertyGroups();
    }

    AbstractView::modelAboutToBeDetached(model);
}

void MaterialBrowserView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                               [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    m_selectedModels = Utils::filtered(selectedNodeList, [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DModel();
    });

    m_widget->materialBrowserModel()->setHasModelSelection(!m_selectedModels.isEmpty());
    m_widget->materialBrowserTexturesModel()->setHasSingleModelSelection(m_selectedModels.size() == 1);

    // the logic below selects the material of the first selected model if auto selection is on
    if (!m_autoSelectModelMaterial)
        return;

    if (selectedNodeList.size() > 1 || m_selectedModels.isEmpty())
        return;

    QmlObjectNode qmlObjNode(m_selectedModels.at(0));
    QString matExp = qmlObjNode.expression("materials");
    if (matExp.isEmpty())
        return;

    QString matId = matExp.remove('[').remove(']').split(',', Qt::SkipEmptyParts).at(0);
    ModelNode mat = modelNodeForId(matId);
    if (!mat.isValid())
        return;

    // if selected object is a model, select its material in the material browser and editor
    int idx = m_widget->materialBrowserModel()->materialIndex(mat);
    m_widget->materialBrowserModel()->selectMaterial(idx);
}

void MaterialBrowserView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (isMaterial(node))
        m_widget->updateMaterialPreview(node, pixmap);
}

void MaterialBrowserView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                   [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (isMaterial(node) && property.name() == "objectName")
            m_widget->materialBrowserModel()->updateMaterialName(node);
    }
}

void MaterialBrowserView::nodeReparented(const ModelNode &node,
                                         const NodeAbstractProperty &newPropertyParent,
                                         const NodeAbstractProperty &oldPropertyParent,
                                         [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)

    if (!isMaterial(node) && !isTexture(node))
        return;

    ModelNode newParentNode = newPropertyParent.parentModelNode();
    ModelNode oldParentNode = oldPropertyParent.parentModelNode();
    bool added = newParentNode.id() == Constants::MATERIAL_LIB_ID;
    bool removed = oldParentNode.id() == Constants::MATERIAL_LIB_ID;

    if (!added && !removed)
        return;

    refreshModel(removed);

    if (isMaterial(node)) {
        if (added && !m_puppetResetPending) {
            // Workaround to fix various material issues all likely caused by QTBUG-103316
            resetPuppet();
            m_puppetResetPending = true;
        }
        int idx = m_widget->materialBrowserModel()->materialIndex(node);
        m_widget->materialBrowserModel()->selectMaterial(idx);
    } else { // is texture
        int idx = m_widget->materialBrowserTexturesModel()->textureIndex(node);
        m_widget->materialBrowserTexturesModel()->selectTexture(idx);
    }
}

void MaterialBrowserView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    // removing the material lib node
    if (removedNode.id() == Constants::MATERIAL_LIB_ID) {
        m_widget->materialBrowserModel()->setMaterials({}, m_hasQuick3DImport);
        m_widget->clearPreviewCache();
        return;
    }

    // not under the material lib
    if (removedNode.parentProperty().parentModelNode().id() != Constants::MATERIAL_LIB_ID)
        return;

    if (isMaterial(removedNode))
        m_widget->materialBrowserModel()->removeMaterial(removedNode);
    else if (isTexture(removedNode))
        m_widget->materialBrowserTexturesModel()->removeTexture(removedNode);
}

void MaterialBrowserView::nodeRemoved([[maybe_unused]] const ModelNode &removedNode,
                                      const NodeAbstractProperty &parentProperty,
                                      [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (parentProperty.parentModelNode().id() != Constants::MATERIAL_LIB_ID)
        return;

    m_widget->materialBrowserModel()->updateSelectedMaterial();
    m_widget->materialBrowserTexturesModel()->updateSelectedTexture();
}

void QmlDesigner::MaterialBrowserView::loadPropertyGroups()
{
    if (!m_hasQuick3DImport || m_propertyGroupsLoaded || !model())
        return;

    QString matPropsPath = model()->metaInfo("QtQuick3D.Material").importDirectoryPath()
                               + "/designer/propertyGroups.json";
    m_propertyGroupsLoaded = m_widget->materialBrowserModel()->loadPropertyGroups(matPropsPath);
}

void MaterialBrowserView::requestPreviews()
{
    if (model() && model()->nodeInstanceView()) {
        for (const auto &node : std::as_const(m_previewRequests))
            model()->nodeInstanceView()->previewImageDataForGenericNode(node, {});
    }
    m_previewRequests.clear();
}

void MaterialBrowserView::importsChanged([[maybe_unused]] const QList<Import> &addedImports,
                                         [[maybe_unused]] const QList<Import> &removedImports)
{
    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;

    loadPropertyGroups();

    // Import change will trigger puppet reset, so we don't want to update previews immediately
    refreshModel(false);
}

void MaterialBrowserView::customNotification(const AbstractView *view,
                                             const QString &identifier,
                                             const QList<ModelNode> &nodeList,
                                             [[maybe_unused]] const QList<QVariant> &data)
{
    if (view == this)
        return;

    if (identifier == "selected_material_changed") {
        int idx = m_widget->materialBrowserModel()->materialIndex(nodeList.first());
        if (idx != -1)
            m_widget->materialBrowserModel()->selectMaterial(idx);
    } else if (identifier == "refresh_material_browser") {
        QTimer::singleShot(0, model(), [this]() {
            refreshModel(true);
        });
    } else if (identifier == "delete_selected_material") {
        m_widget->materialBrowserModel()->deleteSelectedMaterial();
    }
}

void MaterialBrowserView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    for (const ModelNode &node : completedNodeList) {
        // We use root node completion as indication of puppet reset
        if (node.isRootNode()) {
            m_puppetResetPending  = false;
            QTimer::singleShot(1000, this, [this]() {
                if (!model() || !model()->nodeInstanceView())
                    return;
                const QList<ModelNode> materials = m_widget->materialBrowserModel()->materials();
                for (const ModelNode &node : materials)
                    m_previewRequests.insert(node);
                if (!m_previewRequests.isEmpty())
                    m_previewTimer.start(0);
            });
            break;
        }
    }
}

void MaterialBrowserView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    for (const auto &nodeProp : propertyList) {
        ModelNode node = nodeProp.first;
        if (node.metaInfo().isQtQuick3DMaterial())
            m_previewRequests.insert(node);
    }
    if (!m_previewRequests.isEmpty() && !m_previewTimer.isActive()) {
        // Updating material browser isn't urgent in e.g. timeline scrubbing case, so have a bit
        // of delay to reduce unnecessary rendering
        m_previewTimer.start(500);
    }
}

} // namespace QmlDesigner
