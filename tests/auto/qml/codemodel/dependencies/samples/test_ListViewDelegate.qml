// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.2
import QtQuick 2.4

Rectangle {
    width: 200; height: 200

    ListModel {
        id: fruitModel
        ListElement {
            name: "Apple"
            cost: 2.45
        }
        ListElement {
            name: "Orange"
            cost: 3.25
        }
        ListElement {
            name: "Banana"
            cost: 1.95
        }
    }

    Component {
        id: fruitDelegate
        Row {
            spacing: 10
            Text { text: name }
            Text { text: '$' + cost }
        }
    }

    ListView {
        anchors.fill: parent
        model: fruitModel
        delegate: fruitDelegate
    }
}
