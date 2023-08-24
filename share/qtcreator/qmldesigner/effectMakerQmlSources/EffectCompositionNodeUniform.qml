// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Item {
    id: root

    height: layout.implicitHeight

    Component.onCompleted: {
        if (uniformType === "vec2")
            valueLoader.source = "ValueVec2.qml"
        else if (uniformType === "vec3")
            valueLoader.source = "ValueVec3.qml"
        else if (uniformType === "vec4")
            valueLoader.source = "ValueVec4.qml"
        else if (uniformType === "bool")
            valueLoader.source = "ValueBool.qml"
        else if (uniformType === "color")
            valueLoader.source = "ValueColor.qml"
//        else if (uniformType === "image") // TODO
//            valueLoader.source = valueImage
        else
            valueLoader.source = "ValueFloat.qml"
    }

    RowLayout {
        id: layout

        spacing: 20
        anchors.fill: parent

        Text {
            text: uniformName
            color: StudioTheme.Values.themeTextColor
            font.pointSize: StudioTheme.Values.smallFontSize
            horizontalAlignment: Text.AlignRight
            Layout.maximumWidth: 140
            Layout.minimumWidth: 140
            Layout.preferredWidth: 140
        }

        Loader {
            id: valueLoader
            Layout.fillWidth: true
        }
    }
}
