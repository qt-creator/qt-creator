// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

ListView {
    width: 160
    height: 80
    model:  ListModel {
        ListElement {
            name: "Red"
            colorCode: "red"
        }
        ListElement {
            name: "Green"
            colorCode: "green"
        }
        ListElement {
            name: "Blue"
            colorCode: "blue"
        }
        ListElement {
            name: "White"
            colorCode: "white"
        }
    }

    delegate: Row {
        spacing: 5
        Rectangle {
            width: 100
            height: 20
            color: colorCode
        }

        Text {
            width: 100
            text: name
        }
    }
}
