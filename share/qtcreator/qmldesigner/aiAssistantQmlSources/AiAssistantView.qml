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
    property alias attachedImage: attachedImageText.text

    color: StudioTheme.Values.themeBackgroundColorNormal

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: StudioTheme.Values.marginTopBottom

        PromptTextBox {
            rootView: root.rootView

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 100
        }

        Text {
            id: attachedImageText

            color: StudioTheme.Values.themeTextColor
        }

        HelperWidgets.AbstractButton {
            id: attachImageButton
            objectName: "AttachImageButton"

            Layout.alignment: Qt.AlignRight

            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.attach_medium
            tooltip: qsTr("Attach an image.\nThe attached image will be analyzed and integrated into the response by the AI.")

            onClicked: attachMenu.show()
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
            let imagesModel = root.rootView.imageAssetsModel()
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
                onTriggered: root.attachedImage = menuItem.text
            }
        }
    }
}
