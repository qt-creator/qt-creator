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

    property real previewScale: 1

    // Create a dummy parent to host the effect qml object
    function createNewComponent() {
        // If we have a working effect, do not show preview image as it shows through
        // transparent parts of the final image
        placeHolder.visible = false;

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
            placeHolder.visible = true;
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
                enabled: root.previewScale < 3
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomIn_medium
                tooltip: qsTr("Zoom In")

                onClicked: {
                    imageScaler.enableAnim(true)
                    root.previewScale += .2
                    imageScaler.enableAnim(false)
                    zoomIndicator.show()
                }
            }

            HelperWidgets.AbstractButton {
                enabled: root.previewScale > .4
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomOut_medium
                tooltip: qsTr("Zoom out")

                onClicked: {
                    imageScaler.enableAnim(true)
                    root.previewScale -= .2
                    imageScaler.enableAnim(false)
                    zoomIndicator.show()
                }
            }

            HelperWidgets.AbstractButton {
                enabled: root.previewScale !== 1 || imageScaler.x !== root.previewMargin
                                                 || imageScaler.y !== root.previewMargin
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.fitAll_medium
                tooltip: qsTr("Reset View")

                onClicked: {
                    imageScaler.resetTransforms()
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
                pressX = mouseX - imageScaler.x
                pressY = mouseY - imageScaler.y
                panning = true
            }

            onReleased: {
                panning = false
            }

            onWheel: (wheel) => {
                let oldPoint = imageScaler.mapFromItem(mouseArea, Qt.point(wheel.x, wheel.y))

                if (wheel.angleDelta.y > 0) {
                    if (root.previewScale < 3)
                        root.previewScale += .2
                } else {
                    if (root.previewScale > .4)
                        root.previewScale -= .2
                }

                let newPoint = imageScaler.mapFromItem(mouseArea, Qt.point(wheel.x, wheel.y))
                imageScaler.x -= (oldPoint.x - newPoint.x) * imageScaler.scale
                imageScaler.y -= (oldPoint.y - newPoint.y) * imageScaler.scale

                imageScaler.checkBounds()
                zoomIndicator.show()
            }

            Timer { // pan timer
                running: parent.panning
                interval: 16
                repeat: true

                onTriggered: {
                    imageScaler.x = mouseArea.mouseX - mouseArea.pressX
                    imageScaler.y = mouseArea.mouseY - mouseArea.pressY
                    imageScaler.checkBounds()
                }
            }
        }

        Image {
            id: placeHolder
            anchors.fill: parent
            anchors.margins: root.previewMargin
            fillMode: Image.PreserveAspectFit
            source: imagesComboBox.selectedImage
            smooth: true
        }

        Item { // Source item as a canvas (render target) for effect
            id: source
            width: sourceImage.sourceSize.width
            height: sourceImage.sourceSize.height
            layer.enabled: true
            layer.mipmap: true
            layer.smooth: true
            visible: false

            Image {
                id: sourceImage

                onSourceChanged: imageScaler.resetTransforms()

                fillMode: Image.Pad

                source: imagesComboBox.selectedImage
                smooth: true
            }
        }

        BlurHelper {
            id: blurHelper
            source: source
            property int blurMax: g_propertyData.blur_helper_max_level ? g_propertyData.blur_helper_max_level : 64
            property real blurMultiplier: g_propertyData.blurMultiplier ? g_propertyData.blurMultiplier : 0
        }

        Item {
            id: imageScaler
            x: root.previewMargin
            y: root.previewMargin
            width: parent.width - root.previewMargin * 2
            height: parent.height - root.previewMargin * 2

            scale: root.previewScale * (width > height ? height / sourceImage.sourceSize.height
                                                       : width / sourceImage.sourceSize.width)

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

            function checkBounds() {
                let edgeMargin = 10
                // correction factor to account for an observation that edgeMargin decreases
                // with increased zoom
                let corrFactor = 10 * imageScaler.scale
                let imgW2 = sourceImage.paintedWidth * imageScaler.scale * .5
                let imgH2 = sourceImage.paintedHeight * imageScaler.scale * .5
                let srcW2 = width * .5
                let srcH2 = height * .5

                if (imageScaler.x < -srcW2 - imgW2 + edgeMargin + corrFactor)
                    imageScaler.x = -srcW2 - imgW2 + edgeMargin + corrFactor
                else if (x > srcW2 + imgW2 - edgeMargin - corrFactor)
                    imageScaler.x = srcW2 + imgW2 - edgeMargin - corrFactor

                if (imageScaler.y < -srcH2 - imgH2 + edgeMargin + corrFactor)
                    imageScaler.y = -srcH2 - imgH2 + edgeMargin + corrFactor
                else if (y > srcH2 + imgH2 - edgeMargin - corrFactor)
                    imageScaler.y = srcH2 + imgH2 - edgeMargin - corrFactor
            }

            function resetTransforms() {
                imageScaler.enableAnim(true)
                root.previewScale = 1
                imageScaler.x = root.previewMargin
                imageScaler.y = root.previewMargin
                imageScaler.enableAnim(false)
            }

            function enableAnim(flag) {
                xBehavior.enabled = flag
                yBehavior.enabled = flag
                scaleBehavior.enabled = flag
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
                text: Math.round(root.previewScale * 100) + "%"
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
