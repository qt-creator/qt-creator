/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.2

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
            implicitWidth: 30

            iconSource: "qrc:/qmlprofiler/ico_prev.png"
            tooltip: qsTr("Jump to previous event.")
            onClicked: buttons.jumpToPrev()
        }

        ToolButton {
            id: jumpToNextButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: 30

            iconSource: "qrc:/qmlprofiler/ico_next.png"
            tooltip: qsTr("Jump to next event.")
            onClicked: buttons.jumpToNext()
        }

        ToolButton {
            id: zoomControlButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: 30

            iconSource: "qrc:/qmlprofiler/ico_zoom.png"
            tooltip: qsTr("Show zoom slider.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.zoomControlChanged()
        }

        ToolButton {
            id: rangeButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: 30

            iconSource: checked ? "qrc:/qmlprofiler/ico_rangeselected.png" :
                                  "qrc:/qmlprofiler/ico_rangeselection.png"
            tooltip: qsTr("Select range.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.rangeSelectChanged()
        }

        ToolButton {
            id: lockButton
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            implicitWidth: 30

            iconSource: "qrc:/qmlprofiler/ico_selectionmode.png"
            tooltip: qsTr("View event information on mouseover.")
            checkable: true
            checked: false
            onCheckedChanged: buttons.lockChanged()
        }
    }
}
