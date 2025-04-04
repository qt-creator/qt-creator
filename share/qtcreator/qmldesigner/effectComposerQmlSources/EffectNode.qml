// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Rectangle {
    id: root

    width: 140
    height: 32

    color: mouseArea.containsMouse && modelData.canBeAdded && root.enabled
           ? StudioTheme.Values.themeControlBackgroundInteraction : "transparent"

    signal addEffectNode(var nodeQenPath)
    signal removeEffectNodeFromLibrary(var nodeName)

    ToolTipArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        visible: root.enabled

        tooltip: modelData.canBeAdded ? modelData.nodeDescription
                                      : qsTr("An effect with same properties already exists, this effect cannot be added.")

        onClicked: {
            if (modelData.canBeAdded)
                root.addEffectNode(modelData.nodeQenPath)
        }
    }

    Row {
        spacing: 5
        anchors.fill: parent

        IconImage {
            id: nodeIcon

            width: 32
            height: 32

            color: modelData.canBeAdded ? StudioTheme.Values.themeTextColor
                                        : StudioTheme.Values.themeTextColorDisabled
            source: modelData.nodeIcon
        }

        Text {
            text: modelData.nodeName
            color: modelData.canBeAdded ? StudioTheme.Values.themeTextColor
                                        : StudioTheme.Values.themeTextColorDisabled

            font.pointSize: StudioTheme.Values.smallFontSize
            anchors.verticalCenter: nodeIcon.verticalCenter
            wrapMode: Text.WordWrap
            width: parent.width - parent.spacing - nodeIcon.width

            IconButton {
                id: removeButton

                visible: root.enabled && modelData.canBeRemoved
                         && (mouseArea.containsMouse || removeButton.containsMouse)
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: StudioTheme.Values.iconAreaWidth / 4
                icon: StudioTheme.Constants.close_small
                transparentBg: false
                buttonSize: StudioTheme.Values.iconAreaWidth
                iconSize: StudioTheme.Values.smallIconFontSize
                iconColor: StudioTheme.Values.themeTextColor
                iconScale: removeButton.containsMouse ? 1.2 : 1
                implicitWidth: width
                tooltip: qsTr("Remove custom effect from the library.")

                onPressed: (event) => {
                    root.removeEffectNodeFromLibrary(modelData.nodeName)
                }
            }
        }
    }
}
