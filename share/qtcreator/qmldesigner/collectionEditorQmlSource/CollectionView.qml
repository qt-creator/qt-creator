// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import CollectionEditorBackend

Item {
    id: root
    focus: true

    property var rootView: CollectionEditorBackend.rootView
    property var model: CollectionEditorBackend.model
    property var collectionDetailsModel: CollectionEditorBackend.collectionDetailsModel
    property var collectionDetailsSortFilterModel: CollectionEditorBackend.collectionDetailsSortFilterModel

    function showWarning(title, message) {
        warningDialog.title = title
        warningDialog.message = message
        warningDialog.open()
    }

    ImportDialog {
        id: importDialog

        backendValue: root.rootView
        anchors.centerIn: parent
    }

    NewCollectionDialog {
        id: newCollection

        backendValue: root.rootView
        sourceModel: root.model
        anchors.centerIn: parent
    }

    Message {
        id: warningDialog

        title: ""
        message: ""
    }

    Rectangle {
        // Covers the toolbar color on top to prevent the background
        // color for the margin of splitter

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: topToolbar.height
        color: topToolbar.color
    }

    SplitView {
        id: splitView

        readonly property bool isHorizontal: splitView.orientation === Qt.Horizontal

        orientation: width >= 500 ? Qt.Horizontal : Qt.Vertical
        anchors.fill: parent

        handle: Item {
            id: handleDelegate

            property color color: SplitHandle.pressed ? StudioTheme.Values.themeControlOutlineInteraction
                                                      : SplitHandle.hovered ? StudioTheme.Values.themeControlOutlineHover
                                                                            : StudioTheme.Values.themeControlOutline

            implicitWidth: 1
            implicitHeight: 1

            Rectangle {
                id: handleRect

                property real verticalMargin: splitView.isHorizontal ? StudioTheme.Values.splitterMargin : 0
                property real horizontalMargin: splitView.isHorizontal ? 0 : StudioTheme.Values.splitterMargin

                anchors.fill: parent
                anchors.topMargin: handleRect.verticalMargin
                anchors.bottomMargin: handleRect.verticalMargin
                anchors.leftMargin: handleRect.horizontalMargin
                anchors.rightMargin: handleRect.horizontalMargin

                color: handleDelegate.color
            }

            containmentMask: Item {
                x: splitView.isHorizontal ? ((handleDelegate.width - width) / 2) : 0
                y: splitView.isHorizontal ? 0 : ((handleDelegate.height - height) / 2)
                height: splitView.isHorizontal ? handleDelegate.height : StudioTheme.Values.borderHover
                width: splitView.isHorizontal ? StudioTheme.Values.borderHover : handleDelegate.width
            }
        }

        ColumnLayout {
            id: collectionsSideBar
            spacing: 0

            SplitView.minimumWidth: 200
            SplitView.maximumWidth: 450
            SplitView.minimumHeight: 200
            SplitView.maximumHeight: 400
            SplitView.fillWidth: !splitView.isHorizontal
            SplitView.fillHeight: splitView.isHorizontal

            Rectangle {
                id: topToolbar
                color: StudioTheme.Values.themeToolbarBackground

                Layout.preferredHeight: StudioTheme.Values.toolbarHeight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin

                    text: qsTr("Data Models")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    color: StudioTheme.Values.themeTextColor
                }

                HelperWidgets.AbstractButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin

                    style: StudioTheme.Values.viewBarButtonStyle
                    buttonIcon: StudioTheme.Constants.import_medium
                    tooltip: qsTr("Import a model")

                    onClicked: importDialog.open()
                }
            }

            Item { // Model Groups
                Layout.fillWidth: true
                Layout.minimumHeight: bottomSpacer.isExpanded ? 150 : 0
                Layout.fillHeight: !bottomSpacer.isExpanded
                Layout.preferredHeight: sourceListView.contentHeight
                Layout.maximumHeight: sourceListView.contentHeight
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onClicked: (event) => {
                        root.model.deselect()
                        event.accepted = true
                    }
                }

                ListView {
                    id: sourceListView

                    anchors.fill: parent
                    model: root.model

                    delegate: ModelSourceItem {
                        implicitWidth: sourceListView.width
                        hasSelectedTarget: root.rootView.targetNodeSelected
                    }
                }
            }

            HelperWidgets.IconButton {
                id: addCollectionButton

                iconSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 24
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                tooltip: qsTr("Add a new model")
                icon: StudioTheme.Constants.create_medium
                onClicked: newCollection.open()
            }

            Item {
                id: bottomSpacer

                readonly property bool isExpanded: height > 0
                Layout.minimumWidth: 1
                Layout.fillHeight: true
            }
        }

        CollectionDetailsView {
            model: root.collectionDetailsModel
            backend: root.model
            sortedModel: root.collectionDetailsSortFilterModel
            SplitView.fillHeight: true
            SplitView.fillWidth: true
        }
    }
}
