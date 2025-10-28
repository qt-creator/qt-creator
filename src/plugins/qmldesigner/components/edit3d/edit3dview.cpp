// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dview.h"

#include "backgroundcolorselection.h"
#include "bakelights.h"
#include "cameraspeedconfiguration.h"
#include "edit3dcanvas.h"
#include "edit3dviewconfig.h"
#include "edit3dwidget.h"
#include "snapconfiguration.h"

#include <auxiliarydataproperties.h>
#include <customnotificationpackage.h>
#include <designeractionmanager.h>
#include <designericons.h>
#include <designersettings.h>
#include <designmodewidget.h>
#include <modelutils.h>
#include <nodeabstractproperty.h>
#include <nodehints.h>
#include <nodeinstanceview.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <qmlitemnode.h>
#include <qmlvisualnode.h>
#include <seekerslider.h>
#include <utils3d.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>

#include <qmldesignerutils/asset.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QMenu>
#include <QToolButton>

static const QByteArray operator""_actionId(const char *text, size_t size)
{
    return QString("QmlDesigner.Edit3D.%1").arg(QLatin1String(text, size)).toLatin1();
}

namespace QmlDesigner {

inline static QIcon contextIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().contextIcon(iconId);
};

inline static QIcon toolbarIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().toolbarIcon(iconId);
};

Edit3DView::Edit3DView(ExternalDependenciesInterface &externalDependencies,
                       ModulesStorage &modulesStorage)
    : AbstractView{externalDependencies}
    , m_modulesStorage(modulesStorage)
{
    m_compressionTimer.setInterval(1000);
    m_compressionTimer.setSingleShot(true);
    connect(&m_compressionTimer, &QTimer::timeout, this, &Edit3DView::handleEntriesChanged);

    for (int i = 0; i < 4; ++i)
        m_viewportToolStates.append({0, false, i == 0});
}

void Edit3DView::createEdit3DWidget()
{
    createEdit3DActions();
    m_edit3DWidget = new Edit3DWidget(this);
}

void Edit3DView::checkImports()
{
    edit3DWidget()->showCanvas(model()->hasImport("QtQuick3D"));
}

void Edit3DView::setMouseCursor(int mouseCursor)
{
    if (mouseCursor < 0)
        m_edit3DWidget->canvas()->unsetCursor();
    else
        m_edit3DWidget->canvas()->setCursor(QCursor(static_cast<Qt::CursorShape>(mouseCursor)));
}

WidgetInfo Edit3DView::widgetInfo()
{
    if (!m_edit3DWidget)
        createEdit3DWidget();

    return createWidgetInfo(m_edit3DWidget.data(),
                            "Editor3D",
                            WidgetInfo::CentralPane,
                            tr("3D"),
                            tr("3D view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

Edit3DWidget *Edit3DView::edit3DWidget() const
{
    return m_edit3DWidget.data();
}

void Edit3DView::renderImage3DChanged(const QImage &img)
{
    edit3DWidget()->canvas()->updateRenderImage(img);

    // Notify QML Puppet to resize if received image isn't correct size
    if (img.size() != canvasSize())
        edit3DViewResized(canvasSize());
    if (edit3DWidget()->canvas()->busyIndicator()->isVisible()) {
        edit3DWidget()->canvas()->setOpacity(1.0);
        edit3DWidget()->canvas()->busyIndicator()->hide();
    }
}

void Edit3DView::updateActiveScene3D(const QVariantMap &sceneState)
{
    const QString mouseCursorKey = QStringLiteral("mouseCursor");
    if (sceneState.contains(mouseCursorKey)) {
        setMouseCursor(sceneState[mouseCursorKey].toInt());
        // Mouse cursor state is always reported separately, as we never want to persist this state
        return;
    }

    const QString activeViewportKey = QStringLiteral("activeViewport");
    if (sceneState.contains(activeViewportKey)) {
        setActiveViewport(sceneState[activeViewportKey].toInt());
        // If the sceneState contained just activeViewport key, then this is simply an active Viewport
        // change rather than entire active scene change, and we don't need to process further.
        if (sceneState.size() == 1)
            return;
    } else {
        setActiveViewport(0);
    }

    const QString sceneKey              = QStringLiteral("sceneInstanceId");
    const QString selectKey             = QStringLiteral("selectionMode");
    const QString transformKey          = QStringLiteral("transformMode");
    const QString perspectiveKey        = QStringLiteral("usePerspective");
    const QString orientationKey        = QStringLiteral("globalOrientation");
    const QString editLightKey          = QStringLiteral("showEditLight");
    const QString gridKey               = QStringLiteral("showGrid");
    const QString showLookAtKey         = QStringLiteral("showLookAt");
    const QString selectionBoxKey       = QStringLiteral("showSelectionBox");
    const QString iconGizmoKey          = QStringLiteral("showIconGizmo");
    const QString cameraFrustumKey      = QStringLiteral("showCameraFrustum");
    const QString cameraViewModeKey     = QStringLiteral("cameraViewMode");
    const QString particleEmitterKey    = QStringLiteral("showParticleEmitter");
    const QString particlesPlayKey      = QStringLiteral("particlePlay");
    const QString syncEnvBgKey          = QStringLiteral("syncEnvBackground");
    const QString activePresetKey       = QStringLiteral("activePreset");
    const QString matOverrideKey        = QStringLiteral("matOverride");
    const QString showWireframeKey      = QStringLiteral("showWireframe");

    if (sceneState.contains(sceneKey)) {
        qint32 newActiveScene = sceneState[sceneKey].value<qint32>();
        edit3DWidget()->canvas()->updateActiveScene(newActiveScene);
        updateAlignActionStates();
    }

    if (sceneState.contains(selectKey))
        m_selectionModeAction->action()->setChecked(sceneState[selectKey].toInt() == 1);
    else
        m_selectionModeAction->action()->setChecked(false);

    if (sceneState.contains(transformKey)) {
        const int tool = sceneState[transformKey].toInt();
        if (tool == 0)
            m_moveToolAction->action()->setChecked(true);
        else if (tool == 1)
            m_rotateToolAction->action()->setChecked(true);
        else
            m_scaleToolAction->action()->setChecked(true);
    } else {
        m_moveToolAction->action()->setChecked(true);
    }

    if (sceneState.contains(perspectiveKey)) {
        const QVariantList showList = sceneState[perspectiveKey].toList();
        for (int i = 0; i < 4; ++i)
            m_viewportToolStates[i].isPerspective = i < showList.size() ? showList[i].toBool() : i == 0;
    } else {
        for (int i = 0; i < 4; ++i)
            m_viewportToolStates[i].isPerspective = i == 0;
    }
    m_cameraModeAction->action()->setChecked(m_viewportToolStates[m_activeViewport].isPerspective);

    if (sceneState.contains(orientationKey))
        m_orientationModeAction->action()->setChecked(sceneState[orientationKey].toBool());
    else
        m_orientationModeAction->action()->setChecked(false);

    if (sceneState.contains(editLightKey))
        m_editLightAction->action()->setChecked(sceneState[editLightKey].toBool());
    else
        m_editLightAction->action()->setChecked(false);

    if (sceneState.contains(gridKey))
        m_showGridAction->action()->setChecked(sceneState[gridKey].toBool());
    else
        m_showGridAction->action()->setChecked(false);

    if (sceneState.contains(showLookAtKey))
        m_showLookAtAction->action()->setChecked(sceneState[showLookAtKey].toBool());
    else
        m_showLookAtAction->action()->setChecked(false);

    if (sceneState.contains(selectionBoxKey))
        m_showSelectionBoxAction->action()->setChecked(sceneState[selectionBoxKey].toBool());
    else
        m_showSelectionBoxAction->action()->setChecked(false);

    if (sceneState.contains(iconGizmoKey))
        m_showIconGizmoAction->action()->setChecked(sceneState[iconGizmoKey].toBool());
    else
        m_showIconGizmoAction->action()->setChecked(false);

    if (sceneState.contains(cameraFrustumKey))
        m_showCameraFrustumAction->action()->setChecked(sceneState[cameraFrustumKey].toBool());
    else
        m_showCameraFrustumAction->action()->setChecked(false);

    if (sceneState.contains(cameraViewModeKey))
        m_cameraViewAction->setMode(sceneState[cameraViewModeKey].toByteArray());
    else
        m_cameraViewAction->setMode("");

    if (sceneState.contains(particleEmitterKey))
        m_showParticleEmitterAction->action()->setChecked(sceneState[particleEmitterKey].toBool());
    else
        m_showParticleEmitterAction->action()->setChecked(false);

    if (sceneState.contains(particlesPlayKey))
        m_particlesPlayAction->action()->setChecked(sceneState[particlesPlayKey].toBool());
    else
        m_particlesPlayAction->action()->setChecked(true);

    if (sceneState.contains(matOverrideKey)) {
        const QVariantList overrides = sceneState[matOverrideKey].toList();
        for (int i = 0; i < 4; ++i)
            m_viewportToolStates[i].matOverride = i < overrides.size() ? overrides[i].toInt() : 0;
    } else {
        for (ViewportToolState &state : m_viewportToolStates)
            state.matOverride = 0;
    }

    if (sceneState.contains(showWireframeKey)) {
        const QVariantList showList = sceneState[showWireframeKey].toList();
        for (int i = 0; i < 4; ++i)
            m_viewportToolStates[i].showWireframe = i < showList.size() ? showList[i].toBool() : false;
    } else {
        for (ViewportToolState &state : m_viewportToolStates)
            state.showWireframe = false;
    }

    if (sceneState.contains(syncEnvBgKey))
        m_syncEnvBackgroundAction->action()->setChecked(sceneState[syncEnvBgKey].toBool());
    else
        m_syncEnvBackgroundAction->action()->setChecked(false);

    if (sceneState.contains(activePresetKey))
        syncActivePresetCheckedState(static_cast<ViewPreset>(sceneState[activePresetKey].toInt()));
    else
        syncActivePresetCheckedState(ViewPreset::Single);

    // Selection context change updates visible and enabled states
    SelectionContext selectionContext(this);
    selectionContext.setUpdateMode(SelectionContext::UpdateMode::Fast);
    if (m_bakeLightsAction)
        m_bakeLightsAction->currentContextChanged(selectionContext);

    syncCameraSpeedToNewView();

    storeCurrentSceneEnvironment();
}

void Edit3DView::setActiveViewport(int viewport)
{
    m_activeViewport = viewport;
}

void Edit3DView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    QString currProjectPath = QmlDesigner::DocumentManager::currentProjectDirPath().toUrlishString();
    if (m_currProjectPath != currProjectPath) {
        // Opening a new project -> reset camera speeds
        m_currProjectPath = currProjectPath;
        m_previousCameraSpeed = -1.;
        m_previousCameraMultiplier = -1.;
    }

    edit3DWidget()->canvas()->updateActiveScene(Utils3D::active3DSceneId(model));

    updateAlignActionStates();
    syncSnapAuxPropsToSettings();

    rootModelNode().setAuxiliaryData(edit3dGridColorProperty,
                                     QVariant::fromValue(Edit3DViewConfig::loadColor(
                                         DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR)));
    rootModelNode().setAuxiliaryData(edit3dBgColorProperty,
                                     QVariant::fromValue(Edit3DViewConfig::loadColors(
                                         DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR)));

    checkImports();
    auto cachedImage = m_canvasCache.take(model);
    if (cachedImage) {
        edit3DWidget()->canvas()->updateRenderImage(*cachedImage);
        edit3DWidget()->canvas()->setOpacity(0.5);
    }

    edit3DWidget()->canvas()->busyIndicator()->show();

    m_isBakingLightsSupported = false;
    m_kitVersion = {};
    ProjectExplorer::Target *target = QmlDesignerPlugin::instance()->currentDesignDocument()->currentTarget();
    if (target && target->kit()) {
        if (QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(target->kit())) {
            m_isBakingLightsSupported = qtVer->qtVersion() >= QVersionNumber(6, 5, 0);
            m_kitVersion = qtVer->qtVersion();
            if (m_bakeLights)
                m_bakeLights->setKitVersion(m_kitVersion);
        }
    }

    onEntriesChanged();

    model->sendCustomNotificationToNodeInstanceView(
        Request3DSceneToolStates{Utils3D::active3DSceneNode(this).id()});
}

void Edit3DView::onEntriesChanged()
{
    m_compressionTimer.start();
}

void Edit3DView::registerEdit3DAction(Edit3DAction *action)
{
    if (action->actionType() != View3DActionType::Empty)
        m_edit3DActions.insert(action->actionType(), action);
}

void Edit3DView::handleEntriesChanged()
{
    if (!model())
        return;

    enum ItemLibraryEntryKeys : int { // used to maintain order
        EK_cameras,
        EK_lights,
        EK_primitives
    };

    QMap<ItemLibraryEntryKeys, ItemLibraryDetails> entriesMap{
        {EK_cameras, {tr("Cameras"), contextIcon(DesignerIcons::CameraIcon)}},
        {EK_lights, {tr("Lights"), contextIcon(DesignerIcons::LightIcon)}},
        {EK_primitives, {tr("Primitives"), contextIcon(DesignerIcons::PrimitivesIcon)}}
    };

    auto append = [&](const NodeMetaInfo &metaInfo, ItemLibraryEntryKeys key) {
        auto entries = metaInfo.itemLibrariesEntries();
        if (entries.size())
            entriesMap[key].entryList.append(toItemLibraryEntries(model()->pathCache(), entries));
    };

    append(model()->qtQuick3DModelMetaInfo(), EK_primitives);
    append(model()->qtQuick3DDirectionalLightMetaInfo(), EK_lights);
    append(model()->qtQuick3DSpotLightMetaInfo(), EK_lights);
    append(model()->qtQuick3DPointLightMetaInfo(), EK_lights);
    append(model()->qtQuick3DOrthographicCameraMetaInfo(), EK_cameras);
    append(model()->qtQuick3DPerspectiveCameraMetaInfo(), EK_cameras);

    m_edit3DWidget->updateCreateSubMenu(entriesMap.values());
}

void Edit3DView::updateAlignActionStates()
{
    bool enabled = false;

    ModelNode activeScene = Utils3D::active3DSceneNode(this);
    if (activeScene.isValid()) {
        const QList<ModelNode> nodes = activeScene.allSubModelNodes();
        enabled = ::Utils::anyOf(nodes, [](const ModelNode &node) {
            return node.metaInfo().isQtQuick3DCamera();
        });
    }

    m_alignCamerasAction->action()->setEnabled(enabled);
    m_alignViewAction->action()->setEnabled(enabled);
}

void Edit3DView::setActive3DSceneId(qint32 sceneId)
{
    rootModelNode().setAuxiliaryData(active3dSceneProperty, sceneId);
}

void Edit3DView::emitView3DAction(View3DActionType type, const QVariant &value)
{
    if (isAttached())
        model()->emitView3DAction(type, value);
}

void Edit3DView::modelAboutToBeDetached(Model *model)
{
    m_isBakingLightsSupported = false;

    if (m_bakeLights)
        m_bakeLights->cancel();

    if (m_snapConfiguration)
        m_snapConfiguration->cancel();

    // Hide the canvas when model is detached (i.e. changing documents)
    if (edit3DWidget() && edit3DWidget()->canvas()) {
        m_canvasCache.insert(model, edit3DWidget()->canvas()->renderImage());
        edit3DWidget()->showCanvas(false);
    }

    AbstractView::modelAboutToBeDetached(model);
}

void Edit3DView::importsChanged([[maybe_unused]] const Imports &addedImports,
                                [[maybe_unused]] const Imports &removedImports)
{
    checkImports();
}

void Edit3DView::customNotification([[maybe_unused]] const AbstractView *view,
                                    const QString &identifier,
                                    [[maybe_unused]] const QList<ModelNode> &nodeList,
                                    [[maybe_unused]] const QList<QVariant> &data)
{
    if (identifier == "pick_3d_node_from_2d_scene" && data.size() == 2) {
        // Pick via 2D view, data has pick coordinates in main scene coordinates
        QTimer::singleShot(0, this, [=, self = QPointer{this}]() {
            if (!self)
                return;

            self->emitView3DAction(View3DActionType::GetNodeAtMainScenePos,
                                   QVariantList{data[0], data[1]});
            self->m_nodeAtPosReqType = NodeAtPosReqType::MainScenePick;
            self->m_pickView3dNode = self->modelNodeForInternalId(qint32(data[1].toInt()));
        });
    }
}

/**
 * @brief Get node at position from QML Puppet process
 *
 * Response from QML Puppet process for the model at requested position
 *
 * @param modelNode Node picked at the requested position or invalid node if nothing could be picked
 * @param pos3d 3D scene position of the requested view position
 */
void Edit3DView::nodeAtPosReady(const ModelNode &modelNode, const QVector3D &pos3d)
{
    if (m_nodeAtPosReqType == NodeAtPosReqType::ContextMenu) {
        m_contextMenuPos3D = pos3d;
        if (m_edit3DWidget->canvas()->isFlyMode()) {
            m_contextMenuPendingNode = modelNode;
        } else {
            m_nodeAtPosReqType = NodeAtPosReqType::None;
            showContextMenu();
        }
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::ComponentDrop) {
        ModelNode createdNode;
        executeInTransaction(__FUNCTION__, [&] {
            createdNode = QmlVisualNode::createQml3DNode(
                this, m_droppedEntry, edit3DWidget()->canvas()->activeScene(), pos3d).modelNode();
            if (createdNode.metaInfo().isQtQuick3DModel())
                Utils3D::assignMaterialTo3dModel(this, createdNode);
        });
        if (createdNode.isValid())
            setSelectedModelNode(createdNode);
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::MaterialDrop) {
        bool isModel = modelNode.metaInfo().isQtQuick3DModel();
        if (m_droppedModelNode.isValid() && isModel) {
            executeInTransaction(__FUNCTION__, [&] {
                Utils3D::assignMaterialTo3dModel(this, modelNode, m_droppedModelNode);
            });
        }
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::BundleMaterialDrop) {
        emitCustomNotification("drop_bundle_material", {modelNode}); // To ContentLibraryView
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::BundleEffectDrop) {
        emitCustomNotification("drop_bundle_item", {modelNode}, {pos3d}); // To ContentLibraryView
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::TextureDrop) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
        emitCustomNotification("apply_texture_to_model3D", {modelNode, m_droppedModelNode});
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::AssetDrop) {
        bool isModel = modelNode.metaInfo().isQtQuick3DModel();
        if (!m_droppedTexture.isEmpty() && isModel) {
            QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
            emitCustomNotification("apply_asset_to_model3D", {modelNode}, {m_droppedTexture}); // To MaterialBrowserView
        } else if (!m_dropped3dImports.isEmpty()) {
            ModelNode sceneNode = Utils3D::active3DSceneNode(this);
            if (!sceneNode.isValid())
                sceneNode = rootModelNode();
            ModelNode createdNode;
            executeInTransaction(__FUNCTION__, [&] {
                for (const QString &asset : std::as_const(m_dropped3dImports)) {
                    createdNode = ModelNodeOperations::handleImported3dAssetDrop(
                        asset, sceneNode, pos3d);
                }
            });
            if (createdNode.isValid())
                setSelectedModelNode(createdNode);
        }
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::MainScenePick) {
        if (modelNode.isValid())
            setSelectedModelNode(modelNode);
        else if (m_pickView3dNode.isValid() && !m_pickView3dNode.isSelected())
            setSelectedModelNode(m_pickView3dNode);
        emitView3DAction(View3DActionType::AlignViewToCamera, true);
    }

    m_droppedModelNode = {};
    m_dropped3dImports.clear();
    m_droppedTexture.clear();
    m_nodeAtPosReqType = NodeAtPosReqType::None;
}

void Edit3DView::nodeReparented(const ModelNode &,
                                const NodeAbstractProperty &,
                                const NodeAbstractProperty &,
                                PropertyChangeFlags)
{
    updateAlignActionStates();
}

void Edit3DView::nodeRemoved(const ModelNode &,
                             const NodeAbstractProperty &,
                             PropertyChangeFlags)
{
    updateAlignActionStates();
}

void Edit3DView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    maybeStoreCurrentSceneEnvironment(propertyList);
}

void Edit3DView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags)
{
    maybeStoreCurrentSceneEnvironment(propertyList);
}

void Edit3DView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags)
{
    maybeStoreCurrentSceneEnvironment(propertyList);
}

void Edit3DView::exportedTypeNamesChanged(const ExportedTypeNames &added,
                                          const ExportedTypeNames &removed)
{
    if (Utils3D::hasImported3dType(this, added, removed))
        onEntriesChanged();
}

void Edit3DView::sendInputEvent(QEvent *e) const
{
    if (isAttached())
        model()->sendCustomNotificationToNodeInstanceView(InputEvent{e});
}

void Edit3DView::edit3DViewResized(const QSize &size) const
{
    if (isAttached())
        model()->sendCustomNotificationToNodeInstanceView(Resize3DCanvas{size});
}

QSize Edit3DView::canvasSize() const
{
    if (!m_edit3DWidget.isNull() && m_edit3DWidget->canvas())
        return m_edit3DWidget->canvas()->size();

    return {};
}

void Edit3DView::createSelectBackgroundColorAction(QAction *syncEnvBackgroundAction)
{
    QString description = Tr::tr("Select Background Color");
    QString tooltip = Tr::tr("Select a color for the background of the 3D view.");

    auto operation = [this, syncEnvBackgroundAction](const SelectionContext &) {
        BackgroundColorSelection::showBackgroundColorSelectionWidget(
            edit3DWidget(),
            DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR,
            this,
            edit3dBgColorProperty,
            [this, syncEnvBackgroundAction]() {
                if (syncEnvBackgroundAction->isChecked()) {
                    emitView3DAction(View3DActionType::SyncEnvBackground, false);
                    syncEnvBackgroundAction->setChecked(false);
                }
            });
    };

    m_selectBackgroundColorAction = std::make_unique<Edit3DAction>(
        Constants::EDIT3D_EDIT_SELECT_BACKGROUND_COLOR,
        View3DActionType::Empty,
        description,
        QKeySequence(),
        false,
        false,
        QIcon(),
        this,
        operation,
        tooltip);
}

void Edit3DView::createGridColorSelectionAction()
{
    QString description = Tr::tr("Select Grid Color");
    QString tooltip = Tr::tr("Select a color for the grid lines of the 3D view.");

    auto operation = [this](const SelectionContext &) {
        BackgroundColorSelection::showBackgroundColorSelectionWidget(
            edit3DWidget(),
            DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR,
            this,
            edit3dGridColorProperty);
    };

    m_selectGridColorAction = std::make_unique<Edit3DAction>(
        Constants::EDIT3D_EDIT_SELECT_GRID_COLOR,
        View3DActionType::Empty,
        description,
        QKeySequence(),
        false,
        false,
        QIcon(),
        this,
        operation,
        tooltip);
}

void Edit3DView::createResetColorAction(QAction *syncEnvBackgroundAction)
{
    QString description = Tr::tr("Reset Colors");
    QString tooltip = Tr::tr("Reset the background color and the color of the "
                             "grid lines of the 3D view to the default values.");

    auto operation = [this, syncEnvBackgroundAction](const SelectionContext &) {
        QList<QColor> bgColors = {QRgb(0x222222), QRgb(0x999999)};
        Edit3DViewConfig::setColors(this, edit3dBgColorProperty, bgColors);
        Edit3DViewConfig::saveColors(DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR, bgColors);

        QColor gridColor{0xcccccc};
        Edit3DViewConfig::setColors(this, edit3dGridColorProperty, {gridColor});
        Edit3DViewConfig::saveColors(DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR, {gridColor});

        if (syncEnvBackgroundAction->isChecked()) {
            emitView3DAction(View3DActionType::SyncEnvBackground, false);
            syncEnvBackgroundAction->setChecked(false);
        }
    };

    m_resetColorAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_RESET_BACKGROUND_COLOR,
        View3DActionType::Empty,
        description,
        QKeySequence(),
        false,
        false,
        QIcon(),
        this,
        operation,
        tooltip);
}

void Edit3DView::createSyncEnvBackgroundAction()
{
    QString description = Tr::tr("Use Scene Environment");
    QString tooltip = Tr::tr("Sets the 3D view to use the Scene Environment "
                             "color or skybox as background color.");

    m_syncEnvBackgroundAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SYNC_ENV_BACKGROUND,
        View3DActionType::SyncEnvBackground,
        description,
        QKeySequence(),
        true,
        false,
        QIcon(),
        this,
        nullptr,
        tooltip);
}

void Edit3DView::createViewportPresetActions()
{
    auto createViewportPresetAction = [this](std::unique_ptr<Edit3DAction> &targetAction,
                                             const QByteArray &id,
                                             const QString &label,
                                             const ViewPreset &opCode,
                                             const QIcon &icon,
                                             bool isChecked) {
        auto operation = [this, &targetAction, opCode](const SelectionContext &) {
            for (Edit3DAction *action : std::as_const(m_viewportPresetActions))
                action->action()->setChecked(action->menuId() == targetAction->menuId());

            emitView3DAction(View3DActionType::ViewportPreset, int(opCode));
        };

        targetAction = std::make_unique<Edit3DAction>(
            id,
            View3DActionType::Empty,
            label,
            QKeySequence(),
            true,
            isChecked,
            icon,
            this,
            operation);
    };

    createViewportPresetAction(m_viewportPresetSingleAction, Constants::EDIT3D_PRESET_SINGLE,
                               Tr::tr("Single"), ViewPreset::Single, contextIcon(DesignerIcons::MultiViewPort1Icon), true);
    createViewportPresetAction(m_viewportPresetQuadAction, Constants::EDIT3D_PRESET_QUAD,
                               Tr::tr("Quad"), ViewPreset::Quad, contextIcon(DesignerIcons::MultiViewPort2x2Icon), false);
    createViewportPresetAction(m_viewportPreset3Left1RightAction, Constants::EDIT3D_PRESET_3LEFT1RIGHT,
                               Tr::tr("3 Left 1 Right"), ViewPreset::ThreeLeftOneRight, contextIcon(DesignerIcons::MultiViewPort3plus1Icon), false);
    createViewportPresetAction(m_viewportPreset2HorizontalAction, Constants::EDIT3D_PRESET_2HORIZONTAL,
                               Tr::tr("2 Horizontal"), ViewPreset::TwoHorizontal, contextIcon(DesignerIcons::MultiViewPort2hlIcon), false);
    createViewportPresetAction(m_viewportPreset2VerticalAction, Constants::EDIT3D_PRESET_2VERTICAL,
                               Tr::tr("2 Vertical"), ViewPreset::TwoVertical, contextIcon(DesignerIcons::MultiViewPort2vlIcon), false);

    m_viewportPresetActions << m_viewportPresetSingleAction.get();
    m_viewportPresetActions << m_viewportPresetQuadAction.get();
    m_viewportPresetActions << m_viewportPreset3Left1RightAction.get();
    m_viewportPresetActions << m_viewportPreset2HorizontalAction.get();
    m_viewportPresetActions << m_viewportPreset2VerticalAction.get();
}

void Edit3DView::createSeekerSliderAction()
{
    m_seekerAction = std::make_unique<Edit3DParticleSeekerAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLES_SEEKER,
        View3DActionType::ParticlesSeek,
        this);

    m_seekerAction->action()->setEnabled(false);
    m_seekerAction->action()->setToolTip(QLatin1String("Seek particle system time when paused."));

    connect(m_seekerAction->seekerAction(), &SeekerSliderAction::valueChanged, this,
            [this] (int value) {
        emitView3DAction(View3DActionType::ParticlesSeek, value);
    });
}

QPoint Edit3DView::resolveToolbarPopupPos(Edit3DAction *action) const
{
    QPoint pos;
    const QObjectList &objects = action->action()->associatedObjects();
    for (QObject *obj : objects) {
        if (auto button = qobject_cast<QToolButton *>(obj)) {
            if (auto toolBar = qobject_cast<QWidget *>(button->parent())) {
                // If the button has not yet been shown (i.e. it starts under extension menu),
                // the button x value will be zero.
                if (button->x() >= toolBar->width() - button->width() || button->x() == 0) {
                    pos = toolBar->mapToGlobal(QPoint(toolBar->width() - button->width(), 4));
                } else {
                    // Add small negative modifier to Y coordinate, so highlighted toolbar buttons don't
                    // peek from under the popup
                    pos = button->mapToGlobal(QPoint(0, -2));
                }
            }
            break;
        }
    }
    return pos;
}

template<typename T, typename>
void Edit3DView::maybeStoreCurrentSceneEnvironment(const QList<T> &propertyList)
{
    QSet<qint32> handledNodes;
    QmlObjectNode sceneEnv;
    for (const AbstractProperty &prop : propertyList) {
        ModelNode node = prop.parentModelNode();
        const qint32 id = node.internalId();
        if (handledNodes.contains(id))
            continue;

        handledNodes.insert(id);
        if (!node.metaInfo().isQtQuick3DSceneEnvironment())
            continue;

        if (!sceneEnv.isValid())
            sceneEnv = currentSceneEnv();
        if (sceneEnv == node) {
            storeCurrentSceneEnvironment();
            break;
        }
    }
}

void Edit3DView::showContextMenu()
{
    // If request for context menu is still pending, skip for now
    if (m_nodeAtPosReqType == NodeAtPosReqType::ContextMenu)
        return;

    if (m_contextMenuPendingNode.isValid()) {
        if (!m_contextMenuPendingNode.isSelected())
            setSelectedModelNode(m_contextMenuPendingNode);
    } else {
        clearSelectedModelNodes();
    }

    m_edit3DWidget->showContextMenu(m_contextMenuPosMouse, m_contextMenuPendingNode, m_contextMenuPos3D);
    m_contextMenuPendingNode = {};
}

void Edit3DView::setFlyMode(bool enabled)
{
    emitView3DAction(View3DActionType::FlyModeToggle, enabled);
}

void Edit3DView::syncSnapAuxPropsToSettings()
{
    if (!model())
        return;

    bool snapToggle = m_snapToggleAction->action()->isChecked();
    rootModelNode().setAuxiliaryData(edit3dSnapPosProperty,
                                     snapToggle ? Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION)
                                                : false);
    rootModelNode().setAuxiliaryData(edit3dSnapRotProperty,
                                     snapToggle ? Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION)
                                                : false);
    rootModelNode().setAuxiliaryData(edit3dSnapScaleProperty,
                                     snapToggle ? Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE)
                                                : false);
    rootModelNode().setAuxiliaryData(edit3dSnapAbsProperty,
                                     Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ABSOLUTE));
    rootModelNode().setAuxiliaryData(edit3dSnapPosIntProperty,
                                     Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL));
    rootModelNode().setAuxiliaryData(edit3dSnapRotIntProperty,
                                     Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL));
    rootModelNode().setAuxiliaryData(edit3dSnapScaleIntProperty,
                                     Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL));
}

void Edit3DView::setCameraSpeedAuxData(double speed, double multiplier)
{
    ModelNode node = Utils3D::active3DSceneNode(this);
    node.setAuxiliaryData(edit3dCameraSpeedDocProperty, speed);
    node.setAuxiliaryData(edit3dCameraSpeedMultiplierDocProperty, multiplier);
    rootModelNode().setAuxiliaryData(edit3dCameraTotalSpeedProperty, (speed * multiplier));
    m_previousCameraSpeed = speed;
    m_previousCameraMultiplier = multiplier;
}

void Edit3DView::getCameraSpeedAuxData(double &speed, double &multiplier)
{
    ModelNode node = Utils3D::active3DSceneNode(this);
    auto speedProp = node.auxiliaryData(edit3dCameraSpeedDocProperty);
    auto multProp = node.auxiliaryData(edit3dCameraSpeedMultiplierDocProperty);
    speed = speedProp ? speedProp->toDouble() : CameraSpeedConfiguration::defaultSpeed;
    multiplier = multProp ? multProp->toDouble() : CameraSpeedConfiguration::defaultMultiplier;
}

void Edit3DView::syncCameraSpeedToNewView()
{
    // Camera speed is inherited from previous active view if explicit values have not been
    // stored for the currently active view
    ModelNode node = Utils3D::active3DSceneNode(this);
    auto speedProp = node.auxiliaryData(edit3dCameraSpeedDocProperty);
    auto multProp = node.auxiliaryData(edit3dCameraSpeedMultiplierDocProperty);
    double speed = CameraSpeedConfiguration::defaultSpeed;
    double multiplier = CameraSpeedConfiguration::defaultMultiplier;

    if (!speedProp || !multProp) {
        if (m_previousCameraSpeed > 0 && m_previousCameraMultiplier > 0) {
            speed = m_previousCameraSpeed;
            multiplier = m_previousCameraMultiplier;
        }
    } else {
        speed = speedProp->toDouble();
        multiplier = multProp->toDouble();
    }

    setCameraSpeedAuxData(speed, multiplier);
}

void Edit3DView::syncActivePresetCheckedState(ViewPreset preset)
{
    m_viewportPresetSingleAction->action()->setChecked(preset == ViewPreset::Single);
    m_viewportPresetQuadAction->action()->setChecked(preset == ViewPreset::Quad);
    m_viewportPreset3Left1RightAction->action()->setChecked(preset == ViewPreset::ThreeLeftOneRight);
    m_viewportPreset2HorizontalAction->action()->setChecked(preset == ViewPreset::TwoHorizontal);
    m_viewportPreset2VerticalAction->action()->setChecked(preset == ViewPreset::TwoVertical);
}

QmlObjectNode Edit3DView::currentSceneEnv()
{
    PropertyName envProp{"environment"};
    ModelNode checkNode = Utils3D::active3DSceneNode(this);
    while (checkNode.isValid()) {
        if (checkNode.metaInfo().isQtQuick3DView3D()) {
            QmlObjectNode sceneEnvNode = QmlItemNode(checkNode).bindingProperty(envProp)
                                             .resolveToModelNode();
            if (sceneEnvNode.isValid())
                return sceneEnvNode;
            break;
        }
        if (checkNode.hasParentProperty())
            checkNode = checkNode.parentProperty().parentModelNode();
        else
            break;
    }
    return {};
}

void Edit3DView::storeCurrentSceneEnvironment()
{
    // If current active scene has scene environment, store relevant properties
    QmlObjectNode sceneEnvNode = currentSceneEnv();
    if (sceneEnvNode.isValid()) {
        QVariantMap lastSceneEnvData;

        auto insertPropValue = [](const PropertyName prop, const QmlObjectNode &node,
                                  QVariantMap &map) {
            if (!node.hasProperty(prop))
                return;

            map.insert(QString::fromUtf8(prop), node.modelValue(prop));
        };

        auto insertTextureProps = [&](const PropertyName prop) {
            // For now we just grab the absolute path of texture source for simplicity
            if (!sceneEnvNode.hasProperty(prop))
                return;

            QmlObjectNode bindNode = QmlItemNode(sceneEnvNode).bindingProperty(prop)
                                         .resolveToModelNode();
            if (bindNode.isValid()) {
                QVariantMap props;
                const PropertyName sourceProp = "source";
                if (bindNode.hasProperty(sourceProp)) {
                    Utils::FilePath qmlPath = Utils::FilePath::fromUrl(
                        model()->fileUrl()).absolutePath();
                    Utils::FilePath sourcePath = Utils::FilePath::fromUrl(
                        bindNode.modelValue(sourceProp).toUrl());

                    sourcePath = qmlPath.resolvePath(sourcePath);

                    props.insert(QString::fromUtf8(sourceProp),
                                 sourcePath.absoluteFilePath().toUrl());
                }
                lastSceneEnvData.insert(QString::fromUtf8(prop), props);
            }
        };

        insertPropValue("backgroundMode", sceneEnvNode, lastSceneEnvData);
        insertPropValue("clearColor", sceneEnvNode, lastSceneEnvData);
        insertTextureProps("lightProbe");
        insertTextureProps("skyBoxCubeMap");

        emitView3DAction(View3DActionType::SetLastSceneEnvData, lastSceneEnvData);
    }
}

const QList<Edit3DView::ViewportToolState> &Edit3DView::viewportToolStates() const
{
    return m_viewportToolStates;
}

void Edit3DView::setViewportToolState(int viewportIndex, const ViewportToolState &state)
{
    if (viewportIndex >= m_viewportToolStates.size())
        return;

    m_viewportToolStates[viewportIndex] = state;
}

int Edit3DView::activeViewport() const
{
    return m_activeViewport;
}

bool Edit3DView::isMultiViewportView() const
{
    return m_viewportPresetsMenuAction->action()->isChecked();
}

void Edit3DView::createEdit3DActions()
{
    m_selectionModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_SELECTION_MODE,
        View3DActionType::SelectionModeToggle,
        Tr::tr("Toggle Group/Single Selection Mode"),
        QKeySequence(Qt::Key_Q),
        true,
        false,
        toolbarIcon(DesignerIcons::ToggleGroupIcon),
        this);

    m_moveToolAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_MOVE_TOOL,
                                                      View3DActionType::MoveTool,
                                                      Tr::tr("Activate Move Tool"),
                                                      QKeySequence(Qt::Key_W),
                                                      true,
                                                      true,
                                                      toolbarIcon(DesignerIcons::MoveToolIcon),
                                                      this);

    m_rotateToolAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_ROTATE_TOOL,
                                                        View3DActionType::RotateTool,
                                                        Tr::tr("Activate Rotate Tool"),
                                                        QKeySequence(Qt::Key_E),
                                                        true,
                                                        false,
                                                        toolbarIcon(DesignerIcons::RotateToolIcon),
                                                        this);

    m_scaleToolAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_SCALE_TOOL,
                                                       View3DActionType::ScaleTool,
                                                       Tr::tr("Activate Scale Tool"),
                                                       QKeySequence(Qt::Key_R),
                                                       true,
                                                       false,
                                                       toolbarIcon(DesignerIcons::ScaleToolIcon),
                                                       this);

    m_fitAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_FIT_SELECTED,
                                                 View3DActionType::FitToView,
                                                 Tr::tr("Fit Selected Object to View"),
                                                 QKeySequence(Qt::Key_F),
                                                 false,
                                                 false,
                                                 toolbarIcon(DesignerIcons::FitToViewIcon),
                                                 this);

    m_alignCamerasAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_ALIGN_CAMERAS,
                                                          View3DActionType::AlignCamerasToView,
                                                          Tr::tr("Align Cameras to View"),
                                                          QKeySequence(),
                                                          false,
                                                          false,
                                                          toolbarIcon(
                                                              DesignerIcons::AlignCameraToViewIcon),
                                                          this);

    m_alignViewAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_ALIGN_VIEW,
                                                       View3DActionType::AlignViewToCamera,
                                                       Tr::tr("Align View to Camera"),
                                                       QKeySequence(),
                                                       false,
                                                       false,
                                                       toolbarIcon(DesignerIcons::AlignViewToCameraIcon),
                                                       this);

    SelectionContextOperation cameraModeTrigger = [this](const SelectionContext &) {
        QVariantList list;
        for (int i = 0; i < m_viewportToolStates.size(); ++i) {
            Edit3DView::ViewportToolState state = m_viewportToolStates[i];
            if (i == m_activeViewport) {
                bool isChecked = m_cameraModeAction->action()->isChecked();
                state.isPerspective = isChecked;
                setViewportToolState(i, state);
                list.append(isChecked);
            } else {
                list.append(state.isPerspective);
            }
        }
        emitView3DAction(View3DActionType::CameraToggle, list);
    };

    m_cameraModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_CAMERA,
        View3DActionType::Empty,
        Tr::tr("Toggle Perspective/Orthographic Camera Mode"),
        QKeySequence(Qt::Key_T),
        true,
        false,
        toolbarIcon(DesignerIcons::CameraIcon),
        this,
        cameraModeTrigger);

    m_orientationModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_ORIENTATION,
        View3DActionType::OrientationToggle,
        Tr::tr("Toggle Global/Local Orientation"),
        QKeySequence(Qt::Key_Y),
        true,
        false,
        toolbarIcon(DesignerIcons::LocalOrientIcon),
        this);

    m_editLightAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_EDIT_LIGHT,
                                                       View3DActionType::EditLightToggle,
                                                       Tr::tr("Toggle Edit Light On/Off"),
                                                       QKeySequence(Qt::Key_U),
                                                       true,
                                                       false,
                                                       toolbarIcon(DesignerIcons::EditLightIcon),
                                                       this);

    m_showGridAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_GRID,
        View3DActionType::ShowGrid,
        Tr::tr("Show Grid"),
        QKeySequence(Qt::Key_G),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        Tr::tr("Toggle the visibility of the helper grid."));

    m_showLookAtAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_LOOKAT,
        View3DActionType::ShowLookAt,
        Tr::tr("Show Look-at"),
        QKeySequence(Qt::Key_L),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        Tr::tr("Toggle the visibility of the edit camera look-at indicator."));

    m_showSelectionBoxAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_SELECTION_BOX,
        View3DActionType::ShowSelectionBox,
        Tr::tr("Show Selection Boxes"),
        QKeySequence(Qt::Key_B),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        Tr::tr("Toggle the visibility of selection boxes."));

    m_showIconGizmoAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_ICON_GIZMO,
        View3DActionType::ShowIconGizmo,
        Tr::tr("Show Icon Gizmos"),
        QKeySequence(Qt::Key_I),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        Tr::tr(

            "Toggle the visibility of icon gizmos, such as light and camera icons."));

    m_showCameraFrustumAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM,
        View3DActionType::ShowCameraFrustum,
        Tr::tr("Always Show Camera Frustums"),
        QKeySequence(Qt::Key_C),
        true,
        false,
        QIcon(),
        this,
        nullptr,
        Tr::tr("Toggle between always showing the camera frustum visualization and only showing it "
               "when the camera is selected."));

    m_cameraViewAction = std::make_unique<Edit3DCameraViewAction>("CamerView"_actionId,
                                                                  View3DActionType::CameraViewMode,
                                                                  this);

    m_showParticleEmitterAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_PARTICLE_EMITTER,
        View3DActionType::ShowParticleEmitter,
        Tr::tr("Always Show Particle Emitters and Attractors"),
        QKeySequence(Qt::Key_M),
        true,
        false,
        QIcon(),
        this,
        nullptr,
        Tr::tr(
            "Toggle between always showing the particle emitter and attractor visualizations and "
            "only showing them when the emitter or attractor is selected."));

    SelectionContextOperation resetTrigger = [this](const SelectionContext &) {
        m_particlesPlayAction->action()->setEnabled(particlemode);
        m_particlesRestartAction->action()->setEnabled(particlemode);
        if (particlemode)
            m_particlesPlayAction->action()->setChecked(true);
        if (m_seekerAction)
            m_seekerAction->action()->setEnabled(false);
        resetPuppet();
    };

    SelectionContextOperation particlesTrigger = [this](const SelectionContext &) {
        particlemode = !particlemode;
        m_particlesPlayAction->action()->setEnabled(particlemode);
        m_particlesRestartAction->action()->setEnabled(particlemode);
        if (m_seekerAction)
            m_seekerAction->action()->setEnabled(false);
        QmlDesignerPlugin::settings().insert("particleMode", particlemode);
        resetPuppet();
    };

    SelectionContextOperation particlesPlayTrigger = [this](const SelectionContext &) {
        if (m_seekerAction)
            m_seekerAction->action()->setEnabled(!m_particlesPlayAction->action()->isChecked());
    };

    SelectionContextOperation bakeLightsTrigger = [this](const SelectionContext &) {
        if (!m_isBakingLightsSupported)
            return;

        // BakeLights cleans itself up when its dialog is closed
        if (!m_bakeLights) {
            m_bakeLights = new BakeLights(this, m_modulesStorage);
            m_bakeLights->setKitVersion(m_kitVersion);
        } else {
            m_bakeLights->raiseDialog();
        }
    };

    m_particleViewModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLE_MODE,
        View3DActionType::Edit3DParticleModeToggle,
        Tr::tr("Toggle Particle Animation On/Off"),
        QKeySequence(Qt::Key_V),
        true,
        false,
        toolbarIcon(DesignerIcons::ParticlesAnimationIcon),
        this,
        particlesTrigger);

    particlemode = false;
    m_particlesPlayAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_PARTICLES_PLAY,
                                                           View3DActionType::ParticlesPlay,
                                                           Tr::tr("Play Particles"),
                                                           QKeySequence(Qt::Key_Comma),
                                                           true,
                                                           true,
                                                           toolbarIcon(DesignerIcons::ParticlesPlayIcon),
                                                           this,
                                                           particlesPlayTrigger);

    m_particlesRestartAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLES_RESTART,
        View3DActionType::ParticlesRestart,
        Tr::tr("Restart Particles"),
        QKeySequence(Qt::Key_Slash),
        false,
        false,
        toolbarIcon(DesignerIcons::ParticlesRestartIcon),
        this);

    m_particlesPlayAction->action()->setEnabled(particlemode);
    m_particlesRestartAction->action()->setEnabled(particlemode);

    m_resetAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_RESET_VIEW,
                                                   View3DActionType::Empty,
                                                   Tr::tr("Reset View"),
                                                   QKeySequence(Qt::Key_P),
                                                   false,
                                                   false,
                                                   toolbarIcon(DesignerIcons::ResetViewIcon),
                                                   this,
                                                   resetTrigger);

    SelectionContextOperation visibilityTogglesTrigger = [this](const SelectionContext &) {
        if (!edit3DWidget()->visibilityTogglesMenu())
            return;

        edit3DWidget()->showVisibilityTogglesMenu(
            !edit3DWidget()->visibilityTogglesMenu()->isVisible(),
            resolveToolbarPopupPos(m_visibilityTogglesAction.get()));
    };

    m_visibilityTogglesAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_VISIBILITY_TOGGLES,
        View3DActionType::Empty,
        Tr::tr("Visibility Toggles"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::VisibilityIcon),
        this,
        visibilityTogglesTrigger);

    SelectionContextOperation backgroundColorActionsTrigger = [this](const SelectionContext &) {
        if (!edit3DWidget()->backgroundColorMenu())
            return;

        edit3DWidget()->showBackgroundColorMenu(
            !edit3DWidget()->backgroundColorMenu()->isVisible(),
            resolveToolbarPopupPos(m_backgroundColorMenuAction.get()));
    };

    m_backgroundColorMenuAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_BACKGROUND_COLOR_ACTIONS,
        View3DActionType::Empty,
        Tr::tr("Background Color Actions"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::EditColorIcon),
        this,
        backgroundColorActionsTrigger);

    createSeekerSliderAction();

    m_bakeLightsAction = std::make_unique<Edit3DBakeLightsAction>(
        toolbarIcon(DesignerIcons::BakeLightIcon),
        this,
        bakeLightsTrigger);

    SelectionContextOperation snapToggleTrigger = [this](const SelectionContext &) {
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_ENABLED,
                               m_snapToggleAction->action()->isChecked());
        syncSnapAuxPropsToSettings();
    };

    m_snapToggleAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_SNAP_TOGGLE,
        View3DActionType::Empty,
        Tr::tr("Toggle Snapping During Node Drag"),
        QKeySequence(Qt::SHIFT | Qt::Key_Tab),
        true,
        Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ENABLED, false).toBool(),
        toolbarIcon(DesignerIcons::SnappingIcon),
        this,
        snapToggleTrigger);

    SelectionContextOperation snapConfigTrigger = [this](const SelectionContext &) {
        if (!m_snapConfiguration) {
            m_snapConfiguration = new SnapConfiguration(this);
            connect(m_snapConfiguration.data(), &SnapConfiguration::posIntChanged,
                    this, [this] {
                // Notify every change of position interval as that causes visible changes in grid
                rootModelNode().setAuxiliaryData(edit3dSnapPosIntProperty,
                                                 m_snapConfiguration->posInt());
            });
        }
        m_snapConfiguration->showConfigDialog(resolveToolbarPopupPos(m_snapConfigAction.get()));
    };

    m_snapConfigAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_SNAP_CONFIG,
                                                        View3DActionType::Empty,
                                                        Tr::tr("Open Snap Configuration"),
                                                        QKeySequence(),
                                                        false,
                                                        false,
                                                        toolbarIcon(DesignerIcons::SnappingConfIcon),
                                                        this,
                                                        snapConfigTrigger);

    SelectionContextOperation viewportPresetsActionTrigger = [this](const SelectionContext &) {
        if (!edit3DWidget()->viewportPresetsMenu())
            return;

        edit3DWidget()->showViewportPresetsMenu(
            !edit3DWidget()->viewportPresetsMenu()->isVisible(),
            resolveToolbarPopupPos(m_viewportPresetsMenuAction.get()));
    };

    m_viewportPresetsMenuAction = std::make_unique<Edit3DAction>(QmlDesigner::Constants::EDIT3D_PRESETS,
                                                       View3DActionType::Empty,
                                                       Tr::tr("Show Viewport Modes"),
                                                       QKeySequence(),
                                                       false,
                                                       false,
                                                       toolbarIcon(DesignerIcons::MultiViewPortIcon),
                                                       this,
                                                       viewportPresetsActionTrigger);

    SelectionContextOperation cameraSpeedConfigTrigger = [this](const SelectionContext &) {
        if (!m_cameraSpeedConfiguration) {
            m_cameraSpeedConfiguration = new CameraSpeedConfiguration(this);
            connect(m_cameraSpeedConfiguration.data(), &CameraSpeedConfiguration::totalSpeedChanged,
                    this, [this] {
                        setCameraSpeedAuxData(m_cameraSpeedConfiguration->speed(),
                                              m_cameraSpeedConfiguration->multiplier());
                    });
            connect(m_cameraSpeedConfiguration.data(), &CameraSpeedConfiguration::accessibilityOpened,
                    this, [this] {
                        m_cameraSpeedConfigAction->setIndicator(false);
                    });
        }
        m_cameraSpeedConfiguration->showConfigDialog(resolveToolbarPopupPos(m_cameraSpeedConfigAction.get()));
    };

    m_cameraSpeedConfigAction = std::make_unique<Edit3DIndicatorButtonAction>(
        QmlDesigner::Constants::EDIT3D_CAMERA_SPEED_CONFIG,
        View3DActionType::Empty,
        Tr::tr("Open Camera Speed Configuration"),
        toolbarIcon(DesignerIcons::CameraSpeedConfigIcon),
        cameraSpeedConfigTrigger,
        this);

    m_cameraSpeedConfigAction->setIndicator(!isQDSTrusted());

    m_leftActions << m_selectionModeAction.get();
    m_leftActions << nullptr; // Null indicates separator
    m_leftActions << nullptr; // Second null after separator indicates an exclusive group
    m_leftActions << m_moveToolAction.get();
    m_leftActions << m_rotateToolAction.get();
    m_leftActions << m_scaleToolAction.get();
    m_leftActions << nullptr;
    m_leftActions << m_fitAction.get();
    m_leftActions << nullptr;
    m_leftActions << m_cameraModeAction.get();
    m_leftActions << m_orientationModeAction.get();
    m_leftActions << m_editLightAction.get();
    m_leftActions << nullptr;
    m_leftActions << m_snapToggleAction.get();
    m_leftActions << m_snapConfigAction.get();
    m_leftActions << nullptr;
    m_leftActions << m_alignCamerasAction.get();
    m_leftActions << m_alignViewAction.get();
    m_leftActions << m_cameraSpeedConfigAction.get();
    m_leftActions << nullptr;
    m_leftActions << m_visibilityTogglesAction.get();
    m_leftActions << m_backgroundColorMenuAction.get();
    m_leftActions << m_viewportPresetsMenuAction.get();

    m_rightActions << m_particleViewModeAction.get();
    m_rightActions << m_particlesPlayAction.get();
    m_rightActions << m_particlesRestartAction.get();
    m_rightActions << nullptr;
    m_rightActions << m_seekerAction.get();
    m_rightActions << nullptr;
    m_rightActions << m_bakeLightsAction.get();
    m_rightActions << m_resetAction.get();

    m_visibilityToggleActions << m_showGridAction.get();
    m_visibilityToggleActions << m_showLookAtAction.get();
    m_visibilityToggleActions << m_showSelectionBoxAction.get();
    m_visibilityToggleActions << m_showIconGizmoAction.get();
    m_visibilityToggleActions << m_showCameraFrustumAction.get();
    m_visibilityToggleActions << m_showParticleEmitterAction.get();
    m_visibilityToggleActions << m_cameraViewAction.get();

    createSyncEnvBackgroundAction();
    createSelectBackgroundColorAction(m_syncEnvBackgroundAction->action());
    createGridColorSelectionAction();
    createResetColorAction(m_syncEnvBackgroundAction->action());

    m_backgroundColorActions << m_selectBackgroundColorAction.get();
    m_backgroundColorActions << m_selectGridColorAction.get();
    m_backgroundColorActions << m_syncEnvBackgroundAction.get();
    m_backgroundColorActions << m_resetColorAction.get();

    createViewportPresetActions();
}

QVector<Edit3DAction *> Edit3DView::leftActions() const
{
    return m_leftActions;
}

QVector<Edit3DAction *> Edit3DView::rightActions() const
{
    return m_rightActions;
}

QVector<Edit3DAction *> Edit3DView::visibilityToggleActions() const
{
    return m_visibilityToggleActions;
}

QVector<Edit3DAction *> Edit3DView::backgroundColorActions() const
{
    return m_backgroundColorActions;
}

QVector<Edit3DAction *> Edit3DView::viewportPresetActions() const
{
    return m_viewportPresetActions;
}

Edit3DAction *Edit3DView::edit3DAction(View3DActionType type) const
{
    return m_edit3DActions.value(type, nullptr);
}

Edit3DBakeLightsAction *Edit3DView::bakeLightsAction() const
{
    return m_bakeLightsAction.get();
}

// This method is called upon right-clicking the view to prepare for context-menu creation. The actual
// context menu is created when nodeAtPosReady() is received from puppet
void Edit3DView::startContextMenu(const QPoint &pos)
{
    m_contextMenuPosMouse = pos;
    m_nodeAtPosReqType = NodeAtPosReqType::ContextMenu;
}

void Edit3DView::dropMaterial(const ModelNode &matNode, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::MaterialDrop;
    m_droppedModelNode = matNode;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

void Edit3DView::dropBundleMaterial(const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::BundleMaterialDrop;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

void Edit3DView::dropBundleItem(const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::BundleEffectDrop;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

void Edit3DView::dropTexture(const ModelNode &textureNode, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::TextureDrop;
    m_droppedModelNode = textureNode;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

void Edit3DView::dropComponent(const ItemLibraryEntry &entry, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::ComponentDrop;
    m_droppedEntry = entry;
    NodeMetaInfo metaInfo = model()->metaInfo(entry.typeName());
    if (metaInfo.isQtQuick3DNode())
        emitView3DAction(View3DActionType::GetNodeAtPos, pos);
    else
        nodeAtPosReady({}, {}); // No need to actually resolve position for non-node items
}

void QmlDesigner::Edit3DView::dropAssets(const QList<QUrl> &urls, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::AssetDrop;
    m_dropped3dImports.clear();

    for (const QUrl &url : urls) {
        Asset asset(url.toLocalFile());
        // For textures we only support single drops
        if (m_dropped3dImports.isEmpty() && asset.isValidTextureSource()) {
            m_droppedTexture = asset.id();
            break;
        } else if (asset.isImported3D()) {
            m_dropped3dImports.append(asset.id());
        }
    }

    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

bool Edit3DView::isBakingLightsSupported() const
{
    return m_isBakingLightsSupported;
}

} // namespace QmlDesigner
