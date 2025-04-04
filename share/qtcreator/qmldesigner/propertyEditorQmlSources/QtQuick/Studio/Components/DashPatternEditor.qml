// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import QtQuickDesignerTheme
import StudioTheme as StudioTheme

GridLayout {
    id: root
    rowSpacing: StudioTheme.Values.sectionRowSpacing
    columnSpacing: 0
    rows: 2
    columns: 5
    Layout.fillWidth: true

    property bool __block: false
    property int labelWidth: 46
    property bool enableEditors: true
    property variant backendValue: backendValues.dashPattern
    property string expression: backendValue.expression

    onExpressionChanged: root.parseExpression()

    function createArray() {
        if (root.__block)
            return

        var array = []
        array.push(dash01.value)
        array.push(gap01.value)
        array.push(dash02.value)
        array.push(gap02.value)

        root.__block = true
        root.backendValue.expression = '[' + array.toString() + ']'
        root.__block = false
    }

    function parseExpression() {
        if (root.__block)
            return

        root.__block = true

        dash01.value = 0
        gap01.value = 0
        dash02.value = 0
        gap02.value = 0

        var array = backendValue.expression.toString()
        array = array.replace("[", "")
        array = array.replace("]", "")
        array = array.split(',')

        try {
            dash01.value = array[0]
            gap01.value = array[1]
            dash02.value = array[2]
            gap02.value = array[3]
        } catch (err) {
        }

        root.__block = false
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() { parseExpression() }
    }

    ActionIndicator {
        id: actionIndicator
        __parentControl: dash01
        icon.visible: dash01.hover || gap01.hover || dash02.hover || gap02.hover
        icon.color: extFuncLogic.color
        icon.text: extFuncLogic.glyph
        enabled: root.enableEditors
        onClicked: extFuncLogic.show()

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: root.backendValue
        }
    }

    DoubleSpinBox {
        id: dash01

        property color textColor: colorLogic.textColor

        implicitWidth: StudioTheme.Values.twoControlColumnWidth
        maximumValue: 1000
        ColorLogic {
            id: colorLogic
            backendValue: backendValues.dashPattern
        }
        enabled: root.enableEditors
        onValueChanged: root.createArray()
    }

    ControlLabel {
        text: qsTr("Dash")
        color: Theme.color(Theme.PanelTextColorLight)
        elide: Text.ElideRight
        width: root.labelWidth
        horizontalAlignment: Text.AlignLeft
        leftPadding: StudioTheme.Values.controlLabelGap
    }

    DoubleSpinBox {
        id: gap01

        property color textColor: colorLogic.textColor

        implicitWidth: StudioTheme.Values.twoControlColumnWidth
        maximumValue: 1000
        enabled: root.enableEditors
        onValueChanged: root.createArray()
    }

    ControlLabel {
        text: qsTr("Gap")
        color: Theme.color(Theme.PanelTextColorLight)
        elide: Text.ElideRight
        width: root.labelWidth
        horizontalAlignment: Text.AlignLeft
        leftPadding: StudioTheme.Values.controlLabelGap
    }

    Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

    DoubleSpinBox {
        id: dash02

        property color textColor: colorLogic.textColor

        implicitWidth: StudioTheme.Values.twoControlColumnWidth
        maximumValue: 1000
        enabled: root.enableEditors
        onValueChanged: root.createArray()
    }

    ControlLabel {
        text: qsTr("Dash")
        color: Theme.color(Theme.PanelTextColorLight)
        elide: Text.ElideRight
        width: root.labelWidth
        horizontalAlignment: Text.AlignLeft
        leftPadding: StudioTheme.Values.controlLabelGap
    }

    DoubleSpinBox {
        id: gap02

        property color textColor: colorLogic.textColor

        implicitWidth: StudioTheme.Values.twoControlColumnWidth
        maximumValue: 1000
        enabled: root.enableEditors
        onValueChanged: root.createArray()
    }

    ControlLabel {
        text: qsTr("Gap")
        color: Theme.color(Theme.PanelTextColorLight)
        elide: Text.ElideRight
        width: root.labelWidth
        horizontalAlignment: Text.AlignLeft
        leftPadding: StudioTheme.Values.controlLabelGap
    }
}
