/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "edit3dview.h"
#include "edit3dwidget.h"
#include "edit3dcanvas.h"
#include "edit3dactions.h"
#include "designmodewidget.h"

#include <nodeinstanceview.h>
#include <designeractionmanager.h>
#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>
#include <qmldesignericons.h>
#include <designmodecontext.h>
#include <utils/utilsicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QDebug>
#include <QToolButton>

namespace QmlDesigner {

Edit3DView::Edit3DView(QObject *parent)
    : AbstractView(parent)
{
}

Edit3DView::~Edit3DView()
{
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
    bool has3dImport = false;
    const QList<Import> imports = model()->imports();
    for (const auto &import : imports) {
        if (import.url() == "QtQuick3D") {
            has3dImport = true;
            break;
        }
    }

    edit3DWidget()->showCanvas(has3dImport);
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

void Edit3DView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(selectedNodeList)
    Q_UNUSED(lastSelectedNodeList)
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
        rootModelNode().setAuxiliaryData("active3dScene@Internal", newActiveScene);
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
}

void Edit3DView::modelAboutToBeDetached(Model *model)
{
    // Hide the canvas when model is detached (i.e. changing documents)
    m_canvasCache.insert(model, edit3DWidget()->canvas()->renderImage());
    edit3DWidget()->showCanvas(false);

    AbstractView::modelAboutToBeDetached(model);
}

void Edit3DView::importsChanged(const QList<Import> &addedImports,
                                const QList<Import> &removedImports)
{
    Q_UNUSED(addedImports)
    Q_UNUSED(removedImports)

    checkImports();
}

void Edit3DView::customNotification(const AbstractView *view, const QString &identifier,
                                    const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    Q_UNUSED(view)
    Q_UNUSED(nodeList)
    Q_UNUSED(data)

    if (identifier == "asset_import_update")
        resetPuppet();
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

void Edit3DView::createEdit3DActions()
{
    m_selectionModeAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_SELECTION_MODE, View3DActionCommand::SelectionModeToggle,
                QCoreApplication::translate("SelectionModeToggleAction", "Toggle Group/Single Selection Mode"),
                QKeySequence(Qt::Key_Q), true, false, Icons::EDIT3D_SELECTION_MODE_OFF.icon(),
                Icons::EDIT3D_SELECTION_MODE_ON.icon());

    m_moveToolAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_MOVE_TOOL, View3DActionCommand::MoveTool,
                QCoreApplication::translate("MoveToolAction", "Activate Move Tool"),
                QKeySequence(Qt::Key_W), true, true, Icons::EDIT3D_MOVE_TOOL_OFF.icon(),
                Icons::EDIT3D_MOVE_TOOL_ON.icon());

    m_rotateToolAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_ROTATE_TOOL, View3DActionCommand::RotateTool,
                QCoreApplication::translate("RotateToolAction", "Activate Rotate Tool"),
                QKeySequence(Qt::Key_E), true, false, Icons::EDIT3D_ROTATE_TOOL_OFF.icon(),
                Icons::EDIT3D_ROTATE_TOOL_ON.icon());

    m_scaleToolAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_SCALE_TOOL, View3DActionCommand::ScaleTool,
                QCoreApplication::translate("ScaleToolAction", "Activate Scale Tool"),
                QKeySequence(Qt::Key_R), true, false, Icons::EDIT3D_SCALE_TOOL_OFF.icon(),
                Icons::EDIT3D_SCALE_TOOL_ON.icon());

    m_fitAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_FIT_SELECTED, View3DActionCommand::FitToView,
                QCoreApplication::translate("FitToViewAction", "Fit Selected Object to View"),
                QKeySequence(Qt::Key_F), false, false, Icons::EDIT3D_FIT_SELECTED_OFF.icon(), {});

    m_alignCamerasAction = new Edit3DCameraAction(
                QmlDesigner::Constants::EDIT3D_ALIGN_CAMERAS, View3DActionCommand::AlignCamerasToView,
                QCoreApplication::translate("AlignCamerasToViewAction", "Align Selected Cameras to View"),
                QKeySequence(), false, false, Icons::EDIT3D_ALIGN_CAMERA_ON.icon(), {});

    m_alignViewAction = new Edit3DCameraAction(
                QmlDesigner::Constants::EDIT3D_ALIGN_VIEW, View3DActionCommand::AlignViewToCamera,
                QCoreApplication::translate("AlignCamerasToViewAction", "Align View to Selected Camera"),
                QKeySequence(), false, false, Icons::EDIT3D_ALIGN_VIEW_ON.icon(), {});

    m_cameraModeAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_CAMERA, View3DActionCommand::CameraToggle,
                QCoreApplication::translate("CameraToggleAction", "Toggle Perspective/Orthographic Edit Camera"),
                QKeySequence(Qt::Key_T), true, false, Icons::EDIT3D_EDIT_CAMERA_OFF.icon(),
                Icons::EDIT3D_EDIT_CAMERA_ON.icon());

    m_orientationModeAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_ORIENTATION, View3DActionCommand::OrientationToggle,
                QCoreApplication::translate("OrientationToggleAction", "Toggle Global/Local Orientation"),
                QKeySequence(Qt::Key_Y), true, false, Icons::EDIT3D_ORIENTATION_OFF.icon(),
                Icons::EDIT3D_ORIENTATION_ON.icon());

    m_editLightAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_LIGHT, View3DActionCommand::EditLightToggle,
                QCoreApplication::translate("EditLightToggleAction", "Toggle Edit Light On/Off"),
                QKeySequence(Qt::Key_U), true, false, Icons::EDIT3D_LIGHT_OFF.icon(),
                Icons::EDIT3D_LIGHT_ON.icon());

    m_showGridAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_SHOW_GRID, View3DActionCommand::ShowGrid,
                QCoreApplication::translate("ShowGridAction", "Show Grid"),
                QKeySequence(Qt::Key_G), true, true, {}, {}, nullptr,
                QCoreApplication::translate("ShowGridAction", "Toggle the visibility of the helper grid."));

    m_showSelectionBoxAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_SHOW_SELECTION_BOX, View3DActionCommand::ShowSelectionBox,
                QCoreApplication::translate("ShowSelectionBoxAction", "Show Selection Boxes"),
                QKeySequence(Qt::Key_S), true, true, {}, {}, nullptr,
                QCoreApplication::translate("ShowSelectionBoxAction", "Toggle the visibility of selection boxes."));

    m_showIconGizmoAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_SHOW_ICON_GIZMO, View3DActionCommand::ShowIconGizmo,
                QCoreApplication::translate("ShowIconGizmoAction", "Show Icon Gizmos"),
                QKeySequence(Qt::Key_I), true, true, {}, {}, nullptr,
                QCoreApplication::translate("ShowIconGizmoAction", "Toggle the visibility of icon gizmos, such as light and camera icons."));

    m_showCameraFrustumAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM, View3DActionCommand::ShowCameraFrustum,
                QCoreApplication::translate("ShowCameraFrustumAction", "Always Show Camera Frustums"),
                QKeySequence(Qt::Key_C), true, false, {}, {}, nullptr,
                QCoreApplication::translate("ShowCameraFrustumAction", "Toggle between always showing the camera frustum visualization and only showing it when the camera is selected."));

    m_showParticleEmitterAction = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_EDIT_SHOW_PARTICLE_EMITTER, View3DActionCommand::ShowParticleEmitter,
                QCoreApplication::translate("ShowParticleEmitterAction", "Always Show Particle Emitters And Attractors"),
                QKeySequence(Qt::Key_M), true, false, {}, {}, nullptr,
                QCoreApplication::translate("ShowParticleEmitterAction", "Toggle between always showing the particle emitter and attractor visualizations and only showing them when the emitter or attractor is selected."));

    SelectionContextOperation resetTrigger = [this](const SelectionContext &) {
        m_particlesPlayAction->action()->setEnabled(particlemode);
        m_particlesRestartAction->action()->setEnabled(particlemode);
        if (particlemode)
            m_particlesPlayAction->action()->setChecked(true);
        if (m_seeker)
            m_seeker->setEnabled(false);
        setCurrentStateNode(rootModelNode());
        resetPuppet();
    };

    SelectionContextOperation particlesTrigger = [this](const SelectionContext &) {
        particlemode = !particlemode;
        m_particlesPlayAction->action()->setEnabled(particlemode);
        m_particlesRestartAction->action()->setEnabled(particlemode);
        if (m_seeker)
            m_seeker->setEnabled(false);
        QmlDesigner::DesignerSettings::setValue("particleMode", particlemode);
        setCurrentStateNode(rootModelNode());
        resetPuppet();
    };

    SelectionContextOperation particlesPlayTrigger = [this](const SelectionContext &) {
        if (m_seeker)
            m_seeker->setEnabled(!m_particlesPlayAction->action()->isChecked());
    };

    m_particleViewModeAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_PARTICLE_MODE, View3DActionCommand::Edit3DParticleModeToggle,
                QCoreApplication::translate("ParticleViewModeAction", "Toggle particle animation On/Off"),
                QKeySequence(Qt::Key_V), true, false, Icons::EDIT3D_PARTICLE_OFF.icon(),
                Icons::EDIT3D_PARTICLE_ON.icon(), particlesTrigger);
    particlemode = false;
    m_particlesPlayAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_PARTICLES_PLAY, View3DActionCommand::ParticlesPlay,
                QCoreApplication::translate("ParticlesPlayAction", "Play Particles"),
                QKeySequence(Qt::Key_Comma), true, true, Icons::EDIT3D_PARTICLE_PLAY.icon(),
                Icons::EDIT3D_PARTICLE_PAUSE.icon(), particlesPlayTrigger);
    m_particlesRestartAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_PARTICLES_RESTART, View3DActionCommand::ParticlesRestart,
                QCoreApplication::translate("ParticlesRestartAction", "Restart Particles"),
                QKeySequence(Qt::Key_Slash), false, false, Icons::EDIT3D_PARTICLE_RESTART.icon(),
                Icons::EDIT3D_PARTICLE_RESTART.icon());
    m_particlesPlayAction->action()->setEnabled(particlemode);
    m_particlesRestartAction->action()->setEnabled(particlemode);
    m_resetAction
            = new Edit3DAction(
                QmlDesigner::Constants::EDIT3D_RESET_VIEW, View3DActionCommand::Empty,
                QCoreApplication::translate("ResetView", "Reset View"),
                QKeySequence(Qt::Key_P), false, false, Utils::Icons::RESET_TOOLBAR.icon(), {},
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
                QmlDesigner::Constants::EDIT3D_VISIBILITY_TOGGLES, View3DActionCommand::Empty,
                QCoreApplication::translate("VisibilityTogglesAction", "Visibility Toggles"),
                QKeySequence(), false, false, Utils::Icons::EYE_OPEN_TOOLBAR.icon(),
                {}, visibilityTogglesTrigger);

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

}

