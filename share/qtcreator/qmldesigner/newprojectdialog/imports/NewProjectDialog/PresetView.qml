// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick.Window
import QtQuick.Controls

import QtQuick
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme

import BackendApi

ScrollView {
    id: scrollView

    required property Item loader
    required property string currentTabName

    property string backgroundHoverColor: DialogValues.presetItemBackgroundHover

    // selectLast: if true, it will select last item in the model after a model reset.
    property bool selectLast: false

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: scrollView
        x: scrollView.width + (DialogValues.gridMargins - verticalScrollBar.width) * 0.5
        y: scrollView.topPadding
        height: scrollView.availableHeight
        orientation: Qt.Vertical

        show: (scrollView.hovered || scrollView.focus || verticalScrollBar.inUse)
              && verticalScrollBar.isNeeded
    }

    contentWidth: gridView.contentItem.childrenRect.width
    contentHeight: gridView.contentItem.childrenRect.height

    GridView {
        id: gridView

        clip: true
        anchors.fill: parent
        cellWidth: DialogValues.gridCellWidth
        cellHeight: DialogValues.gridCellHeight
        rightMargin: -DialogValues.gridSpacing
        bottomMargin: -DialogValues.gridSpacing
        boundsBehavior: Flickable.StopAtBounds
        model: BackendApi.presetModel

        // called by onModelReset and when user clicks on an item, or when the header item is changed.
        onCurrentIndexChanged: {
            BackendApi.selectedPreset = gridView.currentIndex
            var source = BackendApi.currentPresetQmlPath()
            scrollView.loader.source = source
        }

        Connections {
            target: BackendApi.presetModel

            // called when data is set (setWizardFactories)
            function onModelReset() {
                if (scrollView.selectLast) {
                    gridView.currentIndex = BackendApi.presetModel.rowCount() - 1
                    scrollView.selectLast = false
                } else {
                    gridView.currentIndex = 0
                }

                // This will load Details.qml and Styles.qml by setting "source" on the Loader.
                gridView.currentIndexChanged()
            }
        }

        delegate: ItemDelegate {
            id: delegate

            property bool hover: delegate.hovered || removeMouseArea.containsMouse

            width: DialogValues.presetItemWidth
            height: DialogValues.presetItemHeight

            onClicked: delegate.GridView.view.currentIndex = index
            onDoubleClicked: {
                BackendApi.accept()
            }

            background: Rectangle {
                id: delegateBackground
                width: parent.width
                height: parent.height
                color: delegate.hover ? scrollView.backgroundHoverColor : "transparent"
                border.color: delegate.hover ? presetName.color : "transparent"
            }

            function fontIconCode(index) {
                var code = BackendApi.presetModel.fontIconCode(index)
                return code ? code : StudioTheme.Constants.wizardsUnknown
            }

            contentItem: Item {
                anchors.fill: parent

                ColumnLayout {
                    spacing: 0
                    anchors.top: parent.top
                    anchors.topMargin: -1
                    anchors.horizontalCenter: parent.horizontalCenter

                    Label {
                        id: presetIcon
                        text: delegate.fontIconCode(index)
                        color: DialogValues.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignBottom
                        renderType: Text.NativeRendering
                        font.pixelSize: 65
                        font.family: StudioTheme.Constants.iconFont.family
                        Layout.alignment: Qt.AlignHCenter
                    } // Preset type icon Label

                    Text {
                        id: presetName
                        color: DialogValues.textColor
                        text: name
                        font.pixelSize: DialogValues.defaultPixelSize
                        lineHeight: DialogValues.defaultLineHeight
                        lineHeightMode: Text.FixedHeight
                        width: DialogValues.presetItemWidth - 16
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignTop
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: presetName.width
                        Layout.minimumWidth: presetName.width
                        Layout.maximumWidth: presetName.width

                        ToolTip {
                            id: toolTip
                            y: -toolTip.height
                            visible: delegate.hovered && presetName.truncated
                            text: name
                            delay: 1000
                            height: 20

                            background: Rectangle {
                                color: StudioTheme.Values.themeToolTipBackground
                                border.color: StudioTheme.Values.themeToolTipOutline
                                border.width: StudioTheme.Values.border
                            }

                            contentItem: Text {
                                color: StudioTheme.Values.themeToolTipText
                                text: toolTip.text
                                font.pixelSize: DialogValues.defaultPixelSize
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    Text {
                        id: presetResolution
                        color: DialogValues.textColor
                        text: resolution
                        font.pixelSize: DialogValues.defaultPixelSize
                        lineHeight: DialogValues.defaultLineHeight
                        lineHeightMode: Text.FixedHeight
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignTop
                        Layout.alignment: Qt.AlignHCenter
                    }

                } // ColumnLayout

                Item {
                    id: removePresetButton
                    width: 20
                    height: 20
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 4
                    visible: isUserPreset === true
                             && delegate.hover
                             && scrollView.currentTabName !== BackendApi.recentsTabName()

                    Text {
                        anchors.fill: parent
                        text: StudioTheme.Constants.closeCross
                        color: DialogValues.textColor
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        font.family: StudioTheme.Constants.iconFont.family
                        font.pixelSize: StudioTheme.Values.myIconFontSize
                    }

                    MouseArea {
                        id: removeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: removeMouseArea.containsMouse ? Qt.PointingHandCursor
                                                                   : Qt.ArrowCursor

                        onClicked: {
                            delegate.GridView.view.currentIndex = index

                            removePresetDialog.presetName = presetName.text
                            removePresetDialog.open()
                        }
                    }
                } // Delete preset button Item
            } // Item

            states: [
                State {
                    name: "current"
                    when: delegate.GridView.isCurrentItem

                    PropertyChanges {
                        target: presetName
                        color: DialogValues.textColorInteraction
                    }
                    PropertyChanges {
                        target: presetResolution
                        color: DialogValues.textColorInteraction
                    }
                    PropertyChanges {
                        target: presetIcon
                        color: DialogValues.textColorInteraction
                    }
                    PropertyChanges {
                        target: scrollView
                        backgroundHoverColor: DialogValues.presetItemBackgroundHoverInteraction
                    }
                } // State
            ]
        } // ItemDelegate

        PopupDialog {
            id: removePresetDialog

            property string presetName

            title: qsTr("Delete Custom Preset")
            standardButtons: Dialog.Yes | Dialog.No
            modal: true
            closePolicy: Popup.CloseOnEscape
            anchors.centerIn: parent
            width: DialogValues.popupDialogWidth

            onAccepted: BackendApi.removeCurrentPreset()

            Text {
                text: qsTr("Are you sure you want to delete \"" + removePresetDialog.presetName + "\" ?")
                color: DialogValues.textColor
                font.pixelSize: DialogValues.defaultPixelSize
                wrapMode: Text.WordWrap

                width: DialogValues.popupDialogWidth - 2 * DialogValues.popupDialogPadding
            }

        } // Dialog
    } // GridView
} // ScrollView
