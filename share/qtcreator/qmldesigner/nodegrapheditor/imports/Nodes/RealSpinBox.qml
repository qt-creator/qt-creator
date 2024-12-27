// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import StudioControls as StudioControls
import NodeGraphEditorBackend

Base {
    id: root

    readonly property QtObject value: QtObject {
        property real floating

        onFloatingChanged: {
            realSpinBox.realValue = floating;
        }
    }

    Layout.preferredWidth: 175
    type: "RealSpinBox"

    portsMetaData: QtObject {
        property var pin: []
        property var pout: [
            {
                id: "realspinbox_out",
                alias: "floating",
                name: "OUT",
                type: "real"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "RealSpinBox (real)";
    }

    StudioControls.RealSpinBox {
        id: realSpinBox

        actionIndicatorVisible: false
        anchors.centerIn: parent
        decimals: 2
        realValue: root.value.floating

        onRealValueChanged: {
            NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
            root.value.floating = realValue;
        }
    }
}
