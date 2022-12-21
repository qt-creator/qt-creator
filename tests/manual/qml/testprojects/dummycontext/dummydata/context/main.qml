// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4
import QmlDesigner 1.0

/*This file describes the dummy context for the original main.qml*/

DummyContextObject {
    parent: Item {
        width: 640
        height: 300
    }

    property Item root: Rectangle {
        color: "red"
    }
}
