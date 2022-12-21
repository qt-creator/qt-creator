// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 2.3
import QtQml.Models 2.2

Rectangle {
    width: 220
    height: 350
    color: "#494d53"

    ListModel {
        id: listModel

        ListElement {
            foo: "A"
            bar: "B"
            baz: "C"
        }
        ListElement {
            foo: "A"
            bar: "B"
            baz: "C"
        }
        ListElement {
            foo: "A"
            bar: "B"
            baz: "C"
        }
    }

    Tumbler {
        anchors.centerIn: parent
        model: listModel
    }
}
