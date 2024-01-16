// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme

Rectangle {
    id: panel

    property bool roundLeft: true
    property bool roundRight: true


    /*
    radius: roundLeft || roundRight ? 1 : 0
    gradient: Gradient {
        GradientStop {color: '#555' ; position: 0}
        GradientStop {color: '#444' ; position: 1}
    }
    */

    border.width: roundLeft || roundRight ? 1 : 0
    color: StudioTheme.Values.themeControlBackground
    border.color: StudioTheme.Values.themeControlOutline

    Rectangle {
        anchors.fill: parent
        visible: roundLeft && !roundRight
        anchors.leftMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        color: panel.color
        Component.onCompleted: {
            //gradient = parent.gradient
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: roundRight && !roundLeft
        anchors.rightMargin: 10
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        color: panel.color
        Component.onCompleted: {
            //gradient = parent.gradient
        }
    }

    Rectangle {
        color: StudioTheme.Values.themeControlOutline
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 3 : 0
        anchors.rightMargin: roundRight ? 3 : 0
    }

    Rectangle {
        color: StudioTheme.Values.themeControlOutline
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        anchors.leftMargin: roundLeft ? 2 : 0
        anchors.rightMargin: roundRight ? 2 : 0
    }

}
