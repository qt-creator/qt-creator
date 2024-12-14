// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import NodeGraphEditorBackend

Base {
    id: root

    property QtObject value: QtObject {
        property url text: `image://qmldesigner_nodegrapheditor/${comboBox.currentValue}`

        onTextChanged: {
            NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
        }
    }

    Layout.preferredWidth: 175
    type: "ComboBox"

    portsMetaData: QtObject {
        property var pin: []
        property var pout: [
            {
                id: "combobox_out",
                alias: "text",
                name: "OUT",
                type: "QUrl"
            },
        ]
    }

    Component.onCompleted: {
        node.label = "ComboBox";
    }

    HelperWidgets.FileResourcesModel {
        id: fileModel

        filter: "*.png *.gif *.jpg *.bmp *.jpeg *.svg *.pbm *.pgm *.ppm *.xbm *.xpm *.hdr *.ktx *.webp"
        modelNodeBackendProperty: modelNodeBackend
    }

    StudioControls.ComboBox {
        id: comboBox

        actionIndicatorVisible: false
        anchors.centerIn: parent
        model: fileModel.model
        textRole: "fileName"
        valueRole: "relativeFilePath"

        // valueRole: "absoluteFilePath"

        // model: [
        //     {
        //         text: "#1 (ONE)",
        //         value: "one"
        //     },
        //     {
        //         text: "#2 (TWO)",
        //         value: "two"
        //     },
        //     {
        //         text: "#3 (THREE)",
        //         value: "three"
        //     }
        // ]
        // textRole: "text"
        // valueRole: "value"

        // onActivated: () => {
        //     // root.value.text = comboBox.currentValue;
        //     root.value.text = `image://qmldesigner_nodegrapheditor/${comboBox.currentValue}`;
        // }
    }
}
