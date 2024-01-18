// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import WelcomeScreen 1.0
import DataModels 1.0

Item {
    id: gridContainer
    width: 1460
    height: 760

    property alias hover: scrollView.outsideHover
    property alias model: gridView.model
    property alias delegate: gridView.delegate

    Connections {
        target: gridContainer
        onWidthChanged: Constants.responsiveResize(gridContainer.width)
    }

    CustomScrollView {
        id: scrollView
        anchors.fill: parent

        GridView {
            id: gridView
            clip: true
            anchors.fill: parent
            rightMargin: -Constants.gridSpacing
            bottomMargin: -Constants.gridSpacing
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: Constants.gridCellSize
            cellHeight: Constants.gridCellSize

            model: ExamplesModel {}
            delegate: ThumbnailDelegate {}
        }
    }
}
