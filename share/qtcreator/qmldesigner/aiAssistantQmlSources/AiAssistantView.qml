// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    property var rootView: AiAssistantBackend.rootView
    property alias attachedImageSource: attachedImage.source

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

            AssetImage {
                id: attachedImage

                visible: root.attachedImageSource !== ""
                closable: true

                onCloseRequest: {
                    root.attachedImageSource = ""
                }
            }

            HelperWidgets.AbstractButton {
                id: attachImageButton
                objectName: "AttachImageButton"

                style: StudioTheme.Values.viewBarButtonStyle
                buttonIcon: StudioTheme.Constants.attach_medium
                tooltip: qsTr("Attach an image.\nThe attached image will be analyzed and integrated into the response by the AI.")

                onClicked: assetImagesView.showWindow()
            }
        }
    }

    AssetImagesPopup {
        id: assetImagesView

        snapItem: attachImageButton

        onWindowShown: {
            if (!assetImagesView.model.includes(root.attachedImageSource))
                root.attachedImageSource = ""
        }

        onImageClicked: (imageSource) => root.attachedImageSource = imageSource
    }
}
