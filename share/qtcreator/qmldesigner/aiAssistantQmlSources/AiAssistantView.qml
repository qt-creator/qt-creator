// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    property var rootView: AiAssistantBackend.rootView
    property string attachedImage

    color: StudioTheme.Values.themeBackgroundColorNormal

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: StudioTheme.Values.marginTopBottom

        Flow {
            spacing: 5
            Layout.fillWidth: true

            Repeater {
                model: ["Add a rectangle", "Add a button", "Add a Text", "Create a sample UI", "Remove all objects"]

                delegate: PromptButton {
                    required property string modelData

                    label: modelData
                    enabled: !root.rootView.isGenerating

                    onClicked: root.rootView.handleMessage(modelData)
                }
            }
        }

        PromptTextBox {
            rootView: root.rootView

            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Row {
            spacing: 5
            Layout.alignment: Qt.AlignRight

            Rectangle {
                width: 60
                height: 40
                color: StudioTheme.Values.themeToolTipBackground

                visible: root.attachedImage !== ""

                Image {
                    id: attachedImageImage

                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit

                    StudioControls.ToolTipArea {
                        text: root.attachedImage
                        anchors.fill: parent
                    }

                    HelperWidgets.AbstractButton {
                        id: removeImageButton
                        objectName: "RemoveImageButton"

                        width: 12
                        height: 12
                        iconSize: 10

                        anchors.right: parent.right
                        anchors.top: parent.top

                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.close_small
                        tooltip: qsTr("Remove the attached image.")

                        onClicked: {
                            root.attachedImage = ""
                            attachedImageImage.source = ""
                        }
                    }
                }
            }

            HelperWidgets.AbstractButton {
                id: attachImageButton
                objectName: "AttachImageButton"

                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.attach_medium
                tooltip: qsTr("Attach an image.\nThe attached image will be analyzed and integrated into the response by the AI.")

                onClicked: attachMenu.show()
            }
        }
    }

    StudioControls.Menu {
        id: attachMenu

        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside
        implicitWidth: 250

        onClosed: {
            attachRepeater.model = {}
        }

        function show() {
            updateModel()
            attachMenu.popup(attachImageButton)
        }

        function updateModel() {
            let imagesModel = root.rootView.getImageAssetsPaths()
            attachRepeater.model = imagesModel
            if (!imagesModel.includes(root.attachedImage))
                root.attachedImage = ""
        }

        Repeater {
            id: attachRepeater

            delegate: StudioControls.MenuItem {
                id: menuItem

                required property string modelData
                text: modelData
                onTriggered: {
                    root.attachedImage = menuItem.text
                    attachedImageImage.source = root.rootView.fullAttachedImageUrl()
                }
            }
        }
    }
}
