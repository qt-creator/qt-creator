// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AiAssistantBackend

StudioControls.PopupPanel {
    id: root

    property size imageSize: Qt.size(80, 60)
    property alias model: repeater.model

    signal imageClicked(imageSource : string)
    signal windowShown()

    preferredCorner: Qt.AlignTop | Qt.AlignLeft
    horizontalDirection: Qt.RightToLeft
    verticalDirection: StudioControls.PopupPanel.TopToBottom

    implicitHeight: scrollView.heightNeeded
    implicitWidth: scrollView.widthNeeded

    minimumWidth: 200
    minimumHeight: 50

    maximumWidth: 600

    onClosing: {
        root.model = {}
    }

    function showWindow() {
        root.model = AiAssistantBackend.rootView.getImageAssetsPaths()
        root.showPanel()
        root.windowShown()
    }

    HelperWidgets.ScrollView {
        id: scrollView

        readonly property int padding: 1
        readonly property int extraWidth: 2 * scrollView.padding + StudioTheme.Values.scrollBarThickness
        readonly property int extraHeight: 2 * scrollView.padding
        readonly property int maxContentSize: root.availableSize.width - scrollView.extraWidth
        readonly property int widthNeeded: scrollView.visibleContentItem.implicitWidth + scrollView.extraWidth
        readonly property int heightNeeded: scrollView.contentHeight + scrollView.extraHeight
        readonly property Item visibleContentItem: gridLayout.visible ? gridLayout : emptyAssetsText

        anchors.fill: parent
        anchors.margins: scrollView.padding
        clip: true

        GridLayout {
            id: gridLayout

            readonly property int implicitColumns: scrollView.maxContentSize / (root.imageSize.width + gridLayout.columnSpacing)
            readonly property int horizontalSpacingSum: (gridLayout.columns > 1)
                                                        ? (gridLayout.columns - 1) * gridLayout.columnSpacing
                                                        : 0
            columns: Math.min(gridLayout.implicitColumns, repeater.count)

            columnSpacing: 5
            rowSpacing: 5
            visible: repeater.count > 0

            Repeater {
                id: repeater

                delegate: AssetImage {
                    id: imageItem

                    required property string modelData

                    width: root.imageSize.width
                    height: root.imageSize.height
                    source: imageItem.modelData

                    onClicked: {
                        root.imageClicked(imageItem.source)
                        root.close();
                    }
                }
            }
        }

        Text {
            id: emptyAssetsText

            visible: !gridLayout.visible
            text: qsTr("No image assets available.")
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.mediumFontSize
            padding: 5
        }
    }
}
