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
    property string delFilePath: ""

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

    // called from C++ to close context menu on focus out
    function handleViewFocusOut()
    {
        contextMenu.close()
        selectedAssets = {}
        selectedAssetsChanged()
    }

    ScrollView { // TODO: experiment using ListView instead of ScrollView + Column
        id: assetsView
        anchors.fill: parent

        Item {
            StudioControls.Menu {
                id: contextMenu

                StudioControls.MenuItem {
                    text: qsTr("Expand All")
                    enabled: allExpandedState !== 1
                    visible: !delFilePath
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.toggleExpandAll(true)
                }

                StudioControls.MenuItem {
                    text: qsTr("Collapse All")
                    enabled: allExpandedState !== 2
                    visible: !delFilePath
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.toggleExpandAll(false)
                }

                StudioControls.MenuItem {
                    text: qsTr("Delete File")
                    visible: delFilePath
                    height: visible ? implicitHeight : 0
                    onTriggered: assetsModel.removeFile(delFilePath)
                }
            }
        }

        Column {
            spacing: 2
            Repeater {
                model: assetsModel // context property
                delegate: dirSection
            }

            Component {
                id: dirSection

                Section {
                    width: assetsView.width -
                           (assetsView.verticalScrollBarVisible ? assetsView.verticalThickness : 0)
                    caption: dirName
                    sectionHeight: 30
                    sectionFontSize: 15
                    levelShift: 20
                    leftPadding: 0
                    hideHeader: dirDepth === 0
                    showLeftBorder: true
                    expanded: dirExpanded
                    visible: dirVisible
                    expandOnClick: false
                    useDefaulContextMenu: false

                    onToggleExpand: {
                        dirExpanded = !dirExpanded
                    }
                    onShowContextMenu: {
                        delFilePath = ""
                        allExpandedState = assetsModel.getAllExpandedState()
                        contextMenu.popup()
                    }

                    Column {
                        spacing: 5
                        leftPadding: 15

                        Repeater {
                            model: dirsModel
                            delegate: dirSection
                        }

                        Repeater {
                            model: filesModel
                            delegate: fileSection
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
                                delFilePath = filePath
                                tooltipBackend.hideTooltip()
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
        visible: assetsModel.isEmpty

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
}
