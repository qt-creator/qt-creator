// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dview.h"

#include "backgroundcolorselection.h"
#include "designersettings.h"
#include "designmodecontext.h"
#include "edit3dactions.h"
#include "edit3dcanvas.h"
#include "edit3dviewconfig.h"
#include "edit3dwidget.h"
#include "metainfo.h"
#include "nodehints.h"
#include "nodeinstanceview.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"
#include "qmldesignerplugin.h"
#include "seekerslider.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QToolButton>

namespace QmlDesigner {

Edit3DView::Edit3DView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
    m_compressionTimer.setInterval(1000);
    m_compressionTimer.setSingleShot(true);
    connect(&m_compressionTimer, &QTimer::timeout, this, &Edit3DView::handleEntriesChanged);
}

Edit3DView::~Edit3DView()
{
    qDeleteAll(m_edit3DActions);
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

    return createWidgetInfo(m_edit3DWidget.data(), "Editor3D", WidgetInfo::CentralPane, 0, tr("3D"), DesignerWidgetFlags::IgnoreErrors);
}

Edit3DWidget *Edit3DView::edit3DWidget() const
{
    return m_edit3DWidget.data();
}

void Edit3DView::selectedNodesChanged([[maybe_unused]] const QList<ModelNode> &selectedNodeList,
                                      [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    SelectionContext selectionContext(this);
    selectionContext.setUpdateMode(SelectionContext::UpdateMode::Fast);
    if (m_alignCamerasAction)
        m_alignCamerasAction->currentContextChanged(selectionContext);
    if (m_alignViewAction)
        m_alignViewAction->currentContextChanged(selectionContext);
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

    if (sceneState.contains(sceneKey)) {
        qint32 newActiveScene = sceneState[sceneKey].value<qint32>();
        edit3DWidget()->canvas()->updateActiveScene(newActiveScene);
        model()->setActive3DSceneId(newActiveScene);
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
}

void Edit3DView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    checkImports();
    auto cachedImage = m_canvasCache.take(model);
    if (cachedImage) {
        edit3DWidget()->canvas()->updateRenderImage(*cachedImage);
        edit3DWidget()->canvas()->setOpacity(0.5);
    }

    edit3DWidget()->canvas()->busyIndicator()->show();

    connect(model->metaInfo().itemLibraryInfo(), &ItemLibraryInfo::entriesChanged, this,
            &Edit3DView::onEntriesChanged, Qt::UniqueConnection);
}

void Edit3DView::onEntriesChanged()
{
    m_compressionTimer.start();
}

void Edit3DView::registerEdit3DAction(Edit3DAction *action)
{
    View3DActionType actionType = action->actionType();
    if (actionType == View3DActionType::Empty)
        return;

    if (m_edit3DActions.contains(actionType)) {
        Edit3DAction *formerAction = m_edit3DActions.value(actionType);
        if (formerAction == action)
            return;

        qWarning() << Q_FUNC_INFO << __LINE__ << "Reregistering action for" << int(actionType);
        delete formerAction;
    }

    m_edit3DActions.insert(actionType, action);
}

void Edit3DView::unregisterEdit3DAction(Edit3DAction *action)
{
    View3DActionType actionType = action->actionType();
    if (m_edit3DActions.value(actionType, nullptr) == action)
        m_edit3DActions.remove(actionType);
}

void Edit3DView::handleEntriesChanged()
{
    if (!model())
        return;

    const QString cameras = tr("Cameras");
    const QString lights = tr("Lights");
    const QString primitives = tr("Primitives");
    const QString importedModels = tr("Imported Models");
    const QStringList keys {cameras, lights, primitives, importedModels}; // used to maintain order

    QHash<QString, QList<ItemLibraryEntry>> entriesMap {
        {cameras, {}},
        {lights, {}},
        {primitives, {}},
        {importedModels, {}}
    };

    const QList<ItemLibraryEntry> itemLibEntries = model()->metaInfo().itemLibraryInfo()->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        if (entry.typeName() == "QtQuick3D.Model" && entry.name() != "Empty") {
            entriesMap[primitives].append(entry);
        } else if (entry.typeName() == "QtQuick3D.DirectionalLight"
                || entry.typeName() == "QtQuick3D.PointLight"
                || entry.typeName() == "QtQuick3D.SpotLight") {
            entriesMap[lights].append(entry);
        } else if (entry.typeName() == "QtQuick3D.OrthographicCamera"
                || entry.typeName() == "QtQuick3D.PerspectiveCamera") {
            entriesMap[cameras].append(entry);
        } else if (entry.typeName().startsWith("Quick3DAssets.")
                   && NodeHints::fromItemLibraryEntry(entry).canBeDroppedInView3D()) {
            entriesMap[importedModels].append(entry);
        }
    }

    m_edit3DWidget->updateCreateSubMenu(keys, entriesMap);
}

void Edit3DView::modelAboutToBeDetached(Model *model)
{
    // Hide the canvas when model is detached (i.e. changing documents)
    if (edit3DWidget() && edit3DWidget()->canvas()) {
        m_canvasCache.insert(model, edit3DWidget()->canvas()->renderImage());
        edit3DWidget()->showCanvas(false);
    }

    AbstractView::modelAboutToBeDetached(model);
}

void Edit3DView::importsChanged([[maybe_unused]] const QList<Import> &addedImports,
                                [[maybe_unused]] const QList<Import> &removedImports)
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
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::MaterialDrop) {
        bool isModel = modelNode.metaInfo().isQtQuick3DModel();
        if (m_droppedModelNode.isValid() && modelNode.isValid() && isModel) {
            executeInTransaction(__FUNCTION__, [&] {
                assignMaterialTo3dModel(modelNode, m_droppedModelNode);
            });
        }
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::BundleMaterialDrop) {
        emitCustomNotification("drop_bundle_material", {modelNode}); // To ContentLibraryView
    } else if (m_nodeAtPosReqType == NodeAtPosReqType::TextureDrop) {
        emitCustomNotification("apply_texture_to_model3D", {modelNode, m_droppedModelNode});
    }

    m_droppedModelNode = {};
    m_nodeAtPosReqType = NodeAtPosReqType::None;
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

void Edit3DView::setSeeker(SeekerSlider *slider)
{
    m_seeker = slider;
}

Edit3DAction *Edit3DView::createSelectBackgroundColorAction(QAction *syncBackgroundColorAction)
{
    QString description = QCoreApplication::translate("SelectBackgroundColorAction",
                                                      "Select Background Color");
    QString tooltip = QCoreApplication::translate("SelectBackgroundColorAction",
                                                  "Select a color for the background of the 3D view.");

    auto operation = [this, syncBackgroundColorAction](const SelectionContext &) {
        BackgroundColorSelection::showBackgroundColorSelectionWidget(
            edit3DWidget(),
            DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR,
            this,
            View3DActionType::SelectBackgroundColor,
            [this, syncBackgroundColorAction]() {
                if (syncBackgroundColorAction->isChecked()) {
                    Edit3DViewConfig::set(this, View3DActionType::SyncBackgroundColor, false);
                    syncBackgroundColorAction->setChecked(false);
                }
            });
    };

    return new Edit3DAction(Constants::EDIT3D_EDIT_SELECT_BACKGROUND_COLOR,
                            View3DActionType::SelectBackgroundColor,
                            description,
                            {},
                            false,
                            false,
                            {},
                            {},
                            this,
                            operation,
                            tooltip);
}

Edit3DAction *Edit3DView::createGridColorSelectionAction()
{
    QString description = QCoreApplication::translate("SelectGridColorAction", "Select Grid Color");
    QString tooltip = QCoreApplication::translate("SelectGridColorAction",
                                                  "Select a color for the grid lines of the 3D view.");

    auto operation = [this](const SelectionContext &) {
        BackgroundColorSelection::showBackgroundColorSelectionWidget(
            edit3DWidget(),
            DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR,
            this,
            View3DActionType::SelectGridColor);
    };

    return new Edit3DAction(Constants::EDIT3D_EDIT_SELECT_GRID_COLOR,
                            View3DActionType::SelectGridColor,
                            description,
                            {},
                            false,
                            false,
                            {},
                            {},
                            this,
                            operation,
                            tooltip);
}

Edit3DAction *Edit3DView::createResetColorAction(QAction *syncBackgroundColorAction)
{
    QString description = QCoreApplication::translate("ResetEdit3DColorsAction", "Reset Colors");
    QString tooltip = QCoreApplication::translate("ResetEdit3DColorsAction",
                                                  "Reset the background color and the color of the "
                                                  "grid lines of the 3D view to the default values.");

    auto operation = [this, syncBackgroundColorAction](const SelectionContext &) {
        QList<QColor> bgColors = {QRgb(0x222222), QRgb(0x999999)};
        Edit3DViewConfig::setColors(this, View3DActionType::SelectBackgroundColor, bgColors);
        Edit3DViewConfig::saveColors(DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR, bgColors);

        QColor gridColor{0xaaaaaa};
        Edit3DViewConfig::setColors(this, View3DActionType::SelectGridColor, {gridColor});
        Edit3DViewConfig::saveColors(DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR, {gridColor});

        if (syncBackgroundColorAction->isChecked()) {
            Edit3DViewConfig::set(this, View3DActionType::SyncBackgroundColor, false);
            syncBackgroundColorAction->setChecked(false);
        }
    };

    return new Edit3DAction(QmlDesigner::Constants::EDIT3D_EDIT_RESET_BACKGROUND_COLOR,
                            View3DActionType::ResetBackgroundColor,
                            description,
                            {},
                            false,
                            false,
                            {},
                            {},
                            this,
                            operation,
                            tooltip);
}

Edit3DAction *Edit3DView::createSyncBackgroundColorAction()
{
    QString description = QCoreApplication::translate("SyncEdit3DColorAction",
                                                      "Use Scene Environment Color");
    QString tooltip = QCoreApplication::translate("SyncEdit3DColorAction",
                                                  "Sets the 3D view to use the Scene Environment "
                                                  "color as background color.");

    return new Edit3DAction(QmlDesigner::Constants::EDIT3D_EDIT_SYNC_BACKGROUND_COLOR,
                            View3DActionType::SyncBackgroundColor,
                            description,
                            {},
                            true,
                            false,
                            {},
                            {},
                            this,
                            {},
                            tooltip);
}

void Edit3DView::createEdit3DActions()
{
    m_selectionModeAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_SELECTION_MODE,
        View3DActionType::SelectionModeToggle,
        QCoreApplication::translate("SelectionModeToggleAction", "Toggle Group/Single Selection Mode"),
        QKeySequence(Qt::Key_Q),
        true,
        false,
        Icons::EDIT3D_SELECTION_MODE_OFF.icon(),
        Icons::EDIT3D_SELECTION_MODE_ON.icon(),
        this);

    m_moveToolAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_MOVE_TOOL,
                                        View3DActionType::MoveTool,
                                        QCoreApplication::translate("MoveToolAction",
                                                                    "Activate Move Tool"),
                                        QKeySequence(Qt::Key_W),
                                        true,
                                        true,
                                        Icons::EDIT3D_MOVE_TOOL_OFF.icon(),
                                        Icons::EDIT3D_MOVE_TOOL_ON.icon(),
                                        this);

    m_rotateToolAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_ROTATE_TOOL,
                                          View3DActionType::RotateTool,
                                          QCoreApplication::translate("RotateToolAction",
                                                                      "Activate Rotate Tool"),
                                          QKeySequence(Qt::Key_E),
                                          true,
                                          false,
                                          Icons::EDIT3D_ROTATE_TOOL_OFF.icon(),
                                          Icons::EDIT3D_ROTATE_TOOL_ON.icon(),
                                          this);

    m_scaleToolAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_SCALE_TOOL,
                                         View3DActionType::ScaleTool,
                                         QCoreApplication::translate("ScaleToolAction",
                                                                     "Activate Scale Tool"),
                                         QKeySequence(Qt::Key_R),
                                         true,
                                         false,
                                         Icons::EDIT3D_SCALE_TOOL_OFF.icon(),
                                         Icons::EDIT3D_SCALE_TOOL_ON.icon(),
                                         this);

    m_fitAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_FIT_SELECTED,
                                   View3DActionType::FitToView,
                                   QCoreApplication::translate("FitToViewAction",
                                                               "Fit Selected Object to View"),
                                   QKeySequence(Qt::Key_F),
                                   false,
                                   false,
                                   Icons::EDIT3D_FIT_SELECTED_OFF.icon(),
                                   {},
                                   this);

    m_alignCamerasAction = new Edit3DCameraAction(
        QmlDesigner::Constants::EDIT3D_ALIGN_CAMERAS,
        View3DActionType::AlignCamerasToView,
        QCoreApplication::translate("AlignCamerasToViewAction", "Align Selected Cameras to View"),
        QKeySequence(),
        false,
        false,
        Icons::EDIT3D_ALIGN_CAMERA_ON.icon(),
        {},
        this);

    m_alignViewAction = new Edit3DCameraAction(
        QmlDesigner::Constants::EDIT3D_ALIGN_VIEW,
        View3DActionType::AlignViewToCamera,
        QCoreApplication::translate("AlignCamerasToViewAction", "Align View to Selected Camera"),
        QKeySequence(),
        false,
        false,
        Icons::EDIT3D_ALIGN_VIEW_ON.icon(),
        {},
        this);

    m_cameraModeAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_CAMERA,
        View3DActionType::CameraToggle,
        QCoreApplication::translate("CameraToggleAction",
                                    "Toggle Perspective/Orthographic Edit Camera"),
        QKeySequence(Qt::Key_T),
        true,
        false,
        Icons::EDIT3D_EDIT_CAMERA_OFF.icon(),
        Icons::EDIT3D_EDIT_CAMERA_ON.icon(),
        this);

    m_orientationModeAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_ORIENTATION,
        View3DActionType::OrientationToggle,
        QCoreApplication::translate("OrientationToggleAction", "Toggle Global/Local Orientation"),
        QKeySequence(Qt::Key_Y),
        true,
        false,
        Icons::EDIT3D_ORIENTATION_OFF.icon(),
        Icons::EDIT3D_ORIENTATION_ON.icon(),
        this);

    m_editLightAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_EDIT_LIGHT,
                                         View3DActionType::EditLightToggle,
                                         QCoreApplication::translate("EditLightToggleAction",
                                                                     "Toggle Edit Light On/Off"),
                                         QKeySequence(Qt::Key_U),
                                         true,
                                         false,
                                         Icons::EDIT3D_LIGHT_OFF.icon(),
                                         Icons::EDIT3D_LIGHT_ON.icon(),
                                         this);

    m_showGridAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_GRID,
        View3DActionType::ShowGrid,
        QCoreApplication::translate("ShowGridAction", "Show Grid"),
        QKeySequence(Qt::Key_G),
        true,
        true,
        {},
        {},
        this,
        nullptr,
        QCoreApplication::translate("ShowGridAction", "Toggle the visibility of the helper grid."));

    m_showSelectionBoxAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_SELECTION_BOX,
        View3DActionType::ShowSelectionBox,
        QCoreApplication::translate("ShowSelectionBoxAction", "Show Selection Boxes"),
        QKeySequence(Qt::Key_S),
        true,
        true,
        {},
        {},
        this,
        nullptr,
        QCoreApplication::translate("ShowSelectionBoxAction",
                                    "Toggle the visibility of selection boxes."));

    m_showIconGizmoAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_ICON_GIZMO,
        View3DActionType::ShowIconGizmo,
        QCoreApplication::translate("ShowIconGizmoAction", "Show Icon Gizmos"),
        QKeySequence(Qt::Key_I),
        true,
        true,
        {},
        {},
        this,
        nullptr,
        QCoreApplication::translate(
            "ShowIconGizmoAction",
            "Toggle the visibility of icon gizmos, such as light and camera icons."));

    m_showCameraFrustumAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM,
        View3DActionType::ShowCameraFrustum,
        QCoreApplication::translate("ShowCameraFrustumAction", "Always Show Camera Frustums"),
        QKeySequence(Qt::Key_C),
        true,
        false,
        {},
        {},
        this,
        nullptr,
        QCoreApplication::translate(
            "ShowCameraFrustumAction",
            "Toggle between always showing the camera frustum visualization and only showing it "
            "when the camera is selected."));

    m_showParticleEmitterAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_EDIT_SHOW_PARTICLE_EMITTER,
        View3DActionType::ShowParticleEmitter,
        QCoreApplication::translate("ShowParticleEmitterAction",
                                    "Always Show Particle Emitters And Attractors"),
        QKeySequence(Qt::Key_M),
        true,
        false,
        {},
        {},
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
        if (m_seeker)
            m_seeker->setEnabled(false);
        resetPuppet();
    };

    SelectionContextOperation particlesTrigger = [this](const SelectionContext &) {
        particlemode = !particlemode;
        m_particlesPlayAction->action()->setEnabled(particlemode);
        m_particlesRestartAction->action()->setEnabled(particlemode);
        if (m_seeker)
            m_seeker->setEnabled(false);
        QmlDesignerPlugin::settings().insert("particleMode", particlemode);
        resetPuppet();
    };

    SelectionContextOperation particlesPlayTrigger = [this](const SelectionContext &) {
        if (m_seeker)
            m_seeker->setEnabled(!m_particlesPlayAction->action()->isChecked());
    };

    m_particleViewModeAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_PARTICLE_MODE,
        View3DActionType::Edit3DParticleModeToggle,
        QCoreApplication::translate("ParticleViewModeAction", "Toggle particle animation On/Off"),
        QKeySequence(Qt::Key_V),
        true,
        false,
        Icons::EDIT3D_PARTICLE_OFF.icon(),
        Icons::EDIT3D_PARTICLE_ON.icon(),
        this,
        particlesTrigger);
    particlemode = false;
    m_particlesPlayAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_PARTICLES_PLAY,
                                             View3DActionType::ParticlesPlay,
                                             QCoreApplication::translate("ParticlesPlayAction",
                                                                         "Play Particles"),
                                             QKeySequence(Qt::Key_Comma),
                                             true,
                                             true,
                                             Icons::EDIT3D_PARTICLE_PLAY.icon(),
                                             Icons::EDIT3D_PARTICLE_PAUSE.icon(),
                                             this,
                                             particlesPlayTrigger);
    m_particlesRestartAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_PARTICLES_RESTART,
        View3DActionType::ParticlesRestart,
        QCoreApplication::translate("ParticlesRestartAction", "Restart Particles"),
        QKeySequence(Qt::Key_Slash),
        false,
        false,
        Icons::EDIT3D_PARTICLE_RESTART.icon(),
        Icons::EDIT3D_PARTICLE_RESTART.icon(),
        this);
    m_particlesPlayAction->action()->setEnabled(particlemode);
    m_particlesRestartAction->action()->setEnabled(particlemode);
    m_resetAction = new Edit3DAction(QmlDesigner::Constants::EDIT3D_RESET_VIEW,
                                     View3DActionType::Empty,
                                     QCoreApplication::translate("ResetView", "Reset View"),
                                     QKeySequence(Qt::Key_P),
                                     false,
                                     false,
                                     Utils::Icons::RESET_TOOLBAR.icon(),
                                     {},
                                     this,
                                     resetTrigger);

    SelectionContextOperation visibilityTogglesTrigger = [this](const SelectionContext &) {
        if (!edit3DWidget()->visibilityTogglesMenu())
            return;

        QPoint pos;
        const auto &actionWidgets = m_visibilityTogglesAction->action()->associatedWidgets();
        for (auto actionWidget : actionWidgets) {
            if (auto button = qobject_cast<QToolButton *>(actionWidget)) {
                pos = button->mapToGlobal(QPoint(0, 0));
                break;
            }
        }

        edit3DWidget()->showVisibilityTogglesMenu(!edit3DWidget()->visibilityTogglesMenu()->isVisible(),
                                                  pos);
    };

    m_visibilityTogglesAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_VISIBILITY_TOGGLES,
        View3DActionType::Empty,
        QCoreApplication::translate("VisibilityTogglesAction", "Visibility Toggles"),
        QKeySequence(),
        false,
        false,
        Utils::Icons::EYE_OPEN_TOOLBAR.icon(),
        {},
        this,
        visibilityTogglesTrigger);

    SelectionContextOperation backgroundColorActionsTrigger = [this](const SelectionContext &) {
        if (!edit3DWidget()->backgroundColorMenu())
            return;

        QPoint pos;
        const auto &actionWidgets = m_backgrondColorMenuAction->action()->associatedWidgets();
        for (auto actionWidget : actionWidgets) {
            if (auto button = qobject_cast<QToolButton *>(actionWidget)) {
                pos = button->mapToGlobal(QPoint(0, 0));
                break;
            }
        }

        edit3DWidget()->showBackgroundColorMenu(!edit3DWidget()->backgroundColorMenu()->isVisible(),
                                                pos);
    };

    m_backgrondColorMenuAction = new Edit3DAction(
        QmlDesigner::Constants::EDIT3D_BACKGROUND_COLOR_ACTIONS,
        View3DActionType::Empty,
        QCoreApplication::translate("BackgroundColorMenuActions", "Background Color Actions"),
        QKeySequence(),
        false,
        false,
        Icons::COLOR_PALETTE.icon(),
        {},
        this,
        backgroundColorActionsTrigger);

    m_leftActions << m_selectionModeAction;
    m_leftActions << nullptr; // Null indicates separator
    m_leftActions << nullptr; // Second null after separator indicates an exclusive group
    m_leftActions << m_moveToolAction;
    m_leftActions << m_rotateToolAction;
    m_leftActions << m_scaleToolAction;
    m_leftActions << nullptr;
    m_leftActions << m_fitAction;
    m_leftActions << nullptr;
    m_leftActions << m_cameraModeAction;
    m_leftActions << m_orientationModeAction;
    m_leftActions << m_editLightAction;
    m_leftActions << nullptr;
    m_leftActions << m_alignCamerasAction;
    m_leftActions << m_alignViewAction;
    m_leftActions << nullptr;
    m_leftActions << m_visibilityTogglesAction;
    m_leftActions << nullptr;
    m_leftActions << m_backgrondColorMenuAction;

    m_rightActions << m_particleViewModeAction;
    m_rightActions << m_particlesPlayAction;
    m_rightActions << m_particlesRestartAction;
    m_rightActions << nullptr;
    m_rightActions << m_resetAction;

    m_visibilityToggleActions << m_showGridAction;
    m_visibilityToggleActions << m_showSelectionBoxAction;
    m_visibilityToggleActions << m_showIconGizmoAction;
    m_visibilityToggleActions << m_showCameraFrustumAction;
    m_visibilityToggleActions << m_showParticleEmitterAction;

    Edit3DAction *syncBackgroundColorAction = createSyncBackgroundColorAction();
    m_backgroundColorActions << createSelectBackgroundColorAction(syncBackgroundColorAction->action());
    m_backgroundColorActions << createGridColorSelectionAction();
    m_backgroundColorActions << syncBackgroundColorAction;
    m_backgroundColorActions << createResetColorAction(syncBackgroundColorAction->action());
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

void Edit3DView::addQuick3DImport()
{
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (document && !document->inFileComponentModelActive() && model()) {
        const QList<Import> imports = model()->possibleImports();
        for (const auto &import : imports) {
            if (import.url() == "QtQuick3D") {
                if (!import.version().isEmpty() && import.majorVersion() >= 6) {
                    // Prefer empty version number in Qt6 and beyond
                    model()->changeImports({Import::createLibraryImport(
                                                import.url(), {}, import.alias(),
                                                import.importPaths())}, {});
                } else {
                    model()->changeImports({import}, {});
                }
                return;
            }
        }
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

void Edit3DView::dropTexture(const ModelNode &textureNode, const QPointF &pos)
{
    m_nodeAtPosReqType = NodeAtPosReqType::TextureDrop;
    m_droppedModelNode = textureNode;
    emitView3DAction(View3DActionType::GetNodeAtPos, pos);
}

} // namespace QmlDesigner
