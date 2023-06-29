// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

// TODO: this will be redone based on new specs

Rectangle {
    property real margin: StudioTheme.Values.marginTopBottom

    id: root
    height: col.height + margin * 2

    signal showContextMenu()

    border.width: EffectMakerBackend.effectMakerModel.selectedIndex === index ? 3 : 0
    border.color: EffectMakerBackend.effectMakerModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"
    color: "transparent"
    visible: true // TODO: from rolename -> effectVisible

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            EffectMakerBackend.effectMakerModel.selectEffect(index)
            EffectMakerBackend.rootView.focusSection(0) // TODO: Set the correct section based on current effect

            if (mouse.button === Qt.LeftButton)
                // TODO: Start dragging here
                ;
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }

    ColumnLayout {
        id: col
        Layout.topMargin: margin
        Layout.bottomMargin: margin
        Row {

            width: parent.width

            Text {
                anchors.verticalCenter: parent.verticalCenter
                rightPadding: margin * 3
                text: '⋮⋮'
                color: StudioTheme.Values.themeTextColorDisabled
                font.letterSpacing: -10
                font.bold: true
                font.pointSize: StudioTheme.Values.mediumIconFontSize
            }

            StudioControls.CheckBox {
                text: modelData
                actionIndicatorVisible: false
            }
        }
    }
}
