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
    id: root

    property var selectedAssets: ({})
    property int allExpandedState: 0
    property string contextFilePath: ""
    property var contextDir: undefined
    property bool isDirContextMenu: false

    // Array of supported externally dropped files that are imported as-is
    property var dropSimpleExtFiles: []

    // Array of supported externally dropped files that trigger custom import process
    property var dropComplexExtFiles: []

    function clearSearchFilter()
    {
        searchBox.text = "";
    }

    function updateDropExtFiles(drag)
    {
        root.dropSimpleExtFiles = []
        root.dropComplexExtFiles = []
        var simpleSuffixes = rootView.supportedAssetSuffixes(false);
        var complexSuffixes = rootView.supportedAssetSuffixes(true);
        for (const u of drag.urls) {
            var url = u.toString();
            var ext = '*.' + url.slice(url.lastIndexOf('.') + 1).toLowerCase()
            if (simpleSuffixes.includes(ext))
                root.dropSimpleExtFiles.push(url)
            else if (complexSuffixes.includes(ext))
                root.dropComplexExtFiles.push(url)
        }

        drag.accepted = root.dropSimpleExtFiles.length > 0 || root.dropComplexExtFiles.length > 0
    }

    DropArea { // handles external drop on empty area of the view (goes to root folder)
        id: dropArea
        y: assetsView.y + assetsView.contentHeight + 5
        width: parent.width
        height: parent.height - y

        onEntered: (drag)=> {
            root.updateDropExtFiles(drag)
        }

        onDropped: {
            rootView.handleExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles,
                                        assetsModel.rootDir().dirPath)
        }

        Canvas { // marker for the drop area
            id: dropCanvas
            anchors.fill: parent
            visible: dropArea.containsDrag && root.dropSimpleExtFiles.length > 0

            onWidthChanged: dropCanvas.requestPaint()
            onHeightChanged: dropCanvas.requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = StudioTheme.Values.themeInteraction
                ctx.lineWidth = 2
                ctx.setLineDash([4, 4])
                ctx.rect(5, 5, dropCanvas.width - 10, dropCanvas.height - 10)
                ctx.stroke()
            }
        }
    }

    MouseArea { // right clicking the empty area of the view
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (!assetsModel.isEmpty) {
                root.contextFilePath = ""
                root.contextDir = assetsModel.rootDir()
                root.isDirContextMenu = false
                contextMenu.popup()
            }
        }
    }

    // called from C++ to close context menu on focus out
    function handleViewFocusOut()
    {
        contextMenu.close()
        root.selectedAssets = {}
        root.selectedAssetsChanged()
    }

    StudioControls.Menu {
        id: contextMenu

        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        onOpened: {
            var numSelected = Object.values(root.selectedAssets).filter(p => p).length
            deleteFileItem.text = numSelected > 1 ? qsTr("Delete Files") : qsTr("Delete File")
        }

        StudioControls.MenuItem {
            text: qsTr("Expand All")
            enabled: root.allExpandedState !== 1
            visible: root.isDirContextMenu
            height: visible ? implicitHeight : 0
            onTriggered: assetsModel.toggleExpandAll(true)
        }

        StudioControls.MenuItem {
            text: qsTr("Collapse All")
            enabled: root.allExpandedState !== 2
            visible: root.isDirContextMenu
            height: visible ? implicitHeight : 0
            onTriggered: assetsModel.toggleExpandAll(false)
        }

        StudioControls.MenuSeparator {
            visible: root.isDirContextMenu
            height: visible ? StudioTheme.Values.border : 0
        }

        StudioControls.MenuItem {
            id: deleteFileItem
            text: qsTr("Delete File")
            visible: root.contextFilePath
            height: deleteFileItem.visible ? deleteFileItem.implicitHeight : 0
            onTriggered: {
                assetsModel.deleteFiles(Object.keys(root.selectedAssets).filter(p => root.selectedAssets[p]))
            }
        }

        StudioControls.MenuSeparator {
            visible: root.contextFilePath
            height: visible ? StudioTheme.Values.border : 0
        }

        StudioControls.MenuItem {
            text: qsTr("Rename Folder")
            visible: root.isDirContextMenu
            height: visible ? implicitHeight : 0
            onTriggered: renameFolderDialog.open()
        }

        StudioControls.MenuItem {
            text: qsTr("New Folder")
            onTriggered: newFolderDialog.open()
        }

        StudioControls.MenuItem {
            text: qsTr("Delete Folder")
            visible: root.isDirContextMenu
            height: visible ? implicitHeight : 0
            onTriggered: {
                var dirEmpty = !(root.contextDir.dirsModel && root.contextDir.dirsModel.rowCount() > 0)
                            && !(root.contextDir.filesModel && root.contextDir.filesModel.rowCount() > 0);

                if (dirEmpty)
                    assetsModel.deleteFolder(root.contextDir.dirPath)
                else
                    confirmDeleteFolderDialog.open()
            }
        }
    }

    RegExpValidator {
        id: folderNameValidator
        regExp: /^(\w[^*/><?\\|:]*)$/
    }

    Dialog {
        id: renameFolderDialog

        title: qsTr("Rename Folder")
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
                text: qsTr("Folder name cannot be empty.")
                color: "#ff0000"
                visible: folderRename.text === "" && !renameFolderDialog.renameError
            }

            Text {
                text: qsTr("Could not rename folder. Make sure no folder with the same name exists.")
                wrapMode: Text.WordWrap
                width: renameFolderDialog.width - 12
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
                        var success = assetsModel.renameFolder(root.contextDir.dirPath, folderRename.text)
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
            folderRename.text = root.contextDir.dirName
            folderRename.selectAll()
            folderRename.forceActiveFocus()
            renameFolderDialog.renameError = false
        }
    }

    Dialog {
        id: newFolderDialog

        title: qsTr("Create New Folder")
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        modal: true

        contentItem: Column {
            spacing: 2

            Row {
                Text {
                    text: qsTr("Folder name: ")
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
                text: qsTr("Folder name cannot be empty.")
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
                        assetsModel.addNewFolder(root.contextDir.dirPath + '/' + folderName.text)
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

        title: qsTr("Folder Not Empty")
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        implicitWidth: 300
        modal: true

        contentItem: Column {
            spacing: 20
            width: parent.width

            Text {
                id: folderNotEmpty

                text: qsTr("Folder \"%1\" is not empty. Delete it anyway?")
                            .arg(root.contextDir ? root.contextDir.dirName : "")
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
                        assetsModel.deleteFolder(root.contextDir.dirPath)
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

    Column {
        anchors.fill: parent
        anchors.topMargin: 5
        spacing: 5

        Row {
            id: searchRow

            width: parent.width

            SearchBox {
                id: searchBox

                width: parent.width - addAssetButton.width - 5
            }

            IconButton {
                id: addAssetButton
                anchors.verticalCenter: parent.verticalCenter
                tooltip: qsTr("Add a new asset to the project.")
                icon: StudioTheme.Constants.plus
                buttonSize: parent.height

                onClicked: rootView.handleAddAsset()
            }
        }

        Text {
            text: qsTr("No match found.")
            leftPadding: 10
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: 12
            visible: assetsModel.isEmpty && !searchBox.isEmpty()
        }


        Item { // placeholder when the assets library is empty
            width: parent.width
            height: parent.height - searchRow.height
            visible: assetsModel.isEmpty && searchBox.isEmpty()
            clip: true

            DropArea { // handles external drop (goes into default folder based on suffix)
                anchors.fill: parent

                onEntered: (drag)=> {
                    root.updateDropExtFiles(drag)
                }

                onDropped: {
                    rootView.handleExtFilesDrop(root.dropSimpleExtFiles, root.dropComplexExtFiles)
                }

                Column {
                    id: colNoAssets

                    spacing: 20
                    x: 20
                    width: root.width - 2 * x
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
            }
        }

        ScrollView { // TODO: experiment using ListView instead of ScrollView + Column
            id: assetsView
            width: parent.width
            height: parent.height - y
            clip: true
            interactive: assetsView.verticalScrollBarVisible && !contextMenu.opened

            Column {
                Repeater {
                    model: assetsModel // context property
                    delegate: dirSection
                }

                Component {
                    id: dirSection

                    Section {
                        id: section

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
                        visible: dirVisible
                        expandOnClick: false
                        useDefaulContextMenu: false
                        dropEnabled: true

                        onToggleExpand: {
                            dirExpanded = !dirExpanded
                        }

                        onDropEnter: (drag)=> {
                            root.updateDropExtFiles(drag)
                            section.highlight = drag.accepted && root.dropSimpleExtFiles.length > 0
                        }

                        onDropExit: {
                            section.highlight = false
                        }

                        onDrop: {
                            section.highlight = false
                            rootView.handleExtFilesDrop(root.dropSimpleExtFiles,
                                                        root.dropComplexExtFiles,
                                                        dirPath)
                        }

                        onShowContextMenu: {
                            root.contextFilePath = ""
                            root.contextDir = model
                            root.isDirContextMenu = true
                            root.allExpandedState = assetsModel.getAllExpandedState()
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
                                        root.contextFilePath = ""
                                        root.contextDir = model
                                        root.isDirContextMenu = true
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
                        color: root.selectedAssets[filePath]
                                            ? StudioTheme.Values.themeInteraction
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

                            property bool allowTooltip: true

                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            onExited: tooltipBackend.hideTooltip()
                            onEntered: allowTooltip = true
                            onCanceled: {
                                tooltipBackend.hideTooltip()
                                allowTooltip = true
                            }
                            onPositionChanged: tooltipBackend.reposition()
                            onPressed: (mouse)=> {
                                forceActiveFocus()
                                allowTooltip = false
                                tooltipBackend.hideTooltip()
                                var ctrlDown = mouse.modifiers & Qt.ControlModifier
                                if (mouse.button === Qt.LeftButton) {
                                    if (!root.selectedAssets[filePath] && !ctrlDown)
                                        root.selectedAssets = {}
                                    currFileSelected = ctrlDown ? !root.selectedAssets[filePath] : true
                                    root.selectedAssets[filePath] = currFileSelected
                                    root.selectedAssetsChanged()

                                    if (currFileSelected) {
                                        rootView.startDragAsset(
                                                   Object.keys(root.selectedAssets).filter(p => root.selectedAssets[p]),
                                                   mapToGlobal(mouse.x, mouse.y))
                                    }
                                } else {
                                    if (!root.selectedAssets[filePath] && !ctrlDown)
                                        root.selectedAssets = {}
                                    currFileSelected = root.selectedAssets[filePath] || !ctrlDown
                                    root.selectedAssets[filePath] = currFileSelected
                                    root.selectedAssetsChanged()

                                    root.contextFilePath = filePath
                                    root.contextDir = model.fileDir
                                    root.isDirContextMenu = false

                                    contextMenu.popup()
                                }
                            }

                            onReleased: (mouse)=> {
                                allowTooltip = true
                                if (mouse.button === Qt.LeftButton) {
                                    if (!(mouse.modifiers & Qt.ControlModifier))
                                        root.selectedAssets = {}
                                    root.selectedAssets[filePath] = currFileSelected
                                    root.selectedAssetsChanged()
                                }
                            }

                            ToolTip {
                                visible: !isFont && mouseArea.containsMouse && !contextMenu.visible
                                text: filePath
                                delay: 1000
                            }

                            Timer {
                                interval: 1000
                                running: mouseArea.containsMouse && mouseArea.allowTooltip
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
    }
}
