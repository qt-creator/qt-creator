// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import "../toolbar"
import HelperWidgets 2.0

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
            anchors.topMargin: 3
            anchors.leftMargin: 4
            spacing: 29

            ToolbarButton {
                id: settingButton
                style: StudioTheme.Values.statusbarButtonStyle
                buttonIcon: StudioTheme.Constants.settings_medium
                checkable: true
                checkedInverted: true
                checked: backend.isInSessionMode
                onClicked: settingButton.checked ? backend.triggerProjectSettings() : backend.triggerModeChange()
                enabled: backend.projectOpened
                tooltip: qsTr("Set runtime configuration for the project.")
            }

            Text {
                height: StudioTheme.Values.statusbarButtonStyle.controlSize.height
                color: StudioTheme.Values.themeTextColor
                text: qsTr("Kit")
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                ToolTipArea {
                    anchors.fill: parent
                    tooltip: qsTr("Choose a predefined kit for the runtime configuration of the project.")
                }
            }

            StudioControls.TopLevelComboBox {
                id: kits
                style: StudioTheme.Values.statusbarControlStyle
                width: 160
                model: backend.kits
                onActivated: backend.setCurrentKit(kits.currentIndex)
                openUpwards: true
                enabled: (backend.isInDesignMode || (backend.isInEditMode && backend.projectOpened))
                         && backend.isQt6 && !backend.isMCUs
                property int kitIndex: backend.currentKit
                onKitIndexChanged: kits.currentIndex = backend.currentKit
            }

            Text {
                height: StudioTheme.Values.statusbarButtonStyle.controlSize.height
                color: StudioTheme.Values.themeTextColor
                text: qsTr("Style")
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                ToolTipArea {
                    anchors.fill: parent
                    tooltip: qsTr("Choose a style for the Qt Quick Controls of the project.")
                }
            }

            StudioControls.TopLevelComboBox {
                id: styles
                style: StudioTheme.Values.statusbarControlStyle
                width: 160
                model: backend.styles
                onActivated: backend.setCurrentStyle(styles.currentIndex)
                openUpwards: true
                enabled: backend.isInDesignMode && !backend.isMCUs
                property int currentStyleIndex: backend.currentStyle
                onCurrentStyleIndexChanged: styles.currentIndex = backend.currentStyle
            }
        }
    }
}
