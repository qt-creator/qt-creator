// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme
import StudioWindowManager
import ToolBar

Rectangle {
    id: root
    color: StudioTheme.Values.themeToolbarBackground

    readonly property int mediumBreakpoint: 720
    readonly property int largeBreakpoint: 1200
    readonly property bool flyoutEnabled: root.width < root.largeBreakpoint

    ToolBarBackend { id: backend }

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

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: homeOther.clicked()
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

        SplitButton {
            id: splitButton
            style: StudioTheme.Values.toolbarButtonStyle
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: home.right
            anchors.leftMargin: 10
            width: 160

            runTarget: backend?.runTargetIndex
            runManagerState: backend?.runManagerState

            onClicked: backend.toggleRunning()
            onRunTargetSelected: function(targetName) { backend.selectRunTarget(targetName) }
            onOpenRunTargets: backend.openDeviceManager()
        }

        StudioControls.TopLevelComboBox {
            id: currentFile
            style: StudioTheme.Values.toolbarStyle
            width: 320 - ((root.width > root.mediumBreakpoint) ? 0 : (root.mediumBreakpoint - root.width))
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: splitButton.right
            anchors.leftMargin: 10
            model: backend.documentModel

            property int currentDocumentIndex: backend.documentIndex
            onCurrentDocumentIndexChanged: currentFile.currentIndex =  currentFile.currentDocumentIndex
            onActivated: backend.openFileByIndex(index)
        }

        Text {
            parent: currentFile.contentItem
            visible: backend.isDocumentDirty

            anchors.right: parent.right
            anchors.rightMargin: parent.width - metric.textWidth - 18
            color: StudioTheme.Values.themeTextColor
            text: StudioTheme.Constants.wildcard
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.smallIconFont
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -4

            FontMetrics {
                id: metric
                font: currentFile.font
                property int textWidth: metric.boundingRect(currentFile.currentText).width
            }
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
            anchors.right: backend.isLiteModeEnabled ? shareButton.left : lockWorkspace.left
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
            visible: !root.flyoutEnabled && !backend.isLiteModeEnabled
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
            anchors.right: shareButton.left
            anchors.rightMargin: 10
            visible: !root.flyoutEnabled && !backend.isLiteModeEnabled
            model: WorkspaceModel { id: workspaceModel }
            textRole: "displayName"
            valueRole: "fileName"
            suffix: qsTr(" Workspace")

            property int currentWorkspaceIndex: workspaces.indexOfValue(backend.currentWorkspace)

            onCurrentWorkspaceIndexChanged: workspaces.currentIndex = workspaces.currentWorkspaceIndex
            onActivated: backend.setCurrentWorkspace(workspaces.currentValue)
            onCountChanged: workspaces.currentIndex = workspaces.indexOfValue(backend.currentWorkspace)
        }

        Connections {
            target: WindowManager
            enabled: dvWindow.visible

            function onAboutToQuit() {
                dvWindow.close()
            }

            function onMainWindowVisibleChanged(value) {
                if (!value)
                    dvWindow.close()
            }
        }

        ToolbarButton {
            id: shareButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: moreItems.left
            anchors.rightMargin: 8
            buttonIcon: StudioTheme.Constants.share_large
            enabled: backend.isSharingEnabled
            tooltip: qsTr("You can share your project to Qt Design Viewer web service.<br><br>To be able to use the sharing service, you need to sign in with your Qt Account details.")

            checkable: true
            checked: dvWindow.visible
            checkedInverted: true

            onClicked: {
                if (dvWindow.visible) {
                    dvWindow.close()
                } else {
                    var originMapped = shareButton.mapToGlobal(0,0)
                    dvWindow.x = originMapped.x + shareButton.width - dvWindow.width
                    dvWindow.y = originMapped.y + shareButton.height
                    dvWindow.show()
                    dvWindow.requestActivate()
                }
            }

            Window {
                id: dvWindow

                width: 300
                height: stackLayout.children[stackLayout.currentIndex].implicitHeight

                visible: false
                flags: Qt.FramelessWindowHint | Qt.Tool | Qt.NoDropShadowWindowHint
                modality: Qt.NonModal
                transientParent: null
                color: "transparent"

                onActiveFocusItemChanged: {
                    if (dvWindow.activeFocusItem === null && !dvWindow.active
                        && !shareButton.hover
                        && !backend.designViewerConnector.isWebViewerVisible)
                        dvWindow.close()
                }

                onVisibleChanged: {
                    // if visible and logged in
                    // fetch user info
                }

                onClosing: {
                    if (shareNotification.hasFinished())
                        shareNotification.visible = false
                }

                function formatBytes(bytes, decimals = 2) {
                    if (!+bytes)
                        return '0 Bytes'

                    const k = 1024
                    const dm = decimals < 0 ? 0 : decimals
                    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
                    const i = Math.floor(Math.log(bytes) / Math.log(k))

                    return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
                }

                Connections {
                    target: backend.designViewerConnector

                    function onUserInfoReceived(reply: var) {
                        let jsonReply = JSON.parse(reply)

                        loggedInPage.email = jsonReply.email
                        loggedInPage.license = jsonReply.license
                        loggedInPage.storageUsed = jsonReply.storageUsed
                        loggedInPage.storageLimit = jsonReply.storageLimit
                    }
                }

                StackLayout {
                    id: stackLayout

                    property int internalMargin: 8

                    anchors.fill: parent
                    currentIndex: backend.designViewerConnector.connectorStatus

                    // Fetching
                    Rectangle {
                        id: fetchingPage
                        color: StudioTheme.Values.themePopupBackground
                        Layout.fillWidth: true
                        implicitHeight: 200

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: StackView.status === StackView.Active // TODO test
                        }
                    }

                    // NotLoggedIn
                    Rectangle {
                        id: notLoggedInPage
                        color: StudioTheme.Values.themePopupBackground
                        Layout.fillWidth: true
                        implicitHeight: menuColumn.implicitHeight

                        Column {
                            id: menuColumn
                            anchors.fill: parent
                            padding: StudioTheme.Values.border

                            MenuItemDelegate {
                                width: parent.width

                                myText: qsTr("Sign in")
                                myIcon: StudioTheme.Constants.signin_medium

                                onClicked: backend.designViewerConnector.login()
                            }
                        }
                    }

                    // LoggedIn
                    Rectangle {
                        id: loggedInPage

                        property string email
                        property string license
                        property var storageUsed
                        property var storageLimit

                        color: StudioTheme.Values.themePopupBackground

                        Layout.fillWidth: true
                        implicitHeight: loggedInPageColumn.implicitHeight

                        Column {
                            id: loggedInPageColumn
                            anchors.fill: parent
                            padding: StudioTheme.Values.border
                            spacing: 0

                            MenuItemDelegate {
                                id: shareMenuItem
                                width: parent.width

                                myText: qsTr("Share")
                                myIcon: StudioTheme.Constants.upload_medium

                                onClicked: backend.designViewerConnector.uploadCurrentProject()
                            }

                            ShareNotification {
                                id: shareNotification

                                Connections {
                                    target: backend.designViewerConnector

                                    function onProjectUploadProgress(progress: var) {
                                        shareNotification.setProgress(progress)
                                    }

                                    function onProjectUploaded() {
                                        shareNotification.type = ShareNotification.NotificationType.Success
                                        shareNotification.setHelperText(qsTr("Upload succeeded."))

                                        shareMenuItem.enabled = true
                                    }

                                    function onProjectUploadError(errorCode: int, message: string) {
                                        shareNotification.type = ShareNotification.NotificationType.Error
                                        shareNotification.setHelperText(qsTr("Upload failed (" + errorCode + ")."))

                                        shareMenuItem.enabled = true
                                    }

                                    function onProjectIsPacking() {
                                        shareNotification.type = ShareNotification.NotificationType.Indeterminate
                                        shareNotification.setText(qsTr("Packing"))
                                        shareNotification.visible = true

                                        shareMenuItem.enabled = false
                                    }

                                    function onProjectPackingFailed(errorString: string) {
                                        shareNotification.type = ShareNotification.NotificationType.Error
                                        shareNotification.setHelperText(qsTr("Packing failed."))

                                        shareMenuItem.enabled = true
                                    }

                                    function onProjectIsUploading() {
                                        shareNotification.type = ShareNotification.NotificationType.Normal
                                        shareNotification.setText(qsTr("Uploading"))
                                        shareNotification.visible = true

                                        shareMenuItem.enabled = false
                                    }
                                }
                            }

                            MenuItemDelegate {
                                width: parent.width

                                myText: qsTr("Manage shared projects")
                                myIcon: StudioTheme.Constants.openLink

                                onClicked: Qt.openUrlExternally(backend.designViewerConnector.loginUrl())
                            }

                            Rectangle {
                                width: parent.width
                                height: StudioTheme.Values.height * 2
                                color: StudioTheme.Values.themePanelBackground

                                Row {
                                    anchors.fill: parent
                                    spacing: 0

                                    Label {
                                        id: iconLabel
                                        width: StudioTheme.Values.topLevelComboHeight
                                        height: StudioTheme.Values.topLevelComboHeight

                                        anchors.verticalCenter: parent.verticalCenter

                                        color: StudioTheme.Values.themeTextColor
                                        font.family: StudioTheme.Constants.iconFont.family
                                        font.pixelSize: StudioTheme.Values.topLevelComboIcon
                                        text: StudioTheme.Constants.user_medium
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Column {
                                        width: parent.width - parent.spacing - iconLabel.width - 8 // TODO 8 is the margin
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 4

                                        Text {
                                            color: StudioTheme.Values.themeTextColor
                                            text: loggedInPage.email ?? ""
                                        }

                                        RowLayout {
                                            width: parent.width

                                            Text {
                                                Layout.fillWidth: true
                                                color: StudioTheme.Values.themeTextColor
                                                text: loggedInPage.license ?? ""
                                            }

                                            Text {
                                                color: StudioTheme.Values.themeTextColor
                                                text: `${dvWindow.formatBytes(loggedInPage.storageUsed)} / ${dvWindow.formatBytes(loggedInPage.storageLimit)}`
                                            }
                                        }
                                    }
                                }
                            }

                            MenuItemDelegate {
                                width: parent.width

                                myText: qsTr("Sign out")
                                myIcon: StudioTheme.Constants.signout_medium

                                onClicked: backend.designViewerConnector.logout()
                            }
                        }
                    }
                }
            }
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
            readonly property int morePopupWidth: Math.max(180, row.width)

            width: window.morePopupWidth + window.padding * 2
            height: row.height + (backend.isLiteModeEnabled ? 0 : workspacesFlyout.height)
                    + (backend.isLiteModeEnabled ? 2 : 3) * window.padding
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
                            visible: !backend.isLiteModeEnabled

                            onClicked: backend.setLockWorkspace(lockWorkspaceFlyout.checked)
                        }
                    }

                    StudioControls.ComboBox {
                        id: workspacesFlyout
                        anchors.horizontalCenter: parent.horizontalCenter
                        actionIndicatorVisible: false
                        style: StudioTheme.Values.statusbarControlStyle
                        width: window.morePopupWidth
                        maximumPopupHeight: 400
                        model: workspaceModel
                        textRole: "displayName"
                        valueRole: "fileName"
                        currentIndex: workspacesFlyout.indexOfValue(backend.currentWorkspace)
                        visible: !backend.isLiteModeEnabled

                        onCompressedActivated: backend.setCurrentWorkspace(workspacesFlyout.currentValue)
                        onCountChanged: workspacesFlyout.currentIndex = workspacesFlyout.indexOfValue(backend.currentWorkspace)
                    }
                }
            }
        }
    }
}
