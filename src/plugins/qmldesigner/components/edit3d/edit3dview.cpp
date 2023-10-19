// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dview.h"

#include "backgroundcolorselection.h"
#include "bakelights.h"
#include "designeractionmanager.h"
#include "designericons.h"
#include "designersettings.h"
#include "designmodecontext.h"
#include "edit3dcanvas.h"
#include "edit3dviewconfig.h"
#include "edit3dwidget.h"
#include "materialutils.h"
#include "metainfo.h"
#include "nodeabstractproperty.h"
#include "nodehints.h"
#include "nodeinstanceview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "qmlvisualnode.h"
#include "seekerslider.h"
#include "snapconfiguration.h"

#include <model/modelutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QToolButton>

namespace QmlDesigner {

inline static QIcon contextIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().contextIcon(iconId);
};

inline static QIcon toolbarIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().toolbarIcon(iconId);
};

Edit3DView::Edit3DView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
    m_compressionTimer.setInterval(1000);
    m_compressionTimer.setSingleShot(true);
    connect(&m_compressionTimer, &QTimer::timeout, this, &Edit3DView::handleEntriesChanged);
}

void Edit3DView::createEdit3DWidget()
{
    createEdit3DActions();
    m_edit3DWidget = new Edit3DWidget(this);

    auto editor3DContext = new Internal::Editor3DContext(m_edit3DWidget.data());
    Core::ICore::addContextObject(editor3DContext);
}

void Edit3DView::checkImports()
{
    edit3DWidget()->showCanvas(model()->hasImport("QtQuick3D"));
}

WidgetInfo Edit3DView::widgetInfo()
{
    if (!m_edit3DWidget)
        createEdit3DWidget();

    return createWidgetInfo(m_edit3DWidget.data(),
                            "Editor3D",
                            WidgetInfo::CentralPane,
                            0,
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

    // Notify puppet to resize if received image wasn't correct size
    if (img.size() != canvasSize())
        edit3DViewResized(canvasSize());
    if (edit3DWidget()->canvas()->busyIndicator()->isVisible()) {
        edit3DWidget()->canvas()->setOpacity(1.0);
        edit3DWidget()->canvas()->busyIndicator()->hide();
    }
}

void Edit3DView::updateActiveScene3D(const QVariantMap &sceneState)
{
    const QString sceneKey           = QStringLiteral("sceneInstanceId");
    const QString selectKey          = QStringLiteral("selectionMode");
    const QString transformKey       = QStringLiteral("transformMode");
    const QString perspectiveKey     = QStringLiteral("usePerspective");
    const QString orientationKey     = QStringLiteral("globalOrientation");
    const QString editLightKey       = QStringLiteral("showEditLight");
    const QString gridKey            = QStringLiteral("showGrid");
    const QString selectionBoxKey    = QStringLiteral("showSelectionBox");
    const QString iconGizmoKey       = QStringLiteral("showIconGizmo");
    const QString cameraFrustumKey   = QStringLiteral("showCameraFrustum");
    const QString particleEmitterKey = QStringLiteral("showParticleEmitter");
    const QString particlesPlayKey   = QStringLiteral("particlePlay");
    const QString syncEnvBgKey       = QStringLiteral("syncEnvBackground");

    if (sceneState.contains(sceneKey)) {
        qint32 newActiveScene = sceneState[sceneKey].value<qint32>();
        edit3DWidget()->canvas()->updateActiveScene(newActiveScene);
        model()->setActive3DSceneId(newActiveScene);
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

    if (sceneState.contains(perspectiveKey))
        m_cameraModeAction->action()->setChecked(sceneState[perspectiveKey].toBool());
    else
        m_cameraModeAction->action()->setChecked(false);

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

    if (sceneState.contains(particleEmitterKey))
        m_showParticleEmitterAction->action()->setChecked(sceneState[particleEmitterKey].toBool());
    else
        m_showParticleEmitterAction->action()->setChecked(false);

    if (sceneState.contains(particlesPlayKey))
        m_particlesPlayAction->action()->setChecked(sceneState[particlesPlayKey].toBool());
    else
        m_particlesPlayAction->action()->setChecked(true);

    // Syncing background color only makes sense for children of View3D instances
    bool syncValue = false;
    bool syncEnabled = false;
    bool desiredSyncValue = false;
    if (sceneState.contains(syncEnvBgKey))
        desiredSyncValue = sceneState[syncEnvBgKey].toBool();
    ModelNode checkNode = active3DSceneNode();
    const bool activeSceneValid = checkNode.isValid();

    while (checkNode.isValid()) {
        if (checkNode.metaInfo().isQtQuick3DView3D()) {
            syncValue = desiredSyncValue;
            syncEnabled = true;
            break;
        }
        if (checkNode.hasParentProperty())
            checkNode = checkNode.parentProperty().parentModelNode();
        else
            break;
    }

    if (activeSceneValid && syncValue != desiredSyncValue) {
        // Update actual toolstate as well if we overrode it.
        QTimer::singleShot(0, this, [this, syncValue]() {
            emitView3DAction(View3DActionType::SyncEnvBackground, syncValue);
        });
    }

    m_syncEnvBackgroundAction->action()->setChecked(syncValue);
    m_syncEnvBackgroundAction->action()->setEnabled(syncEnabled);

    // Selection context change updates visible and enabled states
    SelectionContext selectionContext(this);
    selectionContext.setUpdateMode(SelectionContext::UpdateMode::Fast);
    if (m_bakeLightsAction)
        m_bakeLightsAction->currentContextChanged(selectionContext);
}

void Edit3DView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

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
    ProjectExplorer::Target *target = QmlDesignerPlugin::instance()->currentDesignDocument()->currentTarget();
    if (target && target->kit()) {
        if (QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(target->kit()))
            m_isBakingLightsSupported = qtVer->qtVersion() >= QVersionNumber(6, 5, 0);
    }

    connect(model->metaInfo().itemLibraryInfo(), &ItemLibraryInfo::entriesChanged, this,
            &Edit3DView::onEntriesChanged, Qt::UniqueConnection);
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
        EK_primitives,
        EK_importedModels
    };

    QMap<ItemLibraryEntryKeys, ItemLibraryDetails> entriesMap {
        {EK_cameras, {tr("Cameras"), contextIcon(DesignerIcons::CameraIcon)}},
        {EK_lights, {tr("Lights"), contextIcon(DesignerIcons::LightIcon)}},
        {EK_primitives, {tr("Primitives"), contextIcon(DesignerIcons::PrimitivesIcon)}},
        {EK_importedModels, {tr("Imported Models"), contextIcon(DesignerIcons::ImportedModelsIcon)}}
    };

    const QList<ItemLibraryEntry> itemLibEntries = model()->metaInfo().itemLibraryInfo()->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        ItemLibraryEntryKeys entryKey;
        if (entry.typeName() == "QtQuick3D.Model" && entry.name() != "Empty") {
            entryKey = EK_primitives;
        } else if (entry.typeName() == "QtQuick3D.DirectionalLight"
                || entry.typeName() == "QtQuick3D.PointLight"
                || entry.typeName() == "QtQuick3D.SpotLight") {
            entryKey = EK_lights;
        } else if (entry.typeName() == "QtQuick3D.OrthographicCamera"
                || entry.typeName() == "QtQuick3D.PerspectiveCamera") {
            entryKey = EK_cameras;
        } else if (entry.typeName().startsWith("Quick3DAssets.")
                   && NodeHints::fromItemLibraryEntry(entry).canBeDroppedInView3D()) {
            entryKey = EK_importedModels;
        } else {
            continue;
        }
        entriesMap[entryKey].entryList.append(entry);
    }

    m_edit3DWidget->updateCreateSubMenu(entriesMap.values());
}

void Edit3DView::updateAlignActionStates()
{
    bool enabled = false;

    ModelNode activeScene = active3DSceneNode();
    if (activeScene.isValid()) {
        const QList<ModelNode> nodes = activeScene.allSubModelNodes();
        enabled = ::Utils::anyOf(nodes, [](const ModelNode &node) {
            return node.metaInfo().isQtQuick3DCamera();
        });
    }

    m_alignCamerasAction->action()->setEnabled(enabled);
    m_alignViewAction->action()->setEnabled(enabled);
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
    if (identifier == "asset_import_update")
        resetPuppet();
}

/**
 * @brief Get node at position from puppet process
 *
 * Response from puppet process for the model at requested position
 *
 * @param modelNode Node picked at the requested position or invalid node if nothing could be picked
 * @param pos3d 3D scene position of the requested view position
 */
void Edit3DView::nodeAtPosReady(const ModelNode &modelNode, const QVector3D &pos3d)
{
    if (m_nodeAtPosReqType == NodeAtPosReqType::ContextMenu) {
        // Make sure right-clicked item is selected. Due to a bug in puppet side right-clicking an item
        // while the context-menu is shown doesn't select the item.
        if (modelNode.isValid() && !modelNode.isSelected())
            setSelectedModelNode(modelNode);
        m_edit3DWidget->showContextMenu(m_contextMenuPos, modelNode, pos3d);
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::ComponentDrop) {
        ModelNode createdNode;
        executeInTransaction(__FUNCTION__, [&] {
            createdNode = QmlVisualNode::createQml3DNode(
                this, m_droppedEntry, edit3DWidget()->canvas()->activeScene(), pos3d).modelNode();
            if (createdNode.metaInfo().isQtQuick3DModel())
                MaterialUtils::assignMaterialTo3dModel(this, createdNode);
        });
        if (createdNode.isValid())
            setSelectedModelNode(createdNode);
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::MaterialDrop) {
        bool isModel = modelNode.metaInfo().isQtQuick3DModel();
        if (m_droppedModelNode.isValid() && isModel) {
            executeInTransaction(__FUNCTION__, [&] {
                MaterialUtils::assignMaterialTo3dModel(this, modelNode, m_droppedModelNode);
            });
        }
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::BundleMaterialDrop) {
        emitCustomNotification("drop_bundle_material", {modelNode}); // To ContentLibraryView
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::BundleEffectDrop) {
        emitCustomNotification("drop_bundle_effect", {modelNode}, {pos3d}); // To ContentLibraryView
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::TextureDrop) {
        emitCustomNotification("apply_texture_to_model3D", {modelNode, m_droppedModelNode});
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::AssetDrop) {
        bool isModel = modelNode.metaInfo().isQtQuick3DModel();
        if (!m_droppedFile.isEmpty() && isModel)
            emitCustomNotification("apply_asset_to_model3D", {modelNode}, {m_droppedFile}); // To MaterialBrowserView
    }

    m_droppedModelNode = {};
    m_droppedFile.clear();
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

void Edit3DView::sendInputEvent(QInputEvent *e) const
{
    if (nodeInstanceView())
        nodeInstanceView()->sendInputEvent(e);
}

void Edit3DView::edit3DViewResized(const QSize &size) const
{
    if (nodeInstanceView())
        nodeInstanceView()->edit3DViewResized(size);
}

QSize Edit3DView::canvasSize() const
{
    if (!m_edit3DWidget.isNull() && m_edit3DWidget->canvas())
        return m_edit3DWidget->canvas()->size();

    return {};
}

void Edit3DView::createSelectBackgroundColorAction(QAction *syncEnvBackgroundAction)
{
    QString description = QCoreApplication::translate("SelectBackgroundColorAction",
                                                      "Select Background Color");
    QString tooltip = QCoreApplication::translate("SelectBackgroundColorAction",
                                                  "Select a color for the background of the 3D view.");

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
    QString description = QCoreApplication::translate("SelectGridColorAction", "Select Grid Color");
    QString tooltip = QCoreApplication::translate("SelectGridColorAction",
                                                  "Select a color for the grid lines of the 3D view.");

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
    QString description = QCoreApplication::translate("ResetEdit3DColorsAction", "Reset Colors");
    QString tooltip = QCoreApplication::translate("ResetEdit3DColorsAction",
                                                  "Reset the background color and the color of the "
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
    QString description = QCoreApplication::translate("SyncEnvBackgroundAction",
                                                      "Use Scene Environment");
    QString tooltip = QCoreApplication::translate("SyncEnvBackgroundAction",
                                                  "Sets the 3D view to use the Scene Environment "
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

void Edit3DView::createSeekerSliderAction()
{
    m_seekerAction = std::make_unique<Edit3DParticleSeekerAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLES_SEEKER,
        View3DActionType::ParticlesSeek,
        this);

    m_seekerAction->action()->setEnabled(false);
    m_seekerAction->action()->setToolTip(QLatin1String("Seek particle system time when paused."));

    connect(m_seekerAction->seekerAction(),
            &SeekerSliderAction::valueChanged,
            this, [=] (int value) {
        this->emitView3DAction(View3DActionType::ParticlesSeek, value);
            });
}

QPoint Edit3DView::resolveToolbarPopupPos(Edit3DAction *action) const
{
    QPoint pos;
    const QList<QObject *> &objects = action->action()->associatedObjects();
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

void Edit3DView::createEdit3DActions()
{
    m_selectionModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_SELECTION_MODE,
        View3DActionType::SelectionModeToggle,
        QCoreApplication::translate("SelectionModeToggleAction",
                                    "Toggle Group/Single Selection Mode"),
        QKeySequence(Qt::Key_Q),
        true,
        false,
        toolbarIcon(DesignerIcons::ToggleGroupIcon),
        this);

    m_moveToolAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_MOVE_TOOL,
        View3DActionType::MoveTool,
        QCoreApplication::translate("MoveToolAction",
                                    "Activate Move Tool"),
        QKeySequence(Qt::Key_W),
        true,
        true,
        toolbarIcon(DesignerIcons::MoveToolIcon),
        this);

    m_rotateToolAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_ROTATE_TOOL,
        View3DActionType::RotateTool,
        QCoreApplication::translate("RotateToolAction",
                                    "Activate Rotate Tool"),
        QKeySequence(Qt::Key_E),
        true,
        false,
        toolbarIcon(DesignerIcons::RotateToolIcon),
        this);

    m_scaleToolAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_SCALE_TOOL,
        View3DActionType::ScaleTool,
        QCoreApplication::translate("ScaleToolAction",
                                    "Activate Scale Tool"),
        QKeySequence(Qt::Key_R),
        true,
        false,
        toolbarIcon(DesignerIcons::ScaleToolIcon),
        this);

    m_fitAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_FIT_SELECTED,
        View3DActionType::FitToView,
        QCoreApplication::translate("FitToViewAction",
                                    "Fit Selected Object to View"),
        QKeySequence(Qt::Key_F),
        false,
        false,
        toolbarIcon(DesignerIcons::FitToViewIcon),
        this);

    m_alignCamerasAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_ALIGN_CAMERAS,
        View3DActionType::AlignCamerasToView,
        QCoreApplication::translate("AlignCamerasToViewAction",
                                    "Align Cameras to View"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::AlignCameraToViewIcon),
        this);

    m_alignViewAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_ALIGN_VIEW,
        View3DActionType::AlignViewToCamera,
        QCoreApplication::translate("AlignViewToCameraAction",
                                    "Align View to Camera"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::AlignViewToCameraIcon),
        this);

    m_cameraModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_CAMERA,
        View3DActionType::CameraToggle,
        QCoreApplication::translate("CameraToggleAction",
                                    "Toggle Perspective/Orthographic Camera Mode"),
        QKeySequence(Qt::Key_T),
        true,
        false,
        toolbarIcon(DesignerIcons::CameraIcon),
        this);

    m_orientationModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_ORIENTATION,
        View3DActionType::OrientationToggle,
        QCoreApplication::translate("OrientationToggleAction",
                                    "Toggle Global/Local Orientation"),
        QKeySequence(Qt::Key_Y),
        true,
        false,
        toolbarIcon(DesignerIcons::LocalOrientIcon),
        this);

    m_editLightAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_LIGHT,
        View3DActionType::EditLightToggle,
        QCoreApplication::translate("EditLightToggleAction",
                                    "Toggle Edit Light On/Off"),
        QKeySequence(Qt::Key_U),
        true,
        false,
        toolbarIcon(DesignerIcons::EditLightIcon),
        this);

    m_showGridAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_GRID,
        View3DActionType::ShowGrid,
        QCoreApplication::translate("ShowGridAction", "Show Grid"),
        QKeySequence(Qt::Key_G),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        QCoreApplication::translate("ShowGridAction", "Toggle the visibility of the helper grid."));

    m_showSelectionBoxAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_SELECTION_BOX,
        View3DActionType::ShowSelectionBox,
        QCoreApplication::translate("ShowSelectionBoxAction", "Show Selection Boxes"),
        QKeySequence(Qt::Key_S),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        QCoreApplication::translate("ShowSelectionBoxAction",
                                    "Toggle the visibility of selection boxes."));

    m_showIconGizmoAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_ICON_GIZMO,
        View3DActionType::ShowIconGizmo,
        QCoreApplication::translate("ShowIconGizmoAction", "Show Icon Gizmos"),
        QKeySequence(Qt::Key_I),
        true,
        true,
        QIcon(),
        this,
        nullptr,
        QCoreApplication::translate(
            "ShowIconGizmoAction",
            "Toggle the visibility of icon gizmos, such as light and camera icons."));

    m_showCameraFrustumAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM,
        View3DActionType::ShowCameraFrustum,
        QCoreApplication::translate("ShowCameraFrustumAction", "Always Show Camera Frustums"),
        QKeySequence(Qt::Key_C),
        true,
        false,
        QIcon(),
        this,
        nullptr,
        QCoreApplication::translate(
            "ShowCameraFrustumAction",
            "Toggle between always showing the camera frustum visualization and only showing it "
            "when the camera is selected."));

    m_showParticleEmitterAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_PARTICLE_EMITTER,
        View3DActionType::ShowParticleEmitter,
        QCoreApplication::translate("ShowParticleEmitterAction",
                                    "Always Show Particle Emitters And Attractors"),
        QKeySequence(Qt::Key_M),
        true,
        false,
        QIcon(),
        this,
        nullptr,
        QCoreApplication::translate(
            "ShowParticleEmitterAction",
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
        if (!m_bakeLights)
            m_bakeLights = new BakeLights(this);
        else
            m_bakeLights->raiseDialog();
    };

    m_particleViewModeAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLE_MODE,
        View3DActionType::Edit3DParticleModeToggle,
        QCoreApplication::translate("ParticleViewModeAction",
                                    "Toggle particle animation On/Off"),
        QKeySequence(Qt::Key_V),
        true,
        false,
        toolbarIcon(DesignerIcons::ParticlesAnimationIcon),
        this,
        particlesTrigger);

    particlemode = false;
    m_particlesPlayAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLES_PLAY,
        View3DActionType::ParticlesPlay,
        QCoreApplication::translate("ParticlesPlayAction",
                                    "Play Particles"),
        QKeySequence(Qt::Key_Comma),
        true,
        true,
        toolbarIcon(DesignerIcons::ParticlesPlayIcon),
        this,
        particlesPlayTrigger);

    m_particlesRestartAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_PARTICLES_RESTART,
        View3DActionType::ParticlesRestart,
        QCoreApplication::translate("ParticlesRestartAction",
                                    "Restart Particles"),
        QKeySequence(Qt::Key_Slash),
        false,
        false,
        toolbarIcon(DesignerIcons::ParticlesRestartIcon),
        this);

    m_particlesPlayAction->action()->setEnabled(particlemode);
    m_particlesRestartAction->action()->setEnabled(particlemode);

    m_resetAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_RESET_VIEW,
        View3DActionType::Empty,
        QCoreApplication::translate("ResetView", "Reset View"),
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
        QCoreApplication::translate("VisibilityTogglesAction",
                                    "Visibility Toggles"),
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
        QCoreApplication::translate("BackgroundColorMenuActions",
                                    "Background Color Actions"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::EditColorIcon),
        this,
        backgroundColorActionsTrigger);

    createSeekerSliderAction();

    m_bakeLightsAction = std::make_unique<Edit3DBakeLightsAction>(
        toolbarIcon(DesignerIcons::EditLightIcon), //: TODO placeholder icon
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
        QCoreApplication::translate("SnapToggleAction", "Toggle snapping during node drag"),
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
                    this, [this]() {
                // Notify every change of position interval as that causes visible changes in grid
                rootModelNode().setAuxiliaryData(edit3dSnapPosIntProperty,
                                                 m_snapConfiguration->posInt());
            });
        }
        m_snapConfiguration->showConfigDialog(resolveToolbarPopupPos(m_snapConfigAction.get()));
    };

    m_snapConfigAction = std::make_unique<Edit3DAction>(
        QmlDesigner::Constants::EDIT3D_SNAP_CONFIG,
        View3DActionType::Empty,
        QCoreApplication::translate("SnapConfigAction", "Open snap configuration dialog"),
        QKeySequence(),
        false,
        false,
        toolbarIcon(DesignerIcons::SnappingConfIcon),
        this,
        snapConfigTrigger);

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
    m_leftActions << nullptr;
    m_leftActions << m_visibilityTogglesAction.get();
    m_leftActions << m_backgroundColorMenuAction.get();

    m_rightActions << m_particleViewModeAction.get();
    m_rightActions << m_particlesPlayAction.get();
    m_rightActions << m_particlesRestartAction.get();
    m_rightActions << nullptr;
    m_rightActions << m_seekerAction.get();
    m_rightActions << nullptr;
    m_rightActions << m_bakeLightsAction.get();
    m_rightActions << m_resetAction.get();

    m_visibilityToggleActions << m_showGridAction.get();
    m_visibilityToggleActions << m_showSelectionBoxAction.get();
    m_visibilityToggleActions << m_showIconGizmoAction.get();
    m_visibilityToggleActions << m_showCameraFrustumAction.get();
    m_visibilityToggleActions << m_showParticleEmitterAction.get();

    createSyncEnvBackgroundAction();
    createSelectBackgroundColorAction(m_syncEnvBackgroundAction->action());
    createGridColorSelectionAction();
    createResetColorAction(m_syncEnvBackgroundAction->action());

    m_backgroundColorActions << m_selectBackgroundColorAction.get();
    m_backgroundColorActions << m_selectGridColorAction.get();
    m_backgroundColorActions << m_syncEnvBackgroundAction.get();
    m_backgroundColorActions << m_resetColorAction.get();
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


Edit3DAction *Edit3DView::edit3DAction(View3DActionType type) const
{
    return m_edit3DActions.value(type, nullptr);
}

Edit3DBakeLightsAction *Edit3DView::bakeLightsAction() const
{
    return m_bakeLightsAction.get();
}

void Edit3DView::addQuick3DImport()
{
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (document && !document->inFileComponentModelActive() && model()
        && ModelUtils::addImportWithCheck(
            "QtQuick3D",
            [](const Import &import) { return !import.hasVersion() || import.majorVersion() >= 6; },
            model())) {
        return;
    }
    Core::AsynchronousMessageBox::warning(tr("Failed to Add Import"),
                                          tr("Could not add QtQuick3D import to project."));
}

// This method is called upon right-clicking the view to prepare for context-menu creation. The actual
// context menu is created when nodeAtPosReady() is received from puppet
void Edit3DView::startContextMenu(const QPoint &pos)
{
    m_contextMenuPos = pos;
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

void Edit3DView::dropBundleEffect(const QPointF &pos)
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

void Edit3DView::dropAsset(const QString &file, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::AssetDrop;
    m_droppedFile = file;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

bool Edit3DView::isBakingLightsSupported() const
{
    return m_isBakingLightsSupported;
}

} // namespace QmlDesigner
