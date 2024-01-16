// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme

Item {
    id: buttonRowButton

    property int leftPadding: 0
    property bool checked: false
    property bool roundLeftButton: true
    property alias iconSource: image.source
    property alias tooltip: toolTipArea.tooltip

    signal clicked()
    signal doubleClicked()

    width: StudioTheme.Values.height + leftPadding
    height: StudioTheme.Values.height

    function index() {
        for (var i = 0; i < parent.children.length; i++) {
            if (parent.children[i] === buttonRowButton)
                return i
        }
        return -1
    }

    function isFirst() {
        return index() === 0
    }

    function isLast() {
        return index() === (parent.children.length - 1)
    }

    Item {
        anchors.fill: parent

        RoundedPanel {
            roundLeft: isFirst() && buttonRowButton.roundLeftButton
            anchors.fill: parent
            visible: checked
            color: StudioTheme.Values.themeControlBackgroundInteraction
        }

        RoundedPanel {
            roundLeft: isFirst()
            anchors.fill: parent
            visible: !checked
            color: StudioTheme.Values.themeControlBackground
        }
    }

    Image {
        id: image
        width: 16
        height: 16
        smooth: false
        anchors.centerIn: parent
    }

    ToolTipArea {
        anchors.fill: parent
        id: toolTipArea
        anchors.leftMargin: leftPadding
        onClicked: {
            if (buttonRowButton.checked) {
                buttonRowButton.parent.__unCheckButton(index())
            } else {
                buttonRowButton.parent.__checkButton(index())
            }
            buttonRowButton.clicked()
        }
        onDoubleClicked: buttonRowButton.doubleClicked()
    }
}
