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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0

Item {
    id:root

    width: 600
    height: 300

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 4
        GroupBox {
            id: rowBox
            title: "Row layout"
            Layout.fillWidth: true
            RowLayout {
                id: rowLayout
                anchors.fill: parent
                TextField {
                    placeholderText: "This wants to grow horizontally"
                    Layout.fillWidth: true
                }
                Button {
                    text: "Button"
                }
            }
        }

        GroupBox {
            id: gridBox
            title: "Grid layout"
            Layout.fillWidth: true

            GridLayout {
                id: gridLayout
                anchors.fill: parent
                rows: 3
                flow: GridLayout.TopToBottom

                Label { text: "Line 1" }
                Label { text: "Line 2" }
                Label { text: "Line 3" }

                TextField { }
                TextField { }
                TextField { }

                TextArea {
                    text: "This widget spans over three rows in the GridLayout.\n"
                          + "All items in the GridLayout are implicitly positioned from top to bottom."
                    Layout.rowSpan: 3
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
        TextArea {
            id: t3
            text: "This fills the whole cell"
            Layout.minimumHeight: 30
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
