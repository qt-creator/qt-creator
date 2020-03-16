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
#include <qmldesignerplugin.h>
#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <viewmanager.h>
#include <qmldesignericons.h>
#include <utils/utilsicons.h>
#include <coreplugin/icore.h>
#include <designmodecontext.h>

#include <QDebug>

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

WidgetInfo Edit3DView::widgetInfo()
{
    if (!m_edit3DWidget)
        createEdit3DWidget();

    return createWidgetInfo(m_edit3DWidget.data(), nullptr, "Editor3D", WidgetInfo::CentralPane, 0, tr("3D Editor"), DesignerWidgetFlags::IgnoreErrors);
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
}

void Edit3DView::updateActiveScene3D(const QVariantMap &sceneState)
{
    const QString sceneKey = QStringLiteral("sceneInstanceId");
    const QString selectKey = QStringLiteral("groupSelect");
    const QString transformKey = QStringLiteral("groupTransform");
    const QString perspectiveKey = QStringLiteral("usePerspective");
    const QString orientationKey = QStringLiteral("globalOrientation");
    const QString editLightKey = QStringLiteral("showEditLight");

    if (sceneState.contains(sceneKey)) {
        qint32 newActiveScene = sceneState[sceneKey].value<qint32>();
        edit3DWidget()->canvas()->updateActiveScene(newActiveScene);
        rootModelNode().setAuxiliaryData("3d-active-scene", newActiveScene);
    }

    if (sceneState.contains(selectKey))
        m_selectionModeAction->action()->setChecked(sceneState[selectKey].toInt() == 0);
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
}

void Edit3DView::modelAboutToBeDetached(Model *model)
{
    Q_UNUSED(model)

    // Clear the image when model is detached (i.e. changing documents)
    QImage emptyImage;
    edit3DWidget()->canvas()->updateRenderImage(emptyImage);
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

void Edit3DView::createEdit3DActions()
{
    m_selectionModeAction
            = new Edit3DAction(
                "Edit3DSelectionModeToggle", View3DActionCommand::SelectionModeToggle,
                QCoreApplication::translate("SelectionModeToggleAction", "Toggle Group/Single Selection Mode"),
                QKeySequence(Qt::Key_Q), true, false, Icons::EDIT3D_SELECTION_MODE_OFF.icon(),
                Icons::EDIT3D_SELECTION_MODE_ON.icon());

    m_moveToolAction
            = new Edit3DAction(
                "Edit3DMoveTool", View3DActionCommand::MoveTool,
                QCoreApplication::translate("MoveToolAction", "Activate Move Tool"),
                QKeySequence(Qt::Key_W), true, true, Icons::EDIT3D_MOVE_TOOL_OFF.icon(),
                Icons::EDIT3D_MOVE_TOOL_ON.icon());

    m_rotateToolAction
            = new Edit3DAction(
                "Edit3DRotateTool", View3DActionCommand::RotateTool,
                QCoreApplication::translate("RotateToolAction", "Activate Rotate Tool"),
                QKeySequence(Qt::Key_E), true, false, Icons::EDIT3D_ROTATE_TOOL_OFF.icon(),
                Icons::EDIT3D_ROTATE_TOOL_ON.icon());

    m_scaleToolAction
            = new Edit3DAction(
                "Edit3DScaleTool", View3DActionCommand::ScaleTool,
                QCoreApplication::translate("ScaleToolAction", "Activate Scale Tool"),
                QKeySequence(Qt::Key_R), true, false, Icons::EDIT3D_SCALE_TOOL_OFF.icon(),
                Icons::EDIT3D_SCALE_TOOL_ON.icon());

    m_fitAction = new Edit3DAction(
                "Edit3DFitToView", View3DActionCommand::FitToView,
                QCoreApplication::translate("FitToViewAction", "Fit Selected Object to View"),
                QKeySequence(Qt::Key_F), false, false, Icons::EDIT3D_FIT_SELECTED_OFF.icon(), {});

    m_cameraModeAction
            = new Edit3DAction(
                "Edit3DCameraToggle", View3DActionCommand::CameraToggle,
                QCoreApplication::translate("CameraToggleAction", "Toggle Perspective/Orthographic Edit Camera"),
                QKeySequence(Qt::Key_T), true, false, Icons::EDIT3D_EDIT_CAMERA_OFF.icon(),
                Icons::EDIT3D_EDIT_CAMERA_ON.icon());

    m_orientationModeAction
            = new Edit3DAction(
                "Edit3DOrientationToggle", View3DActionCommand::OrientationToggle,
                QCoreApplication::translate("OrientationToggleAction", "Toggle Global/Local Orientation"),
                QKeySequence(Qt::Key_Y), true, false, Icons::EDIT3D_ORIENTATION_OFF.icon(),
                Icons::EDIT3D_ORIENTATION_ON.icon());

    m_editLightAction
            = new Edit3DAction(
                "Edit3DEditLightToggle", View3DActionCommand::EditLightToggle,
                QCoreApplication::translate("EditLightToggleAction", "Toggle Edit Light On/Off"),
                QKeySequence(Qt::Key_U), true, false, Icons::EDIT3D_LIGHT_OFF.icon(),
                Icons::EDIT3D_LIGHT_ON.icon());

    SelectionContextOperation resetTrigger = [this](const SelectionContext &) {
        setCurrentStateNode(rootModelNode());
        resetPuppet();
    };
    m_resetAction
            = new Edit3DAction(
                "Edit3DResetView", View3DActionCommand::Empty,
                QCoreApplication::translate("ResetView", "Reset View"),
                QKeySequence(Qt::Key_P), false, false, Utils::Icons::RESET_TOOLBAR.icon(), {},
                resetTrigger);

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

    m_rightActions << m_resetAction;

    // TODO: Registering actions to action manager causes conflicting shortcuts in form editor.
    //       Registration commented out until UX defines non-conflicting shortcuts.
    //       Also, actions creation needs to be somehow triggered before action manager registers
    //       actions to creator.
//    DesignerActionManager &actionManager = QmlDesignerPlugin::instance()->designerActionManager();
//    for (auto action : qAsConst(m_leftActions)) {
//        if (action)
//            actionManager.addDesignerAction(action);
//    }
//    for (auto action : qAsConst(m_rightActions)) {
//        if (action)
//            actionManager.addDesignerAction(action);
//    }
}

QVector<Edit3DAction *> Edit3DView::leftActions() const
{
    return m_leftActions;
}

QVector<Edit3DAction *> Edit3DView::rightActions() const
{
    return m_rightActions;
}

}

