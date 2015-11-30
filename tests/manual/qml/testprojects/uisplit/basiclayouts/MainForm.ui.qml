/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0

ColumnLayout {
    property alias rowBox_1: rowBox_1
    property alias rowLayout_1: rowLayout_1
    property alias textField_1: textField_1
    property alias button_1: button_1
    property alias gridBox_1: gridBox_1
    property alias gridLayout_1: gridLayout_1
    property alias label_1: label_1
    property alias label_2: label_2
    property alias label_3: label_3
    property alias textField_2: textField_2
    property alias textField_3: textField_3
    property alias textField_4: textField_4
    property alias textField_5: textField_5
    property alias textArea_1: textArea_1

    anchors.fill: parent

    GroupBox {
        id: rowBox_1
        title: "Row layout"
        Layout.fillWidth: true

        RowLayout {
            id: rowLayout_1
            anchors.fill: parent
            TextField {
                id: textField_1
                placeholderText: "This wants to grow horizontally"
                Layout.fillWidth: true
            }
            Button {
                id: button_1
                text: "Button"
            }
        }
    }

    GroupBox {
        id: gridBox_1
        title: "Grid layout"
        Layout.fillWidth: true

        GridLayout {
            id: gridLayout_1
            rows: 3
            flow: GridLayout.TopToBottom
            anchors.fill: parent

            Label { id: label_1; text: "Line 1" }
            Label { id: label_2; text: "Line 2" }
            Label { id: label_3; text: "Line 3" }

            TextField { id: textField_2 }
            TextField { id: textField_3 }
            TextField { id: textField_4 }

            TextArea {
                id: textField_5
                text: "This widget spans over three rows in the GridLayout.\n"
                      + "All items in the GridLayout are implicitly positioned from top to bottom."
                Layout.rowSpan: 3
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
    TextArea {
        id: textArea_1
        text: "This fills the whole cell"
        Layout.minimumHeight: 30
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
