// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as Controls
import welcome 1.0

Controls.ScrollBar {
    id: scrollBar

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: active ? 1 : 2
    visible: orientation === Qt.Horizontal ? contentWidth > width : contentHeight > height
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    contentItem: Rectangle {
        implicitWidth: 13
        implicitHeight: 13
        color: active ? Constants.textHoverColor : Constants.textDefaultColor
    }

    background: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        color: "#3b3c3d"
        visible: active
    }
}
