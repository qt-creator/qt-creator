/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: rootItem

    property var selectedAssets: ({})
    property int allExpandedState: 0
    property string contextFilePath: ""
    property var contextDir: undefined
    property bool isDirContextMenu: false

    DropArea {
        id: dropArea

        property var files // list of supported dropped files

        enabled: true
        anchors.fill: parent

        onEntered: (drag)=> {
            files = []
            for (var i = 0; i < drag.urls.length; ++i) {
                var url = drag.urls[i].toString();
                if (url.startsWith("file:///")) // remove file scheme (happens on Windows)
                    url = url.substr(8)
                var ext = url.slice(url.lastIndexOf('.') + 1).toLowerCase()
                if (rootView.supportedDropSuffixes().includes('*.' + ext))
                    files.push(url)
            }

            if (files.length === 0)
                drag.accepted = false;
        }

        onDropped: {
            if (files.length > 0)
                rootView.handleFilesDrop(files)
        }
    }

    MouseArea { // right clicking the empty area of the view
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (!assetsModel.isEmpty) {
                contextFilePath = ""
                contextDir = assetsModel.rootDir()
                isDirContextMenu = false
                contextMenu.popup()
            }
        }
    }

    // called from C++ to close context menu on focus out
    function handleViewFocusOut()
    {
        contextMenu.close()
        selectedAssets = {}
        selectedAssetsChanged()
    }

    RegExpValidator {
        id: folderNameValidator
        regExp: /^(\w[^*/><?\\|:]*)$/
    }

    Dialog {
        id: renameFolderDialog

        title: qsTr("Rename folder")
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        implicitWidth: 280
        modal: true

        property bool renameError: false

        contentItem: Column {
            spacing: 2

            StudioControls.TextField {
                id: folderRename

                actionIndicator.visible: false
                translationIndicator.visible: false
                width: renameFolderDialog.width - 12
                validator: folderNameValidator

                onEditChanged: renameFolderDialog.renameError = false
                Keys.onEnterPressed: btnRename.onClicked()
                Keys.onReturnPressed: btnRename.onClicked()
            }

            Text {
                text: qsTr("Folder Name cannot be empty.")
                color: "#ff0000"
                visible: folderRename.text === "" && !renameFolderDialog.renameError
            }

            Text {
                text: qsTr("Could not rename directory. Make sure no folder with the same name exists.")
                wrapMode: Text.WordWrap
                width: renameFolderDialog.width
                color: "#ff0000"
                visible: renameFolderDialog.renameError
            }

            Item { // spacer
                width: 1
                height: 10
            }

            Text {
                text: qsTr("If the folder has assets in use, renaming it might cause the project to not work correctly.")
                color: StudioTheme.Values.themeTextColor
                wrapMode: Text.WordWrap
                width: renameFolderDialog.width
                leftPadding: 10
                rightPadding: 10
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right

                Button {
                    id: btnRename

                    text: qsTr("Rename")
                    enabled: folderRename.text !== ""
                    onClicked: {
                        var success = assetsModel.renameFolder(contextDir.dirPath, folderRename.text)
                        if (success)
                            renameFolderDialog.accept()

                        renameFolderDialog.renameError = !success
                    }
                }

                Button {
                    text: qsTr("Cancel")
                    onClicked: renameFolderDialog.reject()
                }
            }
        }

        onOpened: {
            folderRename.text = contextDir.dirName
            folderRename.selectAll()
            folderRename.forceActiveFocus()
            renameFolderDialog.renameError = false
        }
    }

    Dialog {
        id: newFolderDialog

        title: qsTr("Create new folder")
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        modal: true

        contentItem: Column {
            spacing: 2

            Row {
                Text {
                    text: qsTr("Folder Name: ")
                    anchors.verticalCenter: parent.verticalCenter
                    color: StudioTheme.Values.themeTextColor
                }

                StudioControls.TextField {
                    id: folderName

                    actionIndicator.visible: false
                    translationIndicator.visible: false
                    validator: folderNameValidator

                    Keys.onEnterPressed: btnCreate.onClicked()
                    Keys.onReturnPressed: btnCreate.onClicked()
                }
            }

            Text {
                text: qsTr("Folder Name cannot be empty.")
                color: "#ff0000"
                anchors.right: parent.right
                visible: folderName.text === ""
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right

                Button {
                    id: btnCreate

                    text: qsTr("Create")
                    enabled: folderName.text !== ""
                    onClicked: {
                        assetsModel.addNewFolder(contextDir.dirPath + '/' + folderName.text)
                        newFolderDialog.accept()
                    }
                }

                Button {
                    text: qsTr("Cancel")
                    onClicked: newFolderDialog.reject()
                }
            }
        }

        onOpened: {
            folderName.text = "New folder"
            folderName.selectAll()
            folderName.forceActiveFocus()
        }
    }

    Dialog {
        id: confirmDeleteFolderDialog

        title: qsTr("Folder not empty")
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        implicitWidth: 300
        modal: true

        contentItem: Column {
            spacing: 20
            width: parent.width

            Text {
                id: folderNotEmpty

                text: qsTr("Folder '%1' is not empty. Are you sure you want to delete it?")
                            .arg(contextDir ? contextDir.dirName : "")
                color: StudioTheme.Values.themeTextColor
                wrapMode: Text.WordWrap
                width: confirmDeleteFolderDialog.width
                leftPadding: 10
                rightPadding: 10

                Keys.onEnterPressed: btnDelete.onClicked()
                Keys.onReturnPressed: btnDelete.onClicked()
            }

            Text {
                text: qsTr("If the folder has assets in use, deleting it might cause the project to not work correctly.")
                color: StudioTheme.Values.themeTextColor
                wrapMode: Text.WordWrap
                width: confirmDeleteFolderDialog.width
                leftPadding: 10
                rightPadding: 10
            }

            Row {
                anchors.right: parent.right
                Button {
                    id: btnDelete

                    text: qsTr("Delete")

                    onClicked: {
                        assetsModel.deleteFolder(contextDir.dirPath)
                        confirmDeleteFolderDialog.accept()
                    }
                }

                Button {
                    text: qsTr("Cancel")
                    onClicked: confirmDeleteFolderDialog.reject()
                }
            }
        }

        onOpened: folderNotEmpty.forceActiveFocus()
    }

    ScrollView { // TODO: experiment using ListView instead of ScrollView + Column
        id: assetsView
        anchors.fill: parent
        interactive: assetsView.verticalScrollBarVisible

        Item {
            StudioControls.Menu {
                id: contextMenu

                StudioControls.MenuItem {
                    text: qsTr("Expand All")
                    enabled: allExpandedState !== 1
                    visible: isDirContextMenu
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.toggleExpandAll(true)
                }

                StudioControls.MenuItem {
                    text: qsTr("Collapse All")
                    enabled: allExpandedState !== 2
                    visible: isDirContextMenu
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.toggleExpandAll(false)
                }

                StudioControls.MenuSeparator {
                    visible: isDirContextMenu
                    height: visible ? StudioTheme.Values.border : 0
                }

                StudioControls.MenuItem {
                    text: qsTr("Delete File")
                    visible: contextFilePath
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.deleteFile(contextFilePath)
                }

                StudioControls.MenuSeparator {
                    visible: contextFilePath
                    height: visible ? StudioTheme.Values.border : 0
                }

                StudioControls.MenuItem {
                    text: qsTr("Rename Folder")
                    visible: isDirContextMenu
                    height: visible ? implicitHeight : 0
                    onTriggered: renameFolderDialog.open()
                }

                StudioControls.MenuItem {
                    text: qsTr("New Folder")
                    onTriggered: newFolderDialog.open()
                }

                StudioControls.MenuItem {
                    text: qsTr("Delete Folder")
                    visible: isDirContextMenu
                    height: visible ? implicitHeight : 0
                    onTriggered: {
                        var dirEmpty = !(contextDir.dirsModel && contextDir.dirsModel.rowCount() > 0)
                                    && !(contextDir.filesModel && contextDir.filesModel.rowCount() > 0);

                        if (dirEmpty)
                            assetsModel.deleteFolder(contextDir.dirPath)
                        else
                            confirmDeleteFolderDialog.open()
                    }
                }
            }
        }

        Column {
            Repeater {
                model: assetsModel // context property
                delegate: dirSection
            }

            Component {
                id: dirSection

                Section {
                    width: assetsView.width -
                           (assetsView.verticalScrollBarVisible ? assetsView.verticalThickness : 0) - 5
                    caption: dirName
                    sectionHeight: 30
                    sectionFontSize: 15
                    leftPadding: 0
                    topPadding: dirDepth > 0 ? 5 : 0
                    bottomPadding: 0
                    hideHeader: dirDepth === 0
                    showLeftBorder: dirDepth > 0
                    expanded: dirExpanded
                    visible: !assetsModel.isEmpty && dirVisible
                    expandOnClick: false
                    useDefaulContextMenu: false

                    onToggleExpand: {
                        dirExpanded = !dirExpanded
                    }

                    onShowContextMenu: {
                        contextFilePath = ""
                        contextDir = model
                        isDirContextMenu = true
                        allExpandedState = assetsModel.getAllExpandedState()
                        contextMenu.popup()
                    }

                    Column {
                        spacing: 5
                        leftPadding: 5

                        Repeater {
                            model: dirsModel
                            delegate: dirSection
                        }

                        Repeater {
                            model: filesModel
                            delegate: fileSection
                        }

                        Text {
                            text: qsTr("Empty folder")
                            color: StudioTheme.Values.themeTextColorDisabled
                            font.pixelSize: 12
                            visible: !(dirsModel && dirsModel.rowCount() > 0)
                                  && !(filesModel && filesModel.rowCount() > 0)

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                onClicked: {
                                    contextFilePath = ""
                                    contextDir = model
                                    isDirContextMenu = true
                                    contextMenu.popup()
                                }
                            }
                        }
                    }
                }
            }

            Component {
                id: fileSection

                Rectangle {
                    width: assetsView.width -
                           (assetsView.verticalScrollBarVisible ? assetsView.verticalThickness : 0)
                    height: img.height
                    color: selectedAssets[filePath] ? StudioTheme.Values.themeInteraction
                                                    : (mouseArea.containsMouse ? StudioTheme.Values.themeSectionHeadBackground
                                                                               : "transparent")

                    Row {
                        spacing: 5

                        Image {
                            id: img
                            asynchronous: true
                            width: 48
                            height: 48
                            source: "image://qmldesigner_assets/" + filePath
                        }

                        Text {
                            text: fileName
                            color: StudioTheme.Values.themeTextColor
                            font.pixelSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    readonly property string suffix: fileName.substr(-4)
                    readonly property bool isFont: suffix === ".ttf" || suffix === ".otf"
                    property bool currFileSelected: false

                    MouseArea {
                        id: mouseArea

                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onExited: tooltipBackend.hideTooltip()
                        onCanceled: tooltipBackend.hideTooltip()
                        onPositionChanged: tooltipBackend.reposition()
                        onPressed: (mouse)=> {
                            forceActiveFocus()
                            if (mouse.button === Qt.LeftButton) {
                                var ctrlDown = mouse.modifiers & Qt.ControlModifier
                                if (!selectedAssets[filePath] && !ctrlDown)
                                    selectedAssets = {}
                                currFileSelected = ctrlDown ? !selectedAssets[filePath] : true
                                selectedAssets[filePath] = currFileSelected
                                selectedAssetsChanged()

                                var selectedAssetsArr = []
                                for (var assetPath in selectedAssets) {
                                    if (selectedAssets[assetPath])
                                        selectedAssetsArr.push(assetPath)
                                }

                                if (currFileSelected)
                                    rootView.startDragAsset(selectedAssetsArr, mapToGlobal(mouse.x, mouse.y))
                            } else {
                                contextFilePath = filePath
                                contextDir = model.fileDir

                                tooltipBackend.hideTooltip()
                                isDirContextMenu = false
                                contextMenu.popup()
                            }
                        }

                        onReleased: (mouse)=> {
                            if (mouse.button === Qt.LeftButton) {
                                if (!(mouse.modifiers & Qt.ControlModifier))
                                    selectedAssets = {}
                                selectedAssets[filePath] = currFileSelected
                                selectedAssetsChanged()
                            }
                        }

                        ToolTip {
                            visible: !isFont && mouseArea.containsMouse && !contextMenu.visible
                            text: filePath
                            delay: 1000
                        }

                        Timer {
                            interval: 1000
                            running: mouseArea.containsMouse
                            onTriggered: {
                                if (suffix === ".ttf" || suffix === ".otf") {
                                    tooltipBackend.name = fileName
                                    tooltipBackend.path = filePath
                                    tooltipBackend.showTooltip()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Placeholder when the assets panel is empty
    Column {
        id: colNoAssets
        visible: assetsModel.isEmpty && !rootView.searchActive

        spacing: 20
        x: 20
        width: rootItem.width - 2 * x
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: qsTr("Looks like you don't have any assets yet.")
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: 18
            width: colNoAssets.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Image {
            source: "image://qmldesigner_assets/browse"
            anchors.horizontalCenter: parent.horizontalCenter
            scale: maBrowse.containsMouse ? 1.2 : 1
            Behavior on scale {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutQuad
                }
            }

            MouseArea {
                id: maBrowse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: rootView.handleAddAsset();
            }
        }

        Text {
            text: qsTr("Drag-and-drop your assets here or click the '+' button to browse assets from the file system.")
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: 18
            width: colNoAssets.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    Text {
        text: qsTr("No match found.")
        x: 20
        y: 10
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: 12
        visible: assetsModel.isEmpty && rootView.searchActive
    }
}
