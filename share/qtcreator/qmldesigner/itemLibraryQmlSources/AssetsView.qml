/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

ScrollView { // TODO: experiment using ListView instead of ScrollView + Column
    id: assetsView
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
                        fillMode: Image.PreserveAspectFit
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
