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

    readonly property int previewMargin: 5

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
                effectComposerModel.qmlComponentString(),
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
                enabled: sourceImage.scale < 3
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomIn_medium
                tooltip: qsTr("Zoom In")

                onClicked: {
                    sourceImage.enableAnim(true)
                    sourceImage.scale += .2
                    sourceImage.enableAnim(false)
                    zoomIndicator.show()
                }
            }

            HelperWidgets.AbstractButton {
                enabled: sourceImage.scale > .4
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomOut_medium
                tooltip: qsTr("Zoom out")

                onClicked: {
                    sourceImage.enableAnim(true)
                    sourceImage.scale -= .2
                    sourceImage.enableAnim(false)
                    zoomIndicator.show()
                }
            }

            HelperWidgets.AbstractButton {
                enabled: sourceImage.scale !== 1 || sourceImage.x !== root.previewMargin
                                                 || sourceImage.y !== root.previewMargin
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.fitAll_medium
                tooltip: qsTr("Reset View")

                onClicked: {
                    sourceImage.resetTransforms()
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

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton

            property real pressX: 0
            property real pressY: 0
            property bool panning: false

            onPressed:  {
                pressX = mouseX - sourceImage.x
                pressY = mouseY - sourceImage.y
                panning = true
            }

            onReleased: {
                panning = false
            }

            onWheel: (wheel) => {
                let prevScale = sourceImage.scale

                if (wheel.angleDelta.y > 0) {
                    if (sourceImage.scale < 3)
                        sourceImage.scale += .2
                } else {
                    if (sourceImage.scale > .4)
                        sourceImage.scale -= .2
                }

                let dScale = sourceImage.scale - prevScale

                sourceImage.x += (sourceImage.x + sourceImage.width * .5 - wheel.x) * dScale;
                sourceImage.y += (sourceImage.y + sourceImage.height * .5 - wheel.y) * dScale;

                sourceImage.checkBounds()
                zoomIndicator.show()
            }

            Timer { // pan timer
                running: parent.panning
                interval: 16
                repeat: true

                onTriggered: {
                    sourceImage.x = mouseArea.mouseX - mouseArea.pressX
                    sourceImage.y = mouseArea.mouseY - mouseArea.pressY
                    sourceImage.checkBounds()
                }
            }
        }

        Item { // Source item as a canvas (render target) for effect
            id: source
            anchors.fill: parent
            layer.enabled: true
            layer.mipmap: true
            layer.smooth: true

            Image {
                id: sourceImage

                function checkBounds() {
                    let edgeMargin = 10
                    // correction factor to account for an observation that edgeMargin decreases
                    // with increased zoom
                    let corrFactor = 10 * sourceImage.scale
                    let imgW2 = sourceImage.paintedWidth * sourceImage.scale * .5
                    let imgH2 = sourceImage.paintedHeight * sourceImage.scale * .5
                    let srcW2 = source.width * .5
                    let srcH2 = source.height * .5

                    if (sourceImage.x < -srcW2 - imgW2 + edgeMargin + corrFactor)
                        sourceImage.x = -srcW2 - imgW2 + edgeMargin + corrFactor
                    else if (x > srcW2 + imgW2 - edgeMargin - corrFactor)
                        sourceImage.x = srcW2 + imgW2 - edgeMargin - corrFactor

                    if (sourceImage.y < -srcH2 - imgH2 + edgeMargin + corrFactor)
                        sourceImage.y = -srcH2 - imgH2 + edgeMargin + corrFactor
                    else if (y > srcH2 + imgH2 - edgeMargin - corrFactor)
                        sourceImage.y = srcH2 + imgH2 - edgeMargin - corrFactor
                }

                function resetTransforms() {
                    sourceImage.enableAnim(true)
                    sourceImage.scale = 1
                    sourceImage.x = root.previewMargin
                    sourceImage.y = root.previewMargin
                    sourceImage.enableAnim(false)
                }

                function enableAnim(flag) {
                    xBehavior.enabled = flag
                    yBehavior.enabled = flag
                    scaleBehavior.enabled = flag
                }

                onSourceChanged: sourceImage.resetTransforms()

                fillMode: Image.PreserveAspectFit

                x: root.previewMargin
                y: root.previewMargin
                width: parent.width - root.previewMargin * 2
                height: parent.height - root.previewMargin * 2
                source: imagesComboBox.selectedImage
                smooth: true

                Behavior on x {
                    id: xBehavior

                    enabled: false
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutQuad
                    }
                }

                Behavior on y {
                    id: yBehavior

                    enabled: false
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutQuad
                    }
                }

                Behavior on scale {
                    id: scaleBehavior

                    enabled: false
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

        Rectangle {
            id: zoomIndicator

            width: 40
            height: 20
            color: StudioTheme.Values.themeDialogBackground
            visible: false

            function show() {
                zoomIndicator.visible = true
                zoomIndicatorTimer.start()
            }

            Text {
                text: Math.round(sourceImage.scale * 100) + "%"
                color: StudioTheme.Values.themeTextColor
                anchors.centerIn: parent
            }

            Timer {
                id: zoomIndicatorTimer
                interval: 1000
                onTriggered: zoomIndicator.visible = false
            }
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
