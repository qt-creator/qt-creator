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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import TimelineTheme 1.0

ToolBar {
    id: buttons

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

    background: Rectangle {
        anchors.fill: parent
        color: Theme.color(Theme.PanelStatusBarBackgroundColor)
    }


    RowLayout {
        spacing: 0
        anchors.fill: parent

        ImageToolButton {
            id: jumpToPrevButton
            Layout.fillHeight: true

            imageSource: "image://icons/prev"
            ToolTip.text: qsTr("Jump to previous event.")
            onClicked: buttons.jumpToPrev()
        }

        ImageToolButton {
            id: jumpToNextButton
            Layout.fillHeight: true

            imageSource: "image://icons/next"
            ToolTip.text: qsTr("Jump to next event.")
            onClicked: buttons.jumpToNext()
        }

        ImageToolButton {
            id: zoomControlButton
            Layout.fillHeight: true

            imageSource: "image://icons/zoom"
            ToolTip.text: qsTr("Show zoom slider.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.zoomControlChanged()
        }

        ImageToolButton {
            id: rangeButton
            Layout.fillHeight: true

            imageSource: "image://icons/" + (checked ? "rangeselected" : "rangeselection");
            ToolTip.text: qsTr("Select range.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.rangeSelectChanged()
        }

        ImageToolButton {
            id: lockButton
            Layout.fillHeight: true

            imageSource: "image://icons/selectionmode"
            ToolTip.text: qsTr("View event information on mouseover.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.lockChanged()
        }
    }
}
