/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.2

ToolBar {
    id: buttons
    readonly property int buttonWidth: 30

    signal jumpToPrev()
    signal jumpToNext()
    signal zoomControlChanged()
    signal rangeSelectChanged()
    signal lockChanged()

    function updateLockButton(locked) {
        lockButton.checked = !locked;
    }

    function lockButtonChecked() {
        return lockButton.checked;
    }

    function updateRangeButton(rangeMode) {
        rangeButton.checked = rangeMode;
    }

    function rangeButtonChecked() {
        return rangeButton.checked
    }

    style: ToolBarStyle {
        padding {
            left: 0
            right: 0
            top: 0
            bottom: 0
        }
        background: Rectangle {
            anchors.fill: parent
            color: "#9B9B9B"
        }
    }

    RowLayout {
        spacing: 0
        anchors.fill: parent

        ToolButton {
            id: jumpToPrevButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: buttonWidth

            iconSource: "qrc:/timeline/ico_prev.png"
            tooltip: qsTr("Jump to previous event.")
            onClicked: buttons.jumpToPrev()
        }

        ToolButton {
            id: jumpToNextButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: buttonWidth

            iconSource: "qrc:/timeline/ico_next.png"
            tooltip: qsTr("Jump to next event.")
            onClicked: buttons.jumpToNext()
        }

        ToolButton {
            id: zoomControlButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: buttonWidth

            iconSource: "qrc:/timeline/ico_zoom.png"
            tooltip: qsTr("Show zoom slider.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.zoomControlChanged()
        }

        ToolButton {
            id: rangeButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: buttonWidth

            iconSource: checked ? "qrc:/timeline/ico_rangeselected.png" :
                                  "qrc:/timeline/ico_rangeselection.png"
            tooltip: qsTr("Select range.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.rangeSelectChanged()
        }

        ToolButton {
            id: lockButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: buttonWidth

            iconSource: "qrc:/timeline/ico_selectionmode.png"
            tooltip: qsTr("View event information on mouseover.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.lockChanged()
        }
    }
}
