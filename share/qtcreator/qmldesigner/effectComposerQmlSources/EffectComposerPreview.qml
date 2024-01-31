// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Column {
    id: root

    property real animatedTime: previewFrameTimer.elapsedTime
    property int animatedFrame: previewFrameTimer.currentFrame
    property bool timeRunning: previewAnimationRunning

    required property Item mainRoot
    property var effectComposerModel: EffectComposerBackend.effectComposerModel
    property alias source: source
    // The delay in ms to wait until updating the effect
    readonly property int updateDelay: 100

    // Create a dummy parent to host the effect qml object
    function createNewComponent() {
        // If we have a working effect, do not show preview image as it shows through
        // transparent parts of the final image
        source.visible = false;

        var oldComponent = componentParent.children[0];
        if (oldComponent)
            oldComponent.destroy();
        try {
            const newObject = Qt.createQmlObject(
                effectComposerModel.qmlComponentString,
                componentParent,
                ""
            );
            effectComposerModel.resetEffectError(0);
        } catch (error) {
            let errorString = "QML: ERROR: ";
            let errorLine = -1;
            if (error.qmlErrors.length > 0) {
                // Show the first QML error
                let e = error.qmlErrors[0];
                errorString += e.lineNumber + ": " + e.message;
                errorLine = e.lineNumber;
            }
            effectComposerModel.setEffectError(errorString, 0, errorLine);
            source.visible = true;
        }
    }

    Rectangle { // toolbar
        width: parent.width
        height: StudioTheme.Values.toolbarHeight
        color: StudioTheme.Values.themeToolbarBackground

        Row {
            spacing: 5
            anchors.leftMargin: 5
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            PreviewImagesComboBox {
                id: imagesComboBox

                mainRoot: root.mainRoot
            }

            StudioControls.ColorEditor {
                id: colorEditor

                actionIndicatorVisible: false
                showHexTextField: false
                color: "#dddddd"
            }
        }

        Row {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter

            HelperWidgets.AbstractButton {
                enabled: sourceImage.scale < 2
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomIn_medium
                tooltip: qsTr("Zoom In")

                onClicked: {
                    sourceImage.scale += .2
                }
            }

            HelperWidgets.AbstractButton {
                enabled: sourceImage.scale > .4
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomOut_medium
                tooltip: qsTr("Zoom out")

                onClicked: {
                    sourceImage.scale -= .2
                }
            }

            HelperWidgets.AbstractButton {
                enabled: sourceImage.scale !== 1
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.fitAll_medium
                tooltip: qsTr("Zoom Fit")

                onClicked: {
                    sourceImage.scale = 1
                }
            }
        }

        Row {
            spacing: 5
            anchors.rightMargin: 5
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            Column {
                Text {
                    text: animatedTime >= 100
                          ? animatedTime.toFixed(1) + " s" : animatedTime.toFixed(3) + " s"
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }

                Text {
                    text: (animatedFrame).toString().padStart(6, '0')
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.toStartFrame_medium
                tooltip: qsTr("Restart Animation")

                onClicked: {
                    previewFrameTimer.reset()
                }
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: previewAnimationRunning ? StudioTheme.Constants.pause_medium
                                                    : StudioTheme.Constants.playOutline_medium
                tooltip: qsTr("Play Animation")

                onClicked: {
                    previewAnimationRunning = !previewAnimationRunning
                }
            }
        }
    }

    Rectangle { // preview image
        id: preview

        color: colorEditor.color
        width: parent.width
        height: root.height - y
        clip: true

        Item { // Source item as a canvas (render target) for effect
            id: source
            anchors.fill: parent
            layer.enabled: true
            layer.mipmap: true
            layer.smooth: true

            Image {
                id: sourceImage
                anchors.margins: 5
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: imagesComboBox.selectedImage
                smooth: true

                Behavior on scale {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutQuad
                    }
                }
            }
        }

        BlurHelper {
            id: blurHelper
            source: source
            property int blurMax: g_propertyData.blur_helper_max_level ? g_propertyData.blur_helper_max_level : 64
            property real blurMultiplier: g_propertyData.blurMultiplier ? g_propertyData.blurMultiplier : 0
        }

        Item {
            id: componentParent
            width: source.width
            height: source.height
            anchors.centerIn: parent
            // Cache the layer. This way heavy shaders rendering doesn't
            // slow down code editing & rest of the UI.
            layer.enabled: true
            layer.smooth: true
        }

        Connections {
            target: effectComposerModel
            function onShadersBaked() {
                console.log("Shaders Baked!")
                updateTimer.restart()
            }
        }

        Timer {
            id: updateTimer
            interval: updateDelay
            onTriggered: {
                effectComposerModel.updateQmlComponent()
                createNewComponent()
            }
        }
    }
}
