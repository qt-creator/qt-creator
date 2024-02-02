// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ToolBar

Rectangle {
    id: root
    color: StudioTheme.Values.themeToolbarBackground

    readonly property int mediumBreakpoint: 720
    readonly property int largeBreakpoint: 1200
    readonly property bool flyoutEnabled: root.width < root.largeBreakpoint

    ToolBarBackend {
        id: backend
    }

    Item {
        id: topToolbarOtherMode
        anchors.fill: parent
        visible: !backend.isInDesignMode

        Rectangle {
            id: returnExtended
            height: homeOther.height
            width: backTo.visible ? (homeOther.width + backTo.width + contentRow.spacing + 6) : homeOther.width
            anchors.verticalCenter: topToolbarOtherMode.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            color: StudioTheme.Values.themeToolbarBackground
            radius: StudioTheme.Values.smallRadius
            state: "default"

            Row {
                id: contentRow
                spacing: 6
                anchors.fill: parent

                ToolbarButton {
                    id: homeOther
                    tooltip: backend.isDesignModeEnabled ? qsTr("Switch to Design Mode.")
                                                         : qsTr("Switch to Welcome Mode.")
                    buttonIcon: backend.isDesignModeEnabled ? StudioTheme.Constants.designMode_large
                                                            : StudioTheme.Constants.home_large
                    hover: mouseArea.containsMouse
                    press: mouseArea.pressed

                    onClicked: backend.triggerModeChange()

                }

                Text {
                    id: backTo
                    height: homeOther.height
                    visible: backend.isDesignModeEnabled
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Return to Design")
                    color: StudioTheme.Values.themeTextColor
                }
            }

            MouseArea{
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: homeOther.onClicked()
            }

            states: [
                State {
                    name: "default"
                    when: !mouseArea.containsMouse && !mouseArea.pressed
                },
                State {
                    name: "hover"
                    when: mouseArea.containsMouse && !mouseArea.pressed
                    PropertyChanges {
                        target: returnExtended
                        color: StudioTheme.Values.themeControlBackground_topToolbarHover
                    }
                },
                State {
                    name: "pressed"
                    when: mouseArea.pressed
                    PropertyChanges {
                        target: returnExtended
                        color: StudioTheme.Values.themeInteraction
                    }
                }
            ]
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
            buttonIcon: StudioTheme.Constants.home_large
            onClicked: backend.triggerModeChange()
            tooltip: qsTr("Switch to Welcome Mode.")
        }

        ToolbarButton {
            id: runProject
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: home.right
            anchors.leftMargin: 10
            buttonIcon: StudioTheme.Constants.runProjOutline_large
            style: StudioTheme.ToolbarStyle {
                radius: StudioTheme.Values.smallRadius

                icon: StudioTheme.ControlStyle.IconColors {
                    idle: StudioTheme.Values.themeIdleGreen
                    hover: StudioTheme.Values.themeRunningGreen
                    interaction: "#ffffff"
                    disabled: "#636363"
                }

                background: StudioTheme.ControlStyle.BackgroundColors {
                    idle: StudioTheme.Values.themeControlBackground_toolbarIdle
                    hover: StudioTheme.Values.themeControlBackground_topToolbarHover
                    interaction: StudioTheme.Values.themeInteraction
                    disabled: StudioTheme.Values.themeControlBackground_toolbarIdle
                }

                border: StudioTheme.ControlStyle.BorderColors {
                    idle: StudioTheme.Values.themeControlBackground_toolbarIdle
                    hover: StudioTheme.Values.themeControlBackground_topToolbarHover
                    interaction: StudioTheme.Values.themeInteraction
                    disabled: StudioTheme.Values.themeControlBackground_toolbarIdle
                }
            }

            onClicked: backend.runProject()
            tooltip: qsTr("Run Project")
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

            onClicked: {
                livePreview.trigger()
            }

            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent

                onClicked: {
                    var p = livePreviewButton.mapToGlobal(0, 0)
                    backend.showZoomMenu(p.x, p.y)
                }
            }

            ActionSubscriber {
                id: livePreview
                actionId: "LivePreview"
            }
        }

        StudioControls.TopLevelComboBox {
            id: currentFile
            style: StudioTheme.Values.toolbarStyle
            width: 320 - ((root.width > root.mediumBreakpoint) ? 0 : (root.mediumBreakpoint - root.width))
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: livePreviewButton.right
            anchors.leftMargin: 10
            model: backend.documentModel

            property int currentDocumentIndex: backend.documentIndex
            onCurrentDocumentIndexChanged: currentFile.currentIndex =  currentFile.currentDocumentIndex
            onActivated: backend.openFileByIndex(index)
        }

        ToolbarButton {
            id: backButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: currentFile.right
            anchors.leftMargin: 10
            enabled: backend.canGoBack
            tooltip: qsTr("Go Back")
            buttonIcon: StudioTheme.Constants.previousFile_large
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
            buttonIcon: StudioTheme.Constants.nextFile_large

            onClicked: backend.goForward()
        }

        ToolbarButton {
            id: closeButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: forwardButton.right
            anchors.leftMargin: 10
            tooltip: qsTr("Close")
            buttonIcon: StudioTheme.Constants.closeFile_large

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
            buttonIcon: StudioTheme.Constants.createComponent_large
            visible: !root.flyoutEnabled

            onClicked: moveToComponentBackend.trigger()

            ActionSubscriber {
                id: moveToComponentBackend
                actionId: "MakeComponent"
            }
        }

        ToolbarButton {
            id: enterComponent
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: lockWorkspace.left
            anchors.rightMargin: 10
            enabled: goIntoComponentBackend.available
            tooltip: goIntoComponentBackend.tooltip
            buttonIcon: StudioTheme.Constants.editComponent_large
            visible: !root.flyoutEnabled

            onClicked: goIntoComponentBackend.trigger()

            ActionSubscriber {
                id: goIntoComponentBackend
                actionId: "GoIntoComponent"
            }
        }

        ToolbarButton {
            id: lockWorkspace
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: workspaces.left
            anchors.rightMargin: 10
            tooltip: qsTr("Sets the visible <b>Views</b> to immovable across the Workspaces.")
            buttonIcon: backend.lockWorkspace ? StudioTheme.Constants.lockOn
                                              : StudioTheme.Constants.lockOff
            visible: !root.flyoutEnabled
            checkable: true
            checked: backend.lockWorkspace
            checkedInverted: true

            onClicked: backend.setLockWorkspace(lockWorkspace.checked)
        }

        StudioControls.TopLevelComboBox {
            id: workspaces
            style: StudioTheme.Values.toolbarStyle
            width: 210
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: annotations.left
            anchors.rightMargin: 10
            visible: !root.flyoutEnabled
            model: WorkspaceModel { id: workspaceModel }
            textRole: "displayName"
            valueRole: "fileName"
            suffix: qsTr(" Workspace")

            property int currentWorkspaceIndex: workspaces.indexOfValue(backend.currentWorkspace)

            onCurrentWorkspaceIndexChanged: workspaces.currentIndex = workspaces.currentWorkspaceIndex
            onActivated: backend.setCurrentWorkspace(workspaces.currentValue)
            onCountChanged: workspaces.currentIndex = workspaces.indexOfValue(backend.currentWorkspace)
        }

        ToolbarButton {
            id: annotations
            visible: false
            enabled: backend.isInDesignMode
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: shareButton.left
            anchors.rightMargin: 10
            width: 0
            tooltip: qsTr("Edit Annotations")
            buttonIcon: StudioTheme.Constants.annotations_large

            onClicked: backend.editGlobalAnnoation()
        }

        ToolbarButton {
            id: shareButton
            style: StudioTheme.Values.primaryToolbarStyle
            width: 96
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: moreItems.left
            anchors.rightMargin: 8
            iconFont: StudioTheme.Constants.font
            buttonIcon: qsTr("Share")
            visible: !root.flyoutEnabled
            enabled: backend.isSharingEnabled
            tooltip: shareButton.enabled ? qsTr("Share your project online.") : qsTr("Sharing your project online is disabled in the Community Version.")

            onClicked: backend.shareApplicationOnline()
        }

        ToolbarButton {
            // this needs a pop-up panel where overflow toolbar content goes when toolbar is not wide enough
            id: moreItems
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 10
            tooltip: qsTr("More Items")
            buttonIcon: StudioTheme.Constants.more_medium
            enabled: root.flyoutEnabled
            checkable: true
            checked: window.visible
            checkedInverted: true
            onClicked: {
                if (window.visible) {
                    window.close()
                } else {
                    var originMapped = moreItems.mapToGlobal(0,0)
                    window.x = originMapped.x + moreItems.width - window.width
                    window.y = originMapped.y + moreItems.height + 7
                    window.show()
                    window.requestActivate()
                }
            }
        }

        Window {
            id: window

            readonly property int padding: 6

            width: row.width + window.padding * 2
            height: row.height + workspacesFlyout.height + 3 * window.padding
                    + (workspacesFlyout.popup.opened ? workspacesFlyout.popup.height : 0)
            visible: false
            flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.NoDropShadowWindowHint
            modality: Qt.NonModal
            transientParent: null
            color: "transparent"

            onActiveFocusItemChanged: {
                if (window.activeFocusItem === null && !moreItems.hovered)
                    window.close()
            }

            Rectangle {
                anchors.fill: parent
                color: StudioTheme.Values.themePopupBackground
                radius: StudioTheme.Values.smallRadius

                Column {
                    id: column
                    anchors.margins: window.padding
                    anchors.fill: parent
                    spacing: window.padding

                    Row {
                        id: row
                        spacing: window.padding
                        anchors.horizontalCenter: parent.horizontalCenter

                        ToolbarButton {
                            style: StudioTheme.Values.statusbarButtonStyle
                            anchors.verticalCenter: parent.verticalCenter
                            enabled: moveToComponentBackend.available
                            tooltip: moveToComponentBackend.tooltip
                            buttonIcon: StudioTheme.Constants.createComponent_large

                            onClicked: moveToComponentBackend.trigger()

                            ActionSubscriber {
                                actionId: "MakeComponent"
                            }
                        }

                        ToolbarButton {
                            style: StudioTheme.Values.statusbarButtonStyle
                            anchors.verticalCenter: parent.verticalCenter
                            enabled: goIntoComponentBackend.available
                            tooltip: goIntoComponentBackend.tooltip
                            buttonIcon: StudioTheme.Constants.editComponent_large

                            onClicked: goIntoComponentBackend.trigger()

                            ActionSubscriber {
                                actionId: "GoIntoComponent"
                            }
                        }

                        ToolbarButton {
                            visible: false
                            style: StudioTheme.Values.statusbarButtonStyle
                            anchors.verticalCenter: parent.verticalCenter
                            tooltip: qsTr("Edit Annotations")
                            buttonIcon: StudioTheme.Constants.annotations_large

                            onClicked: backend.editGlobalAnnoation()
                        }

                        ToolbarButton {
                            id: lockWorkspaceFlyout
                            style: StudioTheme.Values.statusbarButtonStyle
                            anchors.verticalCenter: parent.verticalCenter
                            tooltip: lockWorkspace.tooltip
                            buttonIcon: backend.lockWorkspace ? StudioTheme.Constants.lockOn
                                                              : StudioTheme.Constants.lockOff
                            checkable: true
                            checked: backend.lockWorkspace

                            onClicked: backend.setLockWorkspace(lockWorkspaceFlyout.checked)
                        }

                        ToolbarButton {
                            anchors.verticalCenter: parent.verticalCenter
                            style: StudioTheme.Values.primaryToolbarStyle
                            width: shareButton.width
                            iconFont: StudioTheme.Constants.font
                            buttonIcon: qsTr("Share")
                            enabled: backend.isSharingEnabled
                            tooltip: shareButton.enabled ? qsTr("Share your project online.") : qsTr("Sharing your project online is disabled in the Community Version.")

                            onClicked: backend.shareApplicationOnline()
                        }
                    }

                    StudioControls.ComboBox {
                        id: workspacesFlyout
                        anchors.horizontalCenter: parent.horizontalCenter
                        actionIndicatorVisible: false
                        style: StudioTheme.Values.statusbarControlStyle
                        width: row.width
                        maximumPopupHeight: 400
                        model: workspaceModel
                        textRole: "displayName"
                        valueRole: "fileName"
                        currentIndex: workspacesFlyout.indexOfValue(backend.currentWorkspace)

                        onCompressedActivated: backend.setCurrentWorkspace(workspacesFlyout.currentValue)
                        onCountChanged: workspacesFlyout.currentIndex = workspacesFlyout.indexOfValue(backend.currentWorkspace)
                    }
                }
            }
        }
    }
}
