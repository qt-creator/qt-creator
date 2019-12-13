/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick3D 1.0
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import MouseArea3D 1.0

Window {
    id: viewWindow
    width: 1024
    height: 768
    visible: true
    title: "3D Edit View"
    // need all those flags otherwise the title bar disappears after setting WindowStaysOnTopHint flag later
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    property alias scene: editView.importScene
    property alias showEditLight: btnEditViewLight.toggled
    property alias usePerspective: btnPerspective.toggled

    property Node selectedNode: null // This is non-null only in single selection case
    property var selectedNodes: [] // All selected nodes

    property var lightGizmos: []
    property var cameraGizmos: []
    property var selectionBoxes: []
    property rect viewPortRect: Qt.rect(0, 0, 1000, 1000)

    signal selectionChanged(var selectedNodes)
    signal commitObjectProperty(var object, var propName)
    signal changeObjectProperty(var object, var propName)

    function ensureSelectionBoxes(count) {
        var needMore = count - selectionBoxes.length
        if (needMore > 0) {
            var component = Qt.createComponent("SelectionBox.qml");
            if (component.status === Component.Ready) {
                for (var i = 0; i < needMore; ++i) {
                    var geometryName = _generalHelper.generateUniqueName("SelectionBoxGeometry");
                    var box = component.createObject(mainSceneHelpers, {"view3D": editView,
                                                     "geometryName": geometryName});
                    selectionBoxes[selectionBoxes.length] = box;
                }
            }
        }
    }

    function selectObjects(objects) {
        // Create selection boxes as necessary. One more box than is actually needed is created, so
        // that we always have a previously created box to use for new selection.
        // This fixes an occasional visual glitch when creating a new box.
        ensureSelectionBoxes(objects.length + 1)

        var i;
        for (i = 0; i < objects.length; ++i)
            selectionBoxes[i].targetNode = objects[i];
        for (i = objects.length; i < selectionBoxes.length; ++i)
            selectionBoxes[i].targetNode = null;

        selectedNodes = objects;
        if (objects.length === 0 || objects.length > 1)
            selectedNode = null;
        else
            selectedNode = objects[0];
    }

    function handleObjectClicked(object, multi) {
        var theObject = object;
        if (btnSelectGroup.selected) {
            while (theObject && theObject.parent !== scene)
                theObject = theObject.parent;
        }
        // Object selection logic:
        // Regular click: Clear any multiselection, single-selects the clicked object
        // Ctrl-click: No objects selected: Act as single select
        //             One or more objects selected: Multiselect
        // Null object always clears entire selection
        var newSelection = [];
        if (object !== null) {
            if (multi && selectedNodes.length > 0) {
                var deselect = false;
                for (var i = 0; i < selectedNodes.length; ++i) {
                    // Multiselecting already selected object clears that object from selection
                    if (selectedNodes[i] !== object)
                        newSelection[newSelection.length] = selectedNodes[i];
                    else
                        deselect = true;
                }
                if (!deselect)
                    newSelection[newSelection.length] = object;
            } else {
                newSelection[0] = theObject;
            }
        }
        selectObjects(newSelection);
        selectionChanged(newSelection);
    }

    function addLightGizmo(obj)
    {
        // Insert into first available gizmo
        for (var i = 0; i < lightGizmos.length; ++i) {
            if (!lightGizmos[i].targetNode) {
                lightGizmos[i].targetNode = obj;
                return;
            }
        }

        // No free gizmos available, create a new one
        var component = Qt.createComponent("LightGizmo.qml");
        if (component.status === Component.Ready) {
            var gizmo = component.createObject(overlayScene,
                                               {"view3D": overlayView, "targetNode": obj,
                                                "selectedNodes": selectedNodes});
            lightGizmos[lightGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
        }
    }

    function addCameraGizmo(obj)
    {
        // Insert into first available gizmo
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (!cameraGizmos[i].targetNode) {
                cameraGizmos[i].targetNode = obj;
                return;
            }
        }
        // No free gizmos available, create a new one
        var component = Qt.createComponent("CameraGizmo.qml");
        if (component.status === Component.Ready) {
            var geometryName = _generalHelper.generateUniqueName("CameraGeometry");
            var gizmo = component.createObject(
                        overlayScene,
                        {"view3D": overlayView, "targetNode": obj, "geometryName": geometryName,
                         "viewPortRect": viewPortRect, "selectedNodes": selectedNodes});
            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.viewPortRect = Qt.binding(function() {return viewPortRect;});
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
        }
    }

    // Work-around the fact that the projection matrix for the camera is not calculated until
    // the first frame is rendered, so any initial calls to mapFrom3DScene() will fail.
    Component.onCompleted: {
        selectObjects([]);
        _generalHelper.requestOverlayUpdate();
    }

    onWidthChanged: _generalHelper.requestOverlayUpdate();
    onHeightChanged: _generalHelper.requestOverlayUpdate();

    Node {
        id: overlayScene

        PerspectiveCamera {
            id: overlayPerspectiveCamera
            clipFar: editPerspectiveCamera.clipFar
            clipNear: editPerspectiveCamera.clipNear
            position: editPerspectiveCamera.position
            rotation: editPerspectiveCamera.rotation
        }

        OrthographicCamera {
            id: overlayOrthoCamera
            clipFar: editOrthoCamera.clipFar
            clipNear: editOrthoCamera.clipNear
            position: editOrthoCamera.position
            rotation: editOrthoCamera.rotation
            scale: editOrthoCamera.scale
        }

        MouseArea3D {
            id: gizmoDragHelper
            view3D: overlayView
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            globalOrientation: btnLocalGlobal.toggled
            visible: selectedNode && btnMove.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPositionCommit: viewWindow.commitObjectProperty(selectedNode, "position")
            onPositionMove: viewWindow.changeObjectProperty(selectedNode, "position")
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            globalOrientation: false
            visible: selectedNode && btnScale.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onScaleCommit: viewWindow.commitObjectProperty(selectedNode, "scale")
            onScaleChange: viewWindow.changeObjectProperty(selectedNode, "scale")
        }

        RotateGizmo {
            id: rotateGizmo
            scale: autoScale.getScale(Qt.vector3d(7, 7, 7))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            globalOrientation: btnLocalGlobal.toggled
            visible: selectedNode && btnRotate.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onRotateCommit: viewWindow.commitObjectProperty(selectedNode, "rotation")
            onRotateChange: viewWindow.changeObjectProperty(selectedNode, "rotation")
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: moveGizmo.scenePosition
            orientation: moveGizmo.orientation
        }
    }

    Rectangle {
        anchors.fill: parent
        focus: true

        gradient: Gradient {
            GradientStop { position: 1.0; color: "#222222" }
            GradientStop { position: 0.0; color: "#999999" }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                var pickResult = editView.pick(mouse.x, mouse.y);
                handleObjectClicked(_generalHelper.resolvePick(pickResult.objectHit),
                                    mouse.modifiers & Qt.ControlModifier);
                if (!pickResult.objectHit)
                    mouse.accepted = false;
            }
        }

        DropArea {
            anchors.fill: parent
        }

        View3D {
            id: editView
            anchors.fill: parent
            camera: usePerspective ? editPerspectiveCamera : editOrthoCamera

            Node {
                id: mainSceneHelpers

                HelperGrid {
                    id: helperGrid
                    lines: 50
                    step: 50
                }

                PointLight {
                    id: editLight
                    visible: showEditLight
                    position: usePerspective ? editPerspectiveCamera.position
                                             : editOrthoCamera.position
                    quadraticFade: 0
                    linearFade: 0
                }

                // Initial camera position and rotation should be such that they look at origin.
                // Otherwise EditCameraController._lookAtPoint needs to be initialized to correct
                // point.
                PerspectiveCamera {
                    id: editPerspectiveCamera
                    z: -600
                    y: 600
                    rotation.x: 45
                    clipFar: 100000
                    clipNear: 1
                }

                OrthographicCamera {
                    id: editOrthoCamera
                    z: -600
                    y: 600
                    rotation.x: 45
                    clipFar: 100000
                    clipNear: -10000
                }
            }
        }

        View3D {
            id: overlayView
            anchors.fill: parent
            camera: usePerspective ? overlayPerspectiveCamera : overlayOrthoCamera
            importScene: overlayScene
        }

        Overlay2D {
            id: gizmoLabel
            targetNode: moveGizmo.visible ? moveGizmo : scaleGizmo
            targetView: overlayView
            visible: targetNode.dragging

            Rectangle {
                color: "white"
                x: -width / 2
                y: -height - 8
                width: gizmoLabelText.width + 4
                height: gizmoLabelText.height + 4
                border.width: 1
                Text {
                    id: gizmoLabelText
                    text: {
                        var l = Qt.locale();
                        var targetProperty;
                        if (viewWindow.selectedNode) {
                            if (gizmoLabel.targetNode === moveGizmo)
                                targetProperty = viewWindow.selectedNode.position;
                            else
                                targetProperty = viewWindow.selectedNode.scale;
                            return qsTr("x:") + Number(targetProperty.x).toLocaleString(l, 'f', 1)
                                + qsTr(" y:") + Number(targetProperty.y).toLocaleString(l, 'f', 1)
                                + qsTr(" z:") + Number(targetProperty.z).toLocaleString(l, 'f', 1);
                        } else {
                            return "";
                        }
                    }
                    anchors.centerIn: parent
                }
            }
        }

        EditCameraController {
            id: cameraControl
            camera: editView.camera
            anchors.fill: parent
            view3d: editView
        }
    }

    Rectangle { // toolbar
        id: toolbar
        color: "#9F000000"
        width: 35
        height: col.height

        Column {
            id: col
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5
            padding: 5

            property var groupSelect: [btnSelectGroup, btnSelectItem]
            property var groupTransform: [btnMove, btnRotate, btnScale]

            ToolBarButton {
                id: btnSelectItem
                selected: true
                tooltip: qsTr("Select Item")
                shortcut: "Q"
                currentShortcut: selected ? "" : shortcut
                tool: "item_selection"
                buttonsGroup: col.groupSelect
            }

            ToolBarButton {
                id: btnSelectGroup
                tooltip: qsTr("Select Group")
                shortcut: "Q"
                currentShortcut: btnSelectItem.currentShortcut === shortcut ? "" : shortcut
                tool: "group_selection"
                buttonsGroup: col.groupSelect
            }

            Rectangle { // separator
                width: 25
                height: 1
                color: "#f1f1f1"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            ToolBarButton {
                id: btnMove
                selected: true
                tooltip: qsTr("Move current selection")
                shortcut: "W"
                currentShortcut: shortcut
                tool: "move"
                buttonsGroup: col.groupTransform
            }

            ToolBarButton {
                id: btnRotate
                tooltip: qsTr("Rotate current selection")
                shortcut: "E"
                currentShortcut: shortcut
                tool: "rotate"
                buttonsGroup: col.groupTransform
            }

            ToolBarButton {
                id: btnScale
                tooltip: qsTr("Scale current selection")
                shortcut: "R"
                currentShortcut: shortcut
                tool: "scale"
                buttonsGroup: col.groupTransform
            }

            Rectangle { // separator
                width: 25
                height: 1
                color: "#f1f1f1"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            ToolBarButton {
                id: btnFit
                tooltip: qsTr("Fit camera to current selection")
                shortcut: "F"
                currentShortcut: shortcut
                tool: "fit"
                togglable: false

                onSelectedChanged: {
                    if (selected) {
                        var targetNode = viewWindow.selectedNodes.length > 0
                                ? selectionBoxes[0].model : null;
                        cameraControl.focusObject(targetNode, editView.camera.rotation, true);
                    }
                }
            }
        }
    }

    AxisHelper {
        anchors.right: parent.right
        anchors.top: parent.top
        width: 100
        height: width
        editCameraCtrl: cameraControl
        selectedNode : viewWindow.selectedNodes.length ? selectionBoxes[0].model : null
    }

    Rectangle { // top controls bar
        color: "#aa000000"
        width: 290
        height: btnPerspective.height + 10
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: 100

        Row {
            padding: 5
            anchors.fill: parent
            ToggleButton {
                id: btnPerspective
                width: 105
                tooltip: qsTr("Toggle Perspective / Orthographic Projection")
                states: [{iconId: "ortho", text: qsTr("Orthographic")}, {iconId: "persp",  text: qsTr("Perspective")}]
            }

            ToggleButton {
                id: btnLocalGlobal
                width: 65
                tooltip: qsTr("Toggle Global / Local Orientation")
                states: [{iconId: "local",  text: qsTr("Local")}, {iconId: "global", text: qsTr("Global")}]
            }

            ToggleButton {
                id: btnEditViewLight
                width: 110
                toggleBackground: true
                tooltip: qsTr("Toggle Edit Light")
                states: [{iconId: "edit_light_off",  text: qsTr("Edit Light Off")}, {iconId: "edit_light_on", text: qsTr("Edit Light On")}]
            }
        }

    }

    Text {
        id: helpText

        property string modKey: _generalHelper.isMacOS ? qsTr("Option") : qsTr("Alt")

        color: "white"
        text: qsTr("Camera controls: ") + modKey
              + qsTr(" + mouse press and drag. Left: Rotate, Middle: Pan, Right/Wheel: Zoom.")
        anchors.bottom: parent.bottom
    }
}
