// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowserview.h"

#include "bindingproperty.h"
#include "createtexture.h"
#include "designmodecontext.h"
#include "externaldependenciesinterface.h"
#include "materialbrowsermodel.h"
#include "materialbrowsertexturesmodel.h"
#include "materialbrowserwidget.h"
#include "nodeabstractproperty.h"
#include "nodeinstanceview.h"
#include "nodemetainfo.h"
#include "qmldesignerconstants.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

MaterialBrowserView::MaterialBrowserView(AsynchronousImageCache &imageCache,
                                         ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_imageCache(imageCache)
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
        m_widget = new MaterialBrowserWidget(m_imageCache, this);

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
            emitCustomNotification("selected_texture_changed", {texNode});
        });
        connect(texturesModel, &MaterialBrowserTexturesModel::duplicateTextureTriggered, this,
                [&] (const ModelNode &texture) {
            emitCustomNotification("duplicate_texture", {texture});
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::applyToSelectedMaterialTriggered, this,
                [&] (const ModelNode &texture) {
            if (!m_widget)
                return;
            const ModelNode material = m_widget->materialBrowserModel()->selectedMaterial();
            applyTextureToMaterial({material}, texture);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::applyToSelectedModelTriggered, this,
                [&] (const ModelNode &texture) {
            if (m_selectedModels.size() != 1)
                return;
            applyTextureToModel3D(m_selectedModels[0], texture);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::addNewTextureTriggered, this, [&] {
            emitCustomNotification("add_new_texture");
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::updateSceneEnvStateRequested, this, [&]() {
            ModelNode activeSceneEnv = CreateTexture(this).resolveSceneEnv(m_sceneId);
            const bool sceneEnvExists = activeSceneEnv.isValid();
            m_widget->materialBrowserTexturesModel()->setHasSceneEnv(sceneEnvExists);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::updateModelSelectionStateRequested, this, [&]() {
            bool hasModel = false;
            if (m_selectedModels.size() == 1)
                hasModel = getMaterialOfModel(m_selectedModels.at(0)).isValid();
            m_widget->materialBrowserTexturesModel()->setHasSingleModelSelection(hasModel);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::applyAsLightProbeRequested, this,
                [&] (const ModelNode &texture) {
            executeInTransaction(__FUNCTION__, [&] {
                CreateTexture(this).assignTextureAsLightProbe(texture, m_sceneId);
            });
        });
    }

    return createWidgetInfo(m_widget.data(),
                            "MaterialBrowser",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Material Browser"),
                            tr("Material Browser view"));
}

void MaterialBrowserView::createTextures(const QStringList &assetPaths)
{
    auto *create = new CreateTextures(this);

    executeInTransaction("MaterialBrowserView::createTextures", [&]() {
        create->execute(assetPaths, AddTextureMode::Texture, m_sceneId);
    });

    create->deleteLater();
}

void MaterialBrowserView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->materialBrowserModel()->setHasMaterialLibrary(false);
    m_hasQuick3DImport = model->hasImport("QtQuick3D");
    m_widget->materialBrowserModel()->setIsQt6Project(externalDependencies().isQt6Project());

    // Project load is already very busy and may even trigger puppet reset, so let's wait a moment
    // before refreshing the model
    QTimer::singleShot(1000, model, [this]() {
        refreshModel(true);
        loadPropertyGroups(); // Needs the delay because it uses metaInfo
    });

    m_sceneId = model->active3DSceneId();
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

    m_widget->materialBrowserModel()->setMaterials(materials, m_hasQuick3DImport);
    m_widget->materialBrowserTexturesModel()->setTextures(textures);
    m_widget->materialBrowserModel()->setHasMaterialLibrary(matLib.isValid());

    if (updateImages)
        updateMaterialsPreview();
}

void MaterialBrowserView::updateMaterialsPreview()
{
    const QList<ModelNode> materials = m_widget->materialBrowserModel()->materials();
    for (const ModelNode &node : materials)
        m_previewRequests.insert(node);
    if (!m_previewRequests.isEmpty())
        m_previewTimer.start(0);
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
    m_widget->materialBrowserModel()->setHasMaterialLibrary(false);
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

    // the logic below selects the material of the first selected model if auto selection is on
    if (!m_autoSelectModelMaterial)
        return;

    if (selectedNodeList.size() > 1 || m_selectedModels.isEmpty())
        return;

    ModelNode mat = getMaterialOfModel(m_selectedModels.at(0));

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

void MaterialBrowserView::nodeIdChanged(const ModelNode &node, [[maybe_unused]] const QString &newId,
                                                               [[maybe_unused]] const QString &oldId)
{
    if (isTexture(node))
        m_widget->materialBrowserTexturesModel()->updateTextureSource(node);
}

void MaterialBrowserView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                   [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());
        if (isMaterial(node) && property.name() == "objectName") {
            m_widget->materialBrowserModel()->updateMaterialName(node);
        } else if (property.name() == "source") {
            QmlObjectNode selectedTex = m_widget->materialBrowserTexturesModel()->selectedTexture();
            if (isTexture(node))
                m_widget->materialBrowserTexturesModel()->updateTextureSource(node);
            else if (selectedTex.propertyChangeForCurrentState() == node)
                m_widget->materialBrowserTexturesModel()->updateTextureSource(selectedTex);
        }
    }
}

void MaterialBrowserView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &prop : propertyList) {
        if (isTexture(prop.parentModelNode()) && prop.name() == "source")
            m_widget->materialBrowserTexturesModel()->updateTextureSource(prop.parentModelNode());
    }
}

void MaterialBrowserView::nodeReparented(const ModelNode &node,
                                         const NodeAbstractProperty &newPropertyParent,
                                         const NodeAbstractProperty &oldPropertyParent,
                                         [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)

    if (node.id() == Constants::MATERIAL_LIB_ID)
        m_widget->materialBrowserModel()->setHasMaterialLibrary(true);

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
        m_widget->materialBrowserModel()->refreshSearch();
    } else { // is texture
        int idx = m_widget->materialBrowserTexturesModel()->textureIndex(node);
        m_widget->materialBrowserTexturesModel()->selectTexture(idx);
        m_widget->materialBrowserTexturesModel()->refreshSearch();
    }
}

void MaterialBrowserView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    // removing the material lib node
    if (removedNode.id() == Constants::MATERIAL_LIB_ID) {
        m_widget->materialBrowserModel()->setMaterials({}, m_hasQuick3DImport);
        m_widget->materialBrowserModel()->setHasMaterialLibrary(false);
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

ModelNode MaterialBrowserView::getMaterialOfModel(const ModelNode &model, int idx)
{
    QmlObjectNode qmlObjNode(model);
    QString matExp = qmlObjNode.expression("materials");
    if (matExp.isEmpty())
        return {};

    const QStringList mats = matExp.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
    if (mats.isEmpty())
        return {};

    ModelNode mat = modelNodeForId(mats.at(idx));
    QTC_ASSERT(mat.isValid(), return {});

    return mat;
}

void MaterialBrowserView::importsChanged([[maybe_unused]] const Imports &addedImports,
                                         [[maybe_unused]] const Imports &removedImports)
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
                                             const QList<QVariant> &data)
{
    if (view == this && identifier != "select_texture")
        return;

    if (identifier == "select_material") {
        ModelNode matNode;
        if (!data.isEmpty() && !m_selectedModels.isEmpty()) {
            ModelNode model3D = m_selectedModels.at(0);
            QTC_ASSERT(model3D.isValid(), return);
            matNode = getMaterialOfModel(model3D, data[0].toInt());
        } else {
            matNode = nodeList.first();
        }
        QTC_ASSERT(matNode.isValid(), return);

        int idx = m_widget->materialBrowserModel()->materialIndex(matNode);
        if (idx != -1)
            m_widget->materialBrowserModel()->selectMaterial(idx);
    } else if (identifier == "select_texture") {
        int idx = m_widget->materialBrowserTexturesModel()->textureIndex(nodeList.first());
        if (idx != -1) {
            m_widget->materialBrowserTexturesModel()->selectTexture(idx);
            m_widget->materialBrowserTexturesModel()->refreshSearch();
            if (!data.isEmpty() && data[0].toBool())
                m_widget->focusMaterialSection(false);
        }
    } else if (identifier == "refresh_material_browser") {
        QTimer::singleShot(0, model(), [this]() {
            refreshModel(true);
        });
    } else if (identifier == "delete_selected_material") {
        m_widget->deleteSelectedItem();
    } else if (identifier == "apply_asset_to_model3D") {
        m_appliedTexturePath = data.at(0).toString();
        applyTextureToModel3D(nodeList.at(0));
    } else if (identifier == "apply_texture_to_model3D") {
        applyTextureToModel3D(nodeList.at(0), nodeList.at(1));
    } else if (identifier == "apply_texture_to_material") {
        applyTextureToMaterial({nodeList.at(0)}, nodeList.at(1));
    } else if (identifier == "focus_material_section") {
        m_widget->focusMaterialSection(true);
    }
}

void MaterialBrowserView::active3DSceneChanged(qint32 sceneId)
{
    m_sceneId = sceneId;
}

void MaterialBrowserView::currentStateChanged([[maybe_unused]] const ModelNode &node)
{
    m_widget->materialBrowserTexturesModel()->updateAllTexturesSources();
    updateMaterialsPreview();
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

void MaterialBrowserView::applyTextureToModel3D(const QmlObjectNode &model3D, const ModelNode &texture)
{
    if (!texture.isValid() && m_appliedTexturePath.isEmpty())
        return;

    if (!model3D.isValid() || !model3D.modelNode().metaInfo().isQtQuick3DModel())
        return;

    BindingProperty matsProp = model3D.bindingProperty("materials");
    QList<ModelNode> materials;
    if (hasId(matsProp.expression()))
        materials.append(modelNodeForId(matsProp.expression()));
    else
        materials = matsProp.resolveToModelNodeList();

    applyTextureToMaterial(materials, texture);
}

void MaterialBrowserView::applyTextureToMaterial(const QList<ModelNode> &materials,
                                                 const ModelNode &texture)
{
    if (materials.isEmpty())
        return;

    if (texture.isValid())
        m_appliedTextureId = texture.id();

    m_textureModels.clear();
    QStringList materialsModel;
    for (const ModelNode &mat : std::as_const(materials)) {
        QString matName = mat.variantProperty("objectName").value().toString();
        materialsModel.append(QLatin1String("%1 (%2)").arg(matName, mat.id()));
        QList<PropertyName> texProps;
        for (const PropertyMetaInfo &p : mat.metaInfo().properties()) {
            if (p.propertyType().isQtQuick3DTexture())
                texProps.append(p.name());
        }
        m_textureModels.insert(mat.id(), texProps);
    }

    QString path = MaterialBrowserWidget::qmlSourcesPath() + "/ChooseMaterialProperty.qml";

    m_chooseMatPropsView = new QQuickView;
    m_chooseMatPropsView->setTitle(tr("Select a material property"));
    m_chooseMatPropsView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_chooseMatPropsView->setMinimumSize({150, 100});
    m_chooseMatPropsView->setMaximumSize({600, 400});
    m_chooseMatPropsView->setWidth(450);
    m_chooseMatPropsView->setHeight(300);
    m_chooseMatPropsView->setFlags(Qt::Widget);
    m_chooseMatPropsView->setModality(Qt::ApplicationModal);
    m_chooseMatPropsView->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_chooseMatPropsView->rootContext()->setContextProperties({
        {"rootView", QVariant::fromValue(this)},
        {"materialsModel", QVariant::fromValue(materialsModel)},
        {"propertiesModel", QVariant::fromValue(m_textureModels.value(materials.at(0).id()))},
    });
    m_chooseMatPropsView->setSource(QUrl::fromLocalFile(path));
    m_chooseMatPropsView->installEventFilter(this);
    m_chooseMatPropsView->show();
}

void MaterialBrowserView::updatePropsModel(const QString &matId)
{
    m_chooseMatPropsView->rootContext()->setContextProperty("propertiesModel",
                                                QVariant::fromValue(m_textureModels.value(matId)));
}

void MaterialBrowserView::applyTextureToProperty(const QString &matId, const QString &propName)
{
    executeInTransaction(__FUNCTION__, [&] {
        if (m_appliedTextureId.isEmpty() && !m_appliedTexturePath.isEmpty()) {
            auto texCreator = new CreateTexture(this);
            ModelNode tex = texCreator->execute(m_appliedTexturePath, AddTextureMode::Texture);
            m_appliedTextureId = tex.id();
            m_appliedTexturePath.clear();
            texCreator->deleteLater();
        }

        QTC_ASSERT(!m_appliedTextureId.isEmpty(), return);

        QmlObjectNode mat = modelNodeForId(matId);
        QTC_ASSERT(mat.isValid(), return);

        BindingProperty texProp = mat.bindingProperty(propName.toLatin1());
        QTC_ASSERT(texProp.isValid(), return);

        mat.setBindingProperty(propName.toLatin1(), m_appliedTextureId);

        closeChooseMatPropsView();
    });
}

void MaterialBrowserView::closeChooseMatPropsView()
{
    m_chooseMatPropsView->close();
}

bool MaterialBrowserView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (obj == m_chooseMatPropsView)
                m_chooseMatPropsView->close();
        }
    } else if (event->type() == QEvent::Close) {
        if (obj == m_chooseMatPropsView) {
            m_appliedTextureId.clear();
            m_appliedTexturePath.clear();
            m_chooseMatPropsView->deleteLater();
        }
    }

    return AbstractView::eventFilter(obj, event);
}

} // namespace QmlDesigner
