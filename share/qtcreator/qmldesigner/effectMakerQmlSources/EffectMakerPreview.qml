// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Column {
    id: root

    width: parent.width

    required property Item mainRoot
    property var effectMakerModel: EffectMakerBackend.effectMakerModel
    property alias source: source
    // The delay in ms to wait until updating the effect
    readonly property int updateDelay: 200

    Rectangle { // toolbar
        width: parent.width
        height: StudioTheme.Values.toolbarHeight
        color: StudioTheme.Values.themeToolbarBackground

        RowLayout {
            anchors.fill: parent
            spacing: 5
            anchors.rightMargin: 5
            anchors.leftMargin: 5

            PreviewImagesComboBox {
                id: imagesComboBox

                mainRoot: root.mainRoot
            }

            Item {
                Layout.fillWidth: true
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
                enabled: sourceImage.scale < 2
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.zoomIn_medium
                tooltip: qsTr("Zoom In")

                onClicked: {
                    sourceImage.scale += .2
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

            Item {
                Layout.fillWidth: true
            }

            Column {
                Text {
                    text: "0.000s"
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }

                Text {
                    text: "0000000"
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: 10
                }
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.toStartFrame_medium
                tooltip: qsTr("Restart Animation")

                onClicked: {} // TODO
            }

            HelperWidgets.AbstractButton {
                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.topToolbar_runProject
                tooltip: qsTr("Play Animation")

                onClicked: {} // TODO
            }
        }
    }

    Rectangle { // preview image
        id: preview

        color: "#dddddd"
        width: parent.width
        height: 200
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

        Item {
            id: componentParent
            width: source.width
            height: source.height
                anchors.centerIn: parent
            scale: 1 //TODO should come from toolbar
            // Cache the layer. This way heavy shaders rendering doesn't
            // slow down code editing & rest of the UI.
            layer.enabled: true
            layer.smooth: true
        }

        // Create a dummy parent to host the effect qml object
        function createNewComponent() {
            var oldComponent = componentParent.children[0];
            if (oldComponent)
                oldComponent.destroy();

            try {
                const newObject = Qt.createQmlObject(
                    effectMakerModel.qmlComponentString,
                    componentParent,
                    ""
                );
                effectMakerModel.resetEffectError(0);
            } catch (error) {
                let errorString = "QML: ERROR: ";
                let errorLine = -1;
                if (error.qmlErrors.length > 0) {
                    // Show the first QML error
                    let e = error.qmlErrors[0];
                    errorString += e.lineNumber + ": " + e.message;
                    errorLine = e.lineNumber;
                }
                effectMakerModel.setEffectError(errorString, 0, errorLine);
            }
        }

        Connections {
            target: effectMakerModel
            function onShadersBaked() {
                console.log("Shaders Baked!")
                //updateTimer.restart(); // Disable for now
            }
        }

        Timer {
            id: updateTimer
            interval: updateDelay;
            onTriggered: {
                effectMakerModel.updateQmlComponent();
                createNewComponent();
            }
        }
    }
}
