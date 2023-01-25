// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import StudioControls
import StudioTheme 1.0 as StudioTheme
import "../toolbar"

import ToolBar 1.0

Item {
    id: toolbarContainer

    height: 24
    width: 2024

    ToolBarBackend {
        id: backend
    }

    Rectangle {

        color: "#2d2d2d"
        anchors.fill: parent

        ToolbarButton {
            id: settingButton
            x: 16
            width: 24

            buttonIcon: StudioTheme.Constants.actionIconBinding
            anchors.verticalCenter: parent.verticalCenter
            onClicked: backend.triggerProjectSettings()
        }

        TopLevelComboBox {
            id: kits
            style: StudioTheme.ControlStyle {
                controlSize: Qt.size(StudioTheme.Values.topLevelComboWidth, 24)
                baseIconFontSize: StudioTheme.Values.topLevelComboIcon
                smallIconFontSize: 8
            }

            width: 160
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: settingButton.right
            anchors.leftMargin: 32
            model: [ "Qt 6", "Qt 5", "Boot2Qt", "Android" ]
            //onActivated: backend.setCurrentWorkspace(workspaces.currentText)
            openUpwards: true
            enabled: backend.isInDesignMode
        }

        TopLevelComboBox {
            id: styles
            style: StudioTheme.ControlStyle {
                controlSize: Qt.size(StudioTheme.Values.topLevelComboWidth, 24)
                baseIconFontSize: StudioTheme.Values.topLevelComboIcon
                smallIconFontSize: 8
            }

            width: 160
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: kits.right
            anchors.leftMargin: 32
            model: [ "Basic", "Default", "Universal", "Material" ]
            //onActivated: backend.setCurrentWorkspace(workspaces.currentText)
            openUpwards: true
            enabled: backend.isInDesignMode
        }


        Rectangle {
            color: "red"
            width: 2
            height: 24
        }

        Rectangle {
            color: "green"
            width: 2
            height: 24
            x: toolbarContainer.width - 2
        }

    }
}
