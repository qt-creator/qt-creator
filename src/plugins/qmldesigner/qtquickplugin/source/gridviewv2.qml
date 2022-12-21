// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

GridView {
    width: 140
    height: 140
    cellWidth: 70
    cellHeight: 70

    model: ListModel {

        ListElement {
            name: "Grey"
            colorCode: "grey"

        }

        ListElement {
            name: "Red"
            colorCode: "red"

        }

        ListElement {
            name: "Blue"
            colorCode: "blue"

        }

        ListElement {
            name: "Green"
            colorCode: "green"

        }
    }

    delegate:  Item {
        height: 50
        x: 5

        Column {
            spacing: 5
            Rectangle {
                width: 40
                height: 40
                color: colorCode
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                x: 5
                text: name
                anchors.horizontalCenter: parent.horizontalCenter
                font.bold: true
            }

        }
    }
}
