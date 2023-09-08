// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    required property var model

    property alias leftPadding: topRow.leftPadding
    property real rightPadding: topRow.rightPadding

    color: StudioTheme.Values.themeBackgroundColorAlternate

    Column {
        id: topRow

        spacing: 0
        width: parent.width
        leftPadding: 20
        rightPadding: 0
        topPadding: 5

        Text {
            id: collectionNameText

            leftPadding: 8
            rightPadding: 8
            topPadding: 3
            bottomPadding: 3

            color: StudioTheme.Values.themeTextColor
            text: root.model.collectionName
            font.pixelSize: StudioTheme.Values.mediumIconFont
            elide: Text.ElideRight

            Rectangle {
                anchors.fill: parent
                z: parent.z - 1
                color: StudioTheme.Values.themeBackgroundColorNormal
            }
        }

        Item { // spacer
            width: 1
            height: 10
        }

        HorizontalHeaderView {
            id: headerView

            property real topPadding: 5
            property real bottomPadding: 5

            height: headerMetrics.height + topPadding + bottomPadding

            syncView: tableView
            clip: true

            delegate: Rectangle {
                implicitWidth: 100
                implicitHeight: headerText.height
                color: StudioTheme.Values.themeControlBackground
                border.width: 2
                border.color: StudioTheme.Values.themeControlOutline
                clip: true

                Text {
                    id: headerText

                    topPadding: headerView.topPadding
                    bottomPadding: headerView.bottomPadding
                    leftPadding: 5
                    rightPadding: 5
                    text: display
                    font.pixelSize: headerMetrics.font
                    color: StudioTheme.Values.themeIdleGreen
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.centerIn: parent
                    elide: Text.ElideRight
                }
            }

            TextMetrics {
                id: headerMetrics

                font.pixelSize: StudioTheme.Values.baseFontSize
                text: "Xq"
            }
        }
    }

    TableView {
        id: tableView

        anchors {
            top: topRow.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: root.leftPadding
            rightMargin: root.rightPadding
        }

        model: root.model

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: itemText.height
            color: StudioTheme.Values.themeControlBackground
            border.width: 1
            border.color: StudioTheme.Values.themeControlOutline

            Text {
                id: itemText

                text: display
                width: parent.width
                leftPadding: 5
                topPadding: 3
                bottomPadding: 3
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: StudioTheme.Values.themeTextColor
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }
}
