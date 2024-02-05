// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Item {
    id: root

    height: layout.implicitHeight

    visible: !uniformUseCustomValue

    Component.onCompleted: {
        if (uniformType === "int") {
            valueLoader.source = "ValueInt.qml"
        } else if (uniformType === "vec2") {
            valueLoader.source = "ValueVec2.qml"
        } else if (uniformType === "vec3") {
            valueLoader.source = "ValueVec3.qml"
        } else if (uniformType === "vec4") {
            valueLoader.source = "ValueVec4.qml"
        } else if (uniformType === "bool") {
            valueLoader.source = "ValueBool.qml"
        } else if (uniformType === "color") {
            valueLoader.source = "ValueColor.qml"
        } else if (uniformType === "sampler2D") {
            valueLoader.source = "ValueImage.qml"
        } else if (uniformType === "define") {
            if (uniformControlType === "int")
                valueLoader.source = "ValueInt.qml"
            else if (uniformControlType === "bool")
                valueLoader.source = "ValueBool.qml"
            else
                valueLoader.source = "ValueDefine.qml"
        } else {
            valueLoader.source = "ValueFloat.qml"
        }
    }

    RowLayout {
        id: layout

        spacing: 20
        anchors.fill: parent

        Text {
            text: uniformDisplayName
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            horizontalAlignment: Text.AlignRight
            Layout.maximumWidth: 140
            Layout.minimumWidth: 140
            Layout.preferredWidth: 140
            elide: Text.ElideRight

            HelperWidgets.ToolTipArea {
                anchors.fill: parent
                tooltip: uniformDescription
            }
        }

        Loader {
            id: valueLoader
            Layout.fillWidth: true
        }
    }
}
