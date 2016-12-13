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

    style: ToolBarStyle {
        padding {
            left: 0
            right: 0
            top: 0
            bottom: 0
        }
        background: Rectangle {
            anchors.fill: parent
            color: Theme.color(Theme.PanelStatusBarBackgroundColor)
        }
    }

    RowLayout {
        spacing: 0
        anchors.fill: parent

        ImageToolButton {
            id: jumpToPrevButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            imageSource: "image://icons/prev"
            tooltip: qsTr("Jump to previous event.")
            onClicked: buttons.jumpToPrev()
        }

        ImageToolButton {
            id: jumpToNextButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            imageSource: "image://icons/next"
            tooltip: qsTr("Jump to next event.")
            onClicked: buttons.jumpToNext()
        }

        ImageToolButton {
            id: zoomControlButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            imageSource: "image://icons/zoom"
            tooltip: qsTr("Show zoom slider.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.zoomControlChanged()
        }

        ImageToolButton {
            id: rangeButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            imageSource: "image://icons/" + (checked ? "rangeselected" : "rangeselection");
            tooltip: qsTr("Select range.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.rangeSelectChanged()
        }

        ImageToolButton {
            id: lockButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            imageSource: "image://icons/selectionmode"
            tooltip: qsTr("View event information on mouseover.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.lockChanged()
        }
    }
}
