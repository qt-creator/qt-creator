// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowserview.h"

#include "bindingproperty.h"
#include "createtexture.h"
#include "externaldependenciesinterface.h"
#include "materialbrowsermodel.h"
#include "materialbrowsertexturesmodel.h"
#include "materialbrowserwidget.h"

#include <bindingproperty.h>
#include <createtexture.h>
#include <designmodewidget.h>
#include <externaldependenciesinterface.h>
#include <nodeabstractproperty.h>
#include <nodeinstanceview.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <variantproperty.h>
#include <utils3d.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

namespace QmlDesigner {

static bool isMaterial(const ModelNode &node)
{
    return node.metaInfo().isQtQuick3DMaterial();
}

static bool isTexture(const ModelNode &node)
{
    return node.metaInfo().isQtQuick3DTexture();
}

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

static void openPropertyEditor()
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("Properties");
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

        MaterialBrowserModel *matBrowserModel = m_widget->materialBrowserModel().data();

        connect(matBrowserModel, &MaterialBrowserModel::applyToSelectedTriggered, this,
                [&] (const ModelNode &material, bool add) {
            Utils3D::applyMaterialToModels(this, material, Utils3D::getSelectedModels(this), add);
        });

        connect(matBrowserModel,
                &MaterialBrowserModel::renameMaterialTriggered,
                this,
                [&](const ModelNode &material, const QString &newName) {
                    Utils3D::renameMaterial(material, newName);
                    openPropertyEditor();
                });

        connect(matBrowserModel, &MaterialBrowserModel::addNewMaterialTriggered, this, [&] {
            Utils3D::createMaterial(this);
            openPropertyEditor();
        });

        connect(matBrowserModel,
                &MaterialBrowserModel::duplicateMaterialTriggered,
                this,
                [&](const ModelNode &material) {
                    Utils3D::duplicateMaterial(this, material);
                    openPropertyEditor();
                });

        connect(matBrowserModel, &MaterialBrowserModel::pasteMaterialPropertiesTriggered, this,
                [&] (const ModelNode &material,
                     const QList<QmlDesigner::MaterialBrowserModel::PropertyCopyData> &propDatas,
                     bool all) {
            QmlObjectNode mat(material);
            executeInTransaction(__FUNCTION__, [&] {
                if (all) { // all material properties copied
                    // remove current properties
                    if (mat.isInBaseState()) {
                        const QList<AbstractProperty> baseProps = material.properties();
                        PropertyNameViews propNames;
                        for (const auto &baseProp : baseProps) {
                            if (!baseProp.isDynamic())
                                propNames.append(baseProp.name());
                        }

                        for (PropertyNameView propName : propNames) {
                            if (propName != "objectName" && propName != "data")
                                mat.removeProperty(propName);
                        }
                    } else {
                        QmlPropertyChanges changes = mat.propertyChangeForCurrentState();
                        if (changes.isValid()) {
                            PropertyNameViews propNames;
                            const QList<AbstractProperty> changedProps = changes.targetProperties();
                            for (const auto &changedProp : changedProps) {
                                if (!changedProp.isDynamic())
                                    propNames.append(changedProp.name());
                            }

                            for (PropertyNameView propName : propNames) {
                                if (propName != "objectName" && propName != "data")
                                    mat.removeProperty(propName);
                            }
                        }
                    }
                }

                // apply pasted properties
                for (const QmlDesigner::MaterialBrowserModel::PropertyCopyData &propData : propDatas) {
                    if (propData.isValid) {
                        const bool isDynamic = !propData.dynamicTypeName.isEmpty();
                        const bool isBaseState = QmlModelState::isBaseState(currentStateNode());
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
        connect(texturesModel,
                &MaterialBrowserTexturesModel::duplicateTextureTriggered,
                this,
                [&](const ModelNode &texture) {
                    QTC_ASSERT(texture.isValid(), return);
                    CreateTexture(this).execute(texture);
                    openPropertyEditor();
                });

        connect(texturesModel, &MaterialBrowserTexturesModel::applyToSelectedMaterialTriggered, this,
                [&] (const ModelNode &texture) {
            if (!m_widget)
                return;

            ModelNodes materialNodes = Utils3D::getSelectedMaterials(this);
            applyTextureToMaterial(materialNodes, texture);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::applyToSelectedModelTriggered, this,
                [&] (const ModelNode &texture) {
            const QList<ModelNode> selectedModels = Utils3D::getSelectedModels(this);

            if (selectedModels.size() != 1)
                return;
            applyTextureToModel3D(selectedModels[0], texture);
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::addNewTextureTriggered, this, [&] {
            ModelNode texture = CreateTexture(this).execute();
            openPropertyEditor();
        });

        connect(texturesModel, &MaterialBrowserTexturesModel::updateSceneEnvStateRequested, this, [this] {
            ModelNode activeSceneEnv = Utils3D::resolveSceneEnv(this, m_sceneId);
            const bool sceneEnvExists = activeSceneEnv.isValid();
            m_widget->materialBrowserTexturesModel()->setHasSceneEnv(sceneEnvExists);
        });

        connect(texturesModel,
                &MaterialBrowserTexturesModel::applyAsLightProbeRequested,
                this,
                [&](const ModelNode &texture) {
                    executeInTransaction(__FUNCTION__, [&] {
                        Utils3D::assignTextureAsLightProbe(this, texture, m_sceneId);
                    });
                });
    }

    return createWidgetInfo(m_widget.data(),
                            "MaterialBrowser",
                            WidgetInfo::LeftPane,
                            tr("Material Browser"),
                            tr("Material Browser view"));
}

void MaterialBrowserView::createTextures(const QStringList &assetPaths)
{
    executeInTransaction("MaterialBrowserView::createTextures", [&]() {
        CreateTexture(this).execute(assetPaths, AddTextureMode::Texture, m_sceneId);
    });
}

void MaterialBrowserView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->materialBrowserModel()->setHasMaterialLibrary(false);
    m_hasQuick3DImport = model->hasImport("QtQuick3D");
    m_widget->materialBrowserModel()->setIsQt6Project(externalDependencies().isQt6Project());

    // Project load is already very busy and may even trigger QML Puppet reset, so let's wait a moment
    // before refreshing the model
    QTimer::singleShot(1000, model, [this] {
        refreshModel(true);
        loadPropertyGroups(); // Needs the delay because it uses metaInfo
    });

    m_sceneId = Utils3D::active3DSceneId(model);
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

template<typename T, typename> // T can be either BindingProperty or VariantProperty
void MaterialBrowserView::updatePropertyList(const QList<T> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());
        if (isMaterial(node)) {
            if (property.name() == "objectName")
                m_widget->materialBrowserModel()->updateMaterialName(node);
            else
                m_previewRequests << node;
        } else if (isTexture(node)) {
            if (property.name() == "source")
                m_widget->materialBrowserTexturesModel()->updateTextureSource(node);
            else if (property.name() == "objectName")
                m_widget->materialBrowserTexturesModel()->updateTextureName(node);
        } else {
            QmlObjectNode selectedTex = Utils3D::selectedTexture(this);
            if (property.name() == "source" && selectedTex.propertyChangeForCurrentState() == node)
                m_widget->materialBrowserTexturesModel()->updateTextureSource(selectedTex);
        }
    }

    if (!m_previewRequests.isEmpty())
        m_previewTimer.start(0);
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

void MaterialBrowserView::selectedNodesChanged([[maybe_unused]] const QList<ModelNode> &selectedNodeList,
                                               [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    using namespace std::ranges;

    ModelNodes selectedMaterials = Utils::filtered(selectedNodeList, isMaterial);
    ModelNodes deselectedMaterials = Utils::filtered(lastSelectedNodeList, isMaterial);

    ModelNodes selectedTextures = Utils::filtered(selectedNodeList, isTexture);
    ModelNodes deselectedTextures = Utils::filtered(lastSelectedNodeList, isTexture);

    m_widget->materialBrowserModel()->notifySelectionChanges(selectedMaterials, deselectedMaterials);
    m_widget->materialBrowserModel()->setHasModelSelection(!selectedMaterials.isEmpty());

    m_widget->materialBrowserTexturesModel()->notifySelectionChanges(selectedTextures,
                                                                     deselectedTextures);
}

void MaterialBrowserView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                        const QPixmap &pixmap,
                                                        const QByteArray &requestId)
{
    if (!isMaterial(node))
        return;

    // There might be multiple requests for different preview pixmap sizes.
    // Here only the one with the default size is picked.
    if (requestId.isEmpty())
        m_widget->updateMaterialPreview(node, pixmap);
}

void MaterialBrowserView::nodeIdChanged(const ModelNode &node, [[maybe_unused]] const QString &newId,
                                                               [[maybe_unused]] const QString &oldId)
{
    if (isTexture(node))
        m_widget->materialBrowserTexturesModel()->updateTextureId(node);
}

void MaterialBrowserView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                   [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    updatePropertyList(propertyList);
}

void MaterialBrowserView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                                   [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    updatePropertyList(propertyList);
}

void MaterialBrowserView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    updatePropertyList(propertyList);
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
        m_widget->materialBrowserModel()->refreshSearch();
    } else { // is texture
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

void QmlDesigner::MaterialBrowserView::loadPropertyGroups()
{
    if (!m_hasQuick3DImport || m_propertyGroupsLoaded || !model())
        return;

#ifdef QDS_USE_PROJECTSTORAGE
    QString matPropsPath = propertyEditorResourcesPath().append("/QtQuick3D/propertyGroups.json");
#else
    QString matPropsPath = model()->metaInfo("QtQuick3D.Material").importDirectoryPath()
                               + "/designer/propertyGroups.json";
#endif
    m_propertyGroupsLoaded = m_widget->materialBrowserModel()->loadPropertyGroups(matPropsPath);
}

void MaterialBrowserView::requestPreviews()
{
    if (model() && model()->nodeInstanceView()) {
        for (const auto &node : std::as_const(m_previewRequests))
            static_cast<const NodeInstanceView *>(model()->nodeInstanceView())
                ->previewImageDataForGenericNode(node, {});
    }
    m_previewRequests.clear();
}

void MaterialBrowserView::importsChanged([[maybe_unused]] const Imports &addedImports,
                                         [[maybe_unused]] const Imports &removedImports)
{
    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;

    loadPropertyGroups();

    // Import change will trigger QML Puppet reset, so we don't want to update previews immediately
    refreshModel(false);
}

void MaterialBrowserView::customNotification(const AbstractView *view,
                                             const QString &identifier,
                                             const QList<ModelNode> &nodeList,
                                             const QList<QVariant> &data)
{
    if (view == this)
        return;

    if (identifier == "refresh_material_browser") {
        QTimer::singleShot(0, model(), [this] {
            refreshModel(true);
        });
    } else if (identifier == "delete_selected_material") {
        m_widget->deleteSelectedItems();
    } else if (identifier == "apply_asset_to_model3D") {
        m_appliedTexturePath = data.at(0).toString();
        applyTextureToModel3D(nodeList.at(0));
    } else if (identifier == "apply_texture_to_model3D") {
        applyTextureToModel3D(nodeList.at(0), nodeList.at(1));
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
        // We use root node completion as indication of QML Puppet reset
        if (node.isRootNode()) {
            m_puppetResetPending  = false;
            QTimer::singleShot(1000, this, [this] {
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
        if (isMaterial(node))
            m_previewRequests.insert(node);
    }
    if (!m_previewRequests.isEmpty() && !m_previewTimer.isActive()) {
        // Updating material browser isn't urgent in e.g. timeline scrubbing case, so have a bit
        // of delay to reduce unnecessary rendering
        m_previewTimer.start(500);
    }
}

void MaterialBrowserView::auxiliaryDataChanged(const ModelNode &,
                                               AuxiliaryDataKeyView type,
                                               const QVariant &data)
{
    if (type == Utils3D::active3dSceneProperty)
        active3DSceneChanged(data.toInt());
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
            CreateTexture texCreator(this);
            ModelNode tex = texCreator.execute(m_appliedTexturePath, AddTextureMode::Texture);
            m_appliedTextureId = tex.id();
            m_appliedTexturePath.clear();
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

/*!
 * \internal
 * \brief Gets a list of subranges which covers the input list
 * Each subrange will be extended until reaches a gap.
 * A gap is defined as a range that is not included in the input list.
 * Minimum length of the gap should be 2, since 1 is considered as a
 * continuous range.
 * \param values: unsorted integer list
 * \return A sorted list of closed subranges. Each pair consists of two
 * numbers. The first number is the start of the subrange, and the second
 * number is the end of subrange which is available in the values.
 */
QList<QPair<int, int>> MaterialBrowserView::getSortedBounds(const QList<int> &values)
{
    using Bound = QPair<int, int>;
    QList<int> sortedValues = Utils::sorted(values);

    Bound tempBound;
    QList<Bound> bounds;
    bounds.reserve(sortedValues.size());

    if (!sortedValues.isEmpty()) {
        tempBound.first = sortedValues.first();
        tempBound.second = sortedValues.first();
    }

    for (int value : std::as_const(sortedValues)) {
        // If the difference is more than 1, a gap is found.
        // We need to close the previous subrange, and start a new one
        if (value - tempBound.second > 1) {
            bounds << tempBound;
            tempBound.first = value;
        }
        tempBound.second = value;
    }

    if (!sortedValues.isEmpty())
        bounds << tempBound;

    bounds.shrink_to_fit();
    return bounds;
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
