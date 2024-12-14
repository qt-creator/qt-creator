// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick3D as QtQuick3D

import QuickQanava as Qan

import NodeGraphEditorBackend
import NodeGraphEditorBackend

Base {
    id: root

    readonly property QtQuick3D.Texture reset: QtQuick3D.Texture {
    }
    readonly property QtQuick3D.Texture value: QtQuick3D.Texture {
    }

    Layout.preferredHeight: 150
    Layout.preferredWidth: 150
    type: "Texture"

    Component.onCompleted: {
        node.label = "Texture";
    }
    onValueChanged: {
        NodeGraphEditorBackend.nodeGraphEditorModel.hasUnsavedChanges = true;
    }

    Image {
        anchors.centerIn: parent
        height: 96
        source: root.value.source
        width: 96
    }
}
