// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick 2.15
import QtQuick.Controls 2.15

import StudioControls
import StudioTheme 1.0 as StudioTheme

import ToolBar 1.0

Rectangle {
    id: toolbarContainer
    color: "#2d2d2d"
    border.color: "#00000000"

    height: 56
    width: 2024

    ToolBarBackend {
        id: backend
    }

    Item {
        id: topToolbarOtherMode
        anchors.fill: parent
        visible: !backend.isInDesignMode

        ToolbarButton {
            id: homeOther
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            enabled: backend.isDesignModeEnabled
            tooltip: qsTr("Switch to Design Mode")
            buttonIcon: StudioTheme.Constants.topToolbar_designMode

            onClicked: backend.triggerModeChange()
        }
    }

    Item {
        id: topToolbar
        anchors.fill: parent
        visible: backend.isInDesignMode

        ToolbarButton {
            id: home
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            buttonIcon: StudioTheme.Constants.topToolbar_home

            onClicked: backend.triggerModeChange()
        }

        ToolbarButton {
            id: runProject
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: home.right
            anchors.leftMargin: 10
            buttonIcon: StudioTheme.Constants.topToolbar_runProject
            style: StudioTheme.ToolbarStyle {
                icon: StudioTheme.ControlStyle.IconColors {
                    idle: "#649a5d"
                }
            }

            onClicked: backend.runProject()
        }

        ToolbarButton {
            id: livePreviewButton
            style: StudioTheme.Values.primaryToolbarStyle
            width: 96
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: runProject.right
            anchors.leftMargin: 10
            iconFont: StudioTheme.Constants.font
            buttonIcon: qsTr("Live Preview")

            onClicked: livePreview.trigger()

            ActionSubscriber {
                id: livePreview
                actionId: "LivePreview"
            }
        }

        TopLevelComboBox {
            id: currentFile
            style: StudioTheme.Values.toolbarStyle
            width: 320
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: livePreviewButton.right
            anchors.leftMargin: 10
            model: backend.documentModel
            currentIndex: backend.documentIndex

            onActivated: backend.openFileByIndex(index)
        }

        ToolbarButton {
            id: backButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: currentFile.right
            anchors.leftMargin: 10
            enabled: backend.canGoBack
            tooltip: qsTr("Go Back")
            buttonIcon: StudioTheme.Constants.topToolbar_navFile
            iconRotation: 0

            onClicked: backend.goBackward()
        }

        ToolbarButton {
            id: forwardButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: backButton.right
            anchors.leftMargin: 10
            enabled: backend.canGoForward
            tooltip: qsTr("Go Forward")
            buttonIcon: StudioTheme.Constants.topToolbar_navFile
            iconRotation: 180

            onClicked: backend.goForward()
        }

        ToolbarButton {
            id: closeButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: forwardButton.right
            anchors.leftMargin: 10
            tooltip: qsTr("Close")
            buttonIcon: StudioTheme.Constants.topToolbar_closeFile

            onClicked: backend.closeCurrentDocument()
        }

        CrumbleBar {
            id: flickable
            height: 36
            anchors.left: closeButton.right
            anchors.leftMargin: 10
            anchors.right: createComponent.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            model: CrumbleBarModel {
                id: crumbleBarModel
            }

            onClicked: crumbleBarModel.onCrumblePathElementClicked(index)
        }

        ToolbarButton {
            id: createComponent
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: enterComponent.left
            anchors.rightMargin: 10
            enabled: moveToComponentBackend.available
            tooltip: moveToComponentBackend.tooltip
            buttonIcon: StudioTheme.Constants.topToolbar_makeComponent

            onClicked: moveToComponentBackend.trigger()

            ActionSubscriber {
                id: moveToComponentBackend
                actionId: "MakeComponent"
            }
        }

        ToolbarButton {
            id: enterComponent
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: workspaces.left
            anchors.rightMargin: 10
            enabled: goIntoComponentBackend.available
            tooltip: goIntoComponentBackend.tooltip
            buttonIcon: StudioTheme.Constants.topToolbar_enterComponent

            onClicked: goIntoComponentBackend.trigger()

            ActionSubscriber {
                id: goIntoComponentBackend
                actionId: "GoIntoComponent"
            }
        }

        TopLevelComboBox {
            id: workspaces
            style: StudioTheme.Values.toolbarStyle
            width: 210
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: annotations.left
            anchors.rightMargin: 10
            model: backend.workspaces
            onCurrentTextChanged: backend.setCurrentWorkspace(workspaces.currentText)
        }

        ToolbarButton {
            id: annotations
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: shareButton.left
            anchors.rightMargin: 10
            tooltip: qsTr("Edit Annotations")
            buttonIcon: StudioTheme.Constants.topToolbar_annotations

            onClicked: backend.editGlobalAnnoation()
        }

        ToolbarButton {
            id: shareButton
            style: StudioTheme.Values.primaryToolbarStyle
            width: 96
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            iconFont: StudioTheme.Constants.font
            buttonIcon: qsTr("Share")

            onClicked: backend.shareApplicationOnline()
        }
    }
}
