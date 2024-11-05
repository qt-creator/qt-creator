// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets
import EffectComposerBackend

Item {
    id: root

    height: column.implicitHeight + (root.verticalSpacing * 4)

    readonly property real horizontalSpacing: 10
    readonly property real verticalSpacing: 5
    readonly property real labelWidth: 140
    readonly property real controlWidth: (root.width - labelWidth - root.horizontalSpacing
                                          - (StudioTheme.Values.popupMargin * 2))

    property var backendModel: EffectComposerBackend.effectComposerModel

    property string effectNodeName

    property int typeIndex: 1
    property string displayName
    property string uniName
    property string description
    property var minValue: 0
    property var maxValue: 1
    property var defaultValue: 0
    property bool showMinMax: false
    property var minPossibleValue: 0
    property var maxPossibleValue: 0
    property bool userAdded: false
    property bool uniNameEdited: false

    property real minFloat: root.backendModel.valueLimit("float", false)
    property real maxFloat: root.backendModel.valueLimit("float", true)
    property int minInt: root.backendModel.valueLimit("int", false)
    property int maxInt: root.backendModel.valueLimit("int", true)

    property var reservedDispNames: []
    property var reservedUniNames: []
    property bool propNameError: root.reservedDispNames.includes(nameText.text)
    property bool uniNameError: root.reservedUniNames.includes(uniNameText.text)

    property var typeList: [
        {"type": "int",       "controlType": "int"},
        {"type": "float",     "controlType": "float"},
        {"type": "bool",      "controlType": "bool"},
        {"type": "vec2",      "controlType": "vec2"},
        {"type": "vec3",      "controlType": "vec3"},
        {"type": "vec4",      "controlType": "vec4"},
        {"type": "color",     "controlType": "color"},
        {"type": "int",       "controlType": "channel"},
        {"type": "sampler2D", "controlType": "sampler2D"},
        {"type": "define",    "controlType": "int"},
        {"type": "define",    "controlType": "bool"}
    ]

    property var displayTypes: [
        qsTr("Integer"),
        qsTr("Float"),
        qsTr("Boolean"),
        qsTr("Vector 2D"),
        qsTr("Vector 3D"),
        qsTr("Vector 4D"),
        qsTr("Color"),
        qsTr("Color channel"),
        qsTr("Texture sampler"),
        qsTr("Define (integer)"),
        qsTr("Define (boolean)")
    ]

    property vector4d compareMatches: Qt.vector4d(0, 0, 0, 0)

    // Backend value for sampler controls (UrlChooser needs this to function)
    property var urlChooserBackendValue: QtObject {
        id: urlChooserValue
        property bool isBound: false
        property var expression
        property string valueToString
        property var value

        signal valueChangedQml()

        onValueChanged: {
            urlChooserValue.valueToString = value.toString()
            urlChooserValue.valueChangedQml()
        }
    }

    signal accepted()
    signal canceled()

    // If vector is assigned to variant, it just creates reference by default, which breaks the
    // min/max logic. This function creates a deep copy of vectors.
    function copyValue(v)
    {
        if (root.typeList[root.typeIndex].type === "vec2")
            return Qt.vector2d(v.x, v.y)
        else if (root.typeList[root.typeIndex].type === "vec3")
            return Qt.vector3d(v.x, v.y, v.z)
        else if (root.typeList[root.typeIndex].type === "vec4")
            return Qt.vector4d(v.x, v.y, v.z, v.w)

        return v
    }

    // For vectors, copies values of previous comparison matched subcomponents only
    function copyMatchedSubcomponents(v, orig)
    {
        let isVec2 = root.typeList[root.typeIndex].type === "vec2"
        let isVec3 = root.typeList[root.typeIndex].type === "vec3"
        let isVec4 = root.typeList[root.typeIndex].type === "vec4"
        if (isVec2 || isVec3 || isVec4) {
            let vx = root.compareMatches.x > 0 ? v.x : orig.x
            let vy = root.compareMatches.y > 0 ? v.y : orig.y
            if (isVec3 || isVec4) {
                let vz = root.compareMatches.z > 0 ? v.z : orig.z
                if (isVec4) {
                    let vw = root.compareMatches.w > 0 ? v.w : orig.w
                    return Qt.vector4d(vx, vy, vz, vw)
                } else {
                    return Qt.vector3d(vx, vy, vz)
                }
            } else {
                return Qt.vector2d(vx, vy)
            }
        }

        return v
    }

    function showForAdd()
    {
        root.uniNameEdited = false

        if (typeCombo.currentIndex === 0)
            reloadType()
        else
            typeCombo.currentIndex = 0

        root.displayName = root.backendModel.getUniqueDisplayName(root.reservedDispNames)
        root.uniName = root.backendModel.generateUniformName(root.effectNodeName, root.displayName, "")
        root.description = ""
        root.userAdded = true

        titleLabel.text = qsTr("Add property")
        root.visible = true
    }

    function showForEdit(type, controlType, dispName, name, desc, defVal, min, max, user)
    {
        root.uniNameEdited = true

        let targetIndex = typeList.findIndex(function(element) {
            return element.type === type && element.controlType === controlType;
        })

        if (targetIndex < 0)
            return;

        root.userAdded = user

        if (typeCombo.currentIndex === targetIndex)
            reloadType()
        else
            typeCombo.currentIndex = targetIndex

        root.displayName = dispName
        root.description = desc
        root.uniName = name

        if (root.showMinMax) {
            root.minValue = root.copyValue(root.minPossibleValue)
            root.maxValue = root.copyValue(root.maxPossibleValue)
            root.maxValue = root.copyValue(max)
            root.minValue = root.copyValue(min)
            minValueLoader.uniformValue = root.copyValue(min)
            maxValueLoader.uniformValue = root.copyValue(max)
        }
        root.defaultValue = root.copyValue(defVal)
        if (root.urlChooserBackendValue)
            root.urlChooserBackendValue.value = root.copyValue(root.defaultValue)
        defaultValueLoader.uniformValue = root.copyValue(defVal)

        titleLabel.text = qsTr("Edit property")
        root.visible = true
    }

    function propertyData()
    {
        let propData = {
            "type": root.typeList[root.typeIndex].type,
            "controlType": root.typeList[root.typeIndex].controlType,
            "displayName": root.displayName,
            "name": root.uniName,
            "description": root.description,
            "minValue": root.minValue,
            "maxValue": root.maxValue,
            "defaultValue": root.defaultValue,
            "userAdded": root.userAdded
        }

        return propData;
    }

    // Returns true if variant a is considered larger than variant b.
    // In case of vectors, any subcomponent being larger qualifies.
    // root.compareMatches subproperties are set to non-zero to indicate which subproperties
    // were matched.
    function compareValues(a, b)
    {
        let isVec2 = root.typeList[root.typeIndex].type === "vec2"
        let isVec3 = root.typeList[root.typeIndex].type === "vec3"
        let isVec4 = root.typeList[root.typeIndex].type === "vec4"
        if (isVec2 || isVec3 || isVec4) {
            root.compareMatches = Qt.vector4d(0, 0, 0, 0)
            let match = false
            if (a.x > b.x) {
                match = true
                root.compareMatches.x = 1
            }
            if (a.y > b.y) {
                match = true
                root.compareMatches.y = 1
            }
            if (isVec3 || isVec4) {
                if (a.z > b.z) {
                    match = true
                    root.compareMatches.z = 1
                }
                if (isVec4) {
                    if (a.w > b.w) {
                        match = true
                        root.compareMatches.w = 1
                    }
                }
            }
            return match
        } else {
            return a > b
        }
    }

    function reloadType()
    {
        defaultValueLoader.source = ""
        minValueLoader.source = ""
        maxValueLoader.source = ""

        defaultValueLoader.uniformBackendValue = null

        var sourceQml
        let hasMinMax = false
        if (root.typeList[root.typeIndex].controlType === "int") {
            root.minPossibleValue = root.minInt
            root.maxPossibleValue = root.maxInt
            root.defaultValue = 0
            root.minValue = 0
            root.maxValue = 100
            hasMinMax = true
            sourceQml = "ValueInt.qml"
        } else if (root.typeList[root.typeIndex].controlType === "channel") {
            root.minPossibleValue = root.minInt
            root.maxPossibleValue = root.maxInt
            root.defaultValue = 3
            root.minValue = 0
            root.maxValue = 3
            hasMinMax = false // Color channels have hardcoded min/max, so don't show controls
            sourceQml= "ValueChannel.qml"
        } else if (root.typeList[root.typeIndex].controlType === "vec2") {
            root.minPossibleValue = Qt.vector2d(root.minFloat, root.minFloat)
            root.maxPossibleValue = Qt.vector2d(root.maxFloat, root.maxFloat)
            root.defaultValue = Qt.vector2d(0, 0)
            root.minValue = Qt.vector2d(0, 0)
            root.maxValue = Qt.vector2d(1, 1)
            hasMinMax = true
            sourceQml = "ValueVec2.qml"
        } else if (root.typeList[root.typeIndex].controlType === "vec3") {
            root.minPossibleValue = Qt.vector3d(root.minFloat, root.minFloat, root.minFloat)
            root.maxPossibleValue = Qt.vector3d(root.maxFloat, root.maxFloat, root.maxFloat)
            root.defaultValue = Qt.vector3d(0, 0, 0)
            root.minValue = Qt.vector3d(0, 0, 0)
            root.maxValue = Qt.vector3d(1, 1, 1)
            hasMinMax = true
            sourceQml = "ValueVec3.qml"
        } else if (root.typeList[root.typeIndex].controlType === "vec4") {
            root.minPossibleValue = Qt.vector4d(root.minFloat, root.minFloat, root.minFloat, root.minFloat)
            root.maxPossibleValue = Qt.vector4d(root.maxFloat, root.maxFloat, root.maxFloat, root.maxFloat)
            root.defaultValue = Qt.vector4d(0, 0, 0, 0)
            root.minValue = Qt.vector4d(0, 0, 0, 0)
            root.maxValue = Qt.vector4d(1, 1, 1, 1)
            hasMinMax = true
            sourceQml = "ValueVec4.qml"
        } else if (root.typeList[root.typeIndex].controlType === "bool") {
            root.defaultValue = false
            sourceQml = "ValueBool.qml"
        } else if (root.typeList[root.typeIndex].controlType === "color") {
            root.defaultValue = Qt.rgba(0, 0, 0, 1)
            sourceQml = "ValueColor.qml"
        } else if (root.typeList[root.typeIndex].controlType === "sampler2D") {
            root.defaultValue = ""
            sourceQml = "ValueImage.qml"
            defaultValueLoader.uniformBackendValue = root.urlChooserBackendValue
            root.urlChooserBackendValue.value = root.copyValue(root.defaultValue)
        } else {
            root.minPossibleValue = root.minFloat
            root.maxPossibleValue = root.maxFloat
            root.defaultValue = 0
            root.minValue = 0
            root.maxValue = 1
            hasMinMax = true
            sourceQml = "ValueFloat.qml"
        }

        if (hasMinMax) {
            minValueLoader.uniformValue = root.copyValue(root.minValue)
            maxValueLoader.uniformValue = root.copyValue(root.maxValue)
            minValueLoader.source = sourceQml
            maxValueLoader.source = sourceQml
        }

        defaultValueLoader.uniformValue = root.copyValue(root.defaultValue)
        defaultValueLoader.source = sourceQml

        root.showMinMax = hasMinMax
    }

    onMinValueChanged: {
        if (!root.showMinMax)
            return

        if (root.compareValues(root.minValue, root.maxValue)) {
            root.maxValue = root.copyMatchedSubcomponents(root.minValue, root.maxValue)
            maxValueLoader.uniformValue = root.copyValue(root.maxValue)
        }
        if (root.compareValues(root.minValue, root.defaultValue)) {
            root.defaultValue = root.copyMatchedSubcomponents(root.minValue, root.defaultValue)
            defaultValueLoader.uniformValue = root.copyValue(root.defaultValue)
        }
    }
    onMaxValueChanged: {
        if (!root.showMinMax)
            return

        if (root.compareValues(root.minValue, root.maxValue)) {
            root.minValue = root.copyMatchedSubcomponents(root.maxValue, root.minValue)
            minValueLoader.uniformValue = root.copyValue(root.minValue)
        }
        if (root.compareValues(root.defaultValue, root.maxValue)) {
            root.defaultValue = root.copyMatchedSubcomponents(root.maxValue, root.defaultValue)
            defaultValueLoader.uniformValue = root.copyValue(root.defaultValue)
        }
    }

    onTypeIndexChanged: reloadType()

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.rightMargin: 8
        border.width: 1
        border.color: StudioTheme.Values.themeControlOutline
        color: StudioTheme.Values.themeSectionHeadBackground

        Column {
            id: column
            anchors.fill: parent
            anchors.topMargin: root.verticalSpacing
            anchors.bottomMargin: root.verticalSpacing
            spacing: root.verticalSpacing

            Text {
                id: titleLabel
                color: StudioTheme.Values.themeControlOutlineInteraction
                font.bold: true
                font.pixelSize: StudioTheme.Values.baseFontSize
                width: parent.width
                height: 30
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                spacing: root.horizontalSpacing
                width: parent.width

                Text {
                    id: nameLabel
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Display Name")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the display name of the property.")
                    }
                }

                StudioControls.TextField {
                    id: nameText

                    width: root.controlWidth
                    anchors.verticalCenter: parent.verticalCenter

                    actionIndicatorVisible: false
                    translationIndicatorVisible: false

                    text: root.displayName

                    KeyNavigation.tab: uniNameText

                    onEditingFinished: {
                        root.displayName = nameText.text
                        if (!root.uniNameEdited) {
                            root.uniName = root.backendModel.generateUniformName(
                                        root.effectNodeName, root.displayName, root.uniName)
                        }
                    }
                }
            }

            // Error line (for invalid name)
            Row {
                width: parent.width
                spacing: root.horizontalSpacing

                Item { // Spacer
                    width: root.labelWidth
                    height: 1
                }

                Text {
                    text: qsTr("Display name of the property has to be unique.")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.controlWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    visible: root.propNameError
                    color: StudioTheme.Values.themeWarning
                }
            }

            Row {
                spacing: root.horizontalSpacing
                width: parent.width

                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Uniform Name")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the uniform name of the property.")
                    }
                }

                StudioControls.TextField {
                    id: uniNameText

                    width: root.controlWidth
                    anchors.verticalCenter: parent.verticalCenter

                    actionIndicatorVisible: false
                    translationIndicatorVisible: false

                    text: root.uniName

                    KeyNavigation.tab: descriptionEdit

                    onEditingFinished:{
                        if (root.uniName !== uniNameText.text) {
                            root.uniName = uniNameText.text
                            root.uniNameEdited = true
                        }
                    }
                }
            }

            // Error line (for invalid uniform name)
            Row {
                width: parent.width
                spacing: root.horizontalSpacing

                Item { // Spacer
                    width: root.labelWidth
                    height: 1
                }

                Text {
                    text: qsTr("Uniform name has to be unique.")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.controlWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    visible: root.uniNameError
                    color: StudioTheme.Values.themeWarning
                }
            }

            Row {
                spacing: root.horizontalSpacing
                width: parent.width

                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Description")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the property description.")
                    }
                }

                Rectangle {
                    color: StudioTheme.Values.controlStyle.background.idle
                    border.color: StudioTheme.Values.controlStyle.border.idle
                    border.width: StudioTheme.Values.controlStyle.borderWidth
                    height: descriptionEdit.height
                    width: root.controlWidth
                    clip: true

                    TextEdit {
                        id: descriptionEdit
                        padding: StudioTheme.Values.controlStyle.inputHorizontalPadding
                        width: parent.width
                        height: descriptionEdit.contentHeight + descriptionEdit.topPadding
                                + descriptionEdit.bottomPadding

                        font.pixelSize: StudioTheme.Values.baseFontSize
                        color: StudioTheme.Values.themeTextColor
                        wrapMode: TextEdit.WordWrap

                        text: root.description

                        KeyNavigation.tab: typeCombo

                        onEditingFinished: root.description = descriptionEdit.text
                    }
                }
            }

            Row {
                width: parent.width
                spacing: root.horizontalSpacing
                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Type")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the property type.")
                    }
                }

                StudioControls.ComboBox {
                    id: typeCombo
                    style: StudioTheme.Values.connectionPopupControlStyle
                    width: root.controlWidth

                    model: root.displayTypes
                    actionIndicatorVisible: false

                    onCurrentIndexChanged: root.typeIndex = currentIndex

                    delegate: ItemDelegate {
                        required property int index
                        required property var modelData

                        width: typeCombo.width
                        highlighted: typeCombo.highlightedIndex === index
                        contentItem: Text {
                            text: modelData
                            color: StudioTheme.Values.themeTextColor
                            font: typeCombo.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: root.horizontalSpacing
                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Default Value")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the default value of the property.")
                    }
                }

                Loader {
                    id: defaultValueLoader

                    property var uniformValue

                    // No need to deep copy min/max as these bindings do not change the value
                    property var uniformMinValue: root.minValue
                    property var uniformMaxValue: root.maxValue

                    // These are needed for sampler controls (ValueImage type)
                    property var uniformBackendValue
                    property string uniformName: root.uniName
                    property var uniformDefaultValue

                    width: root.controlWidth

                    Connections {
                        target: defaultValueLoader.item

                        function onValueChanged() {
                            root.defaultValue = root.copyValue(defaultValueLoader.uniformValue)
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: root.horizontalSpacing
                visible: root.showMinMax
                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Min Value")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the minimum value of the property.")
                    }
                }

                Loader {
                    id: minValueLoader

                    property var uniformValue
                    // No need to deep copy min/max as these bindings do not change the value
                    property var uniformMinValue: root.minPossibleValue
                    property var uniformMaxValue: root.maxValue

                    width: root.controlWidth

                    Connections {
                        target: minValueLoader.item

                        function onValueChanged() {
                            root.minValue = root.copyValue(minValueLoader.uniformValue)
                        }
                    }

                    onLoaded: {
                        if (root.typeList[root.typeIndex].controlType === "int"
                            || root.typeList[root.typeIndex].controlType === "float") {
                            item.hideSlider = true
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: root.horizontalSpacing
                visible: root.showMinMax
                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Max Value")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.labelWidth
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight

                    StudioControls.ToolTipArea {
                        anchors.fill: parent
                        text: qsTr("Sets the maximum value of the property.")
                    }
                }

                Loader {
                    id: maxValueLoader

                    property var uniformValue
                    // No need to deep copy min/max as these bindings do not change the value
                    property var uniformMinValue: root.minValue
                    property var uniformMaxValue: root.maxPossibleValue

                    width: root.controlWidth

                    Connections {
                        target: maxValueLoader.item

                        function onValueChanged() {
                            root.maxValue = root.copyValue(maxValueLoader.uniformValue)
                        }
                    }

                    onLoaded: {
                        if (root.typeList[root.typeIndex].controlType === "int"
                            || root.typeList[root.typeIndex].controlType === "float") {
                            item.hideSlider = true
                        }
                    }
                }
            }

            Row {
                width: acceptButton.width + root.horizontalSpacing + cancelButton.width
                spacing: root.horizontalSpacing
                anchors.horizontalCenter: parent.horizontalCenter
                height: 35

                HelperWidgets.Button {
                    id: cancelButton
                    width: 130
                    height: 35
                    text: qsTr("Cancel")
                    padding: 4

                    onClicked: {
                        root.canceled()
                        root.visible = false
                    }
                }

                HelperWidgets.Button {
                    id: acceptButton
                    width: 130
                    height: 35
                    text: qsTr("Apply")
                    padding: 4
                    enabled: !root.propNameError && !root.uniNameError

                    onClicked: {
                        root.accepted()
                        root.visible = false
                    }
                }
            }
        }
    }
}
