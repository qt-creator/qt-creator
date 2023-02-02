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

    height: 41
    width: 4048

    ToolBarBackend {
        id: backend
    }

    Rectangle {

        color: StudioTheme.Values.themeStatusbarBackground
        anchors.fill: parent
        Row {
            anchors.fill: parent
            anchors.bottomMargin: 5
            anchors.leftMargin: 5
            spacing: 29
            ToolbarButton {
                id: settingButton
                style: StudioTheme.Values.statusbarButtonStyle
                buttonIcon: StudioTheme.Constants.settings_medium
                anchors.verticalCenter: parent.verticalCenter
                onClicked: backend.triggerProjectSettings()
            }


            Text {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: StudioTheme.Values.themeTextColor
                text: "Kit"
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            TopLevelComboBox {
                id: kits
                style: StudioTheme.Values.statusbarControlStyle

                width: 160
                anchors.verticalCenter: parent.verticalCenter
                //anchors.left: settingButton.right
                //anchors.leftMargin: 32
                model: [ "Qt 6", "Qt 5", "Boot2Qt", "Android" ]
                //onActivated: backend.setCurrentWorkspace(workspaces.currentText)
                openUpwards: true
                enabled: backend.isInDesignMode
            }

            Text {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: StudioTheme.Values.themeTextColor
                text: "Style"
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            TopLevelComboBox {
                id: styles
                style: StudioTheme.Values.statusbarControlStyle
                width: 160
                anchors.verticalCenter: parent.verticalCenter
               // anchors.left: kits.right
               // anchors.leftMargin: 32
                model: backend.styles
                onActivated: backend.setCurrentStyle(styles.currentIndex)
                openUpwards: true
                enabled: backend.isInDesignMode
                currentIndex: backend.currentStyle
            }
        }


//        Rectangle {
//            color: "red"
//            width: 2
//            height: 41
//        }

//        Rectangle {
//            color: "green"
//            width: 2
//            height: 41
//            x: toolbarContainer.width - 2
//        }

    }
}
