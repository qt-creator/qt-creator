// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import StudioControls as StudioControls

Base {
    id: root

    readonly property QtObject value: QtObject {
        property bool binary: checkBox.checked
    }

    portsMetaData: QtObject {
        property var pin: []
        property var pout: [
            {
                id: "checkbox_out",
                alias: "binary",
                name: "OUT",
                type: "bool"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "CheckBox (bool)";
    }

    StudioControls.CheckBox {
        id: checkBox

        actionIndicatorVisible: false
        anchors.centerIn: parent
    }
}
