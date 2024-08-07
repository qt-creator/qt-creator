/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import QtQuick.Templates as T
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Local Custom Properties")

    property DynamicPropertiesModel propertiesModel: null

    property Component colorEditor: Component {
        id: colorEditor
        ColorEditor {
            id: colorEditorControl
            property string propertyType

            signal remove

            supportGradient: false
            spacer.visible: false

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: colorEditorControl.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component intEditor: Component {
        id: intEditor
        SecondColumnLayout {
            id: layoutInt
            property var backendValue
            property string propertyType

            signal remove

            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                backendValue: layoutInt.backendValue
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            Item {
                height: 10
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutInt.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component realEditor: Component {
        id: realEditor
        SecondColumnLayout {
            id: layoutReal
            property var backendValue
            property string propertyType

            signal remove

            SpinBox {
                backendValue: layoutReal.backendValue
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                stepSize: 0.1
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            Item {
                height: 10
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutReal.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component stringEditor: Component {
        id: stringEditor
        SecondColumnLayout {
            id: layoutString
            property var backendValue
            property string propertyType

            signal remove

            LineEdit {
                backendValue: layoutString.backendValue
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutString.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component boolEditor: Component {
        id: boolEditor
        SecondColumnLayout {
            id: layoutBool
            property var backendValue
            property string propertyType

            signal remove

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: layoutBool.backendValue.value
                backendValue: layoutBool.backendValue
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            Item {
                height: 10
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutBool.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component urlEditor: Component {
        id: urlEditor
        SecondColumnLayout {
            id: layoutUrl
            property var backendValue
            property string propertyType

            signal remove

            UrlChooser {
                backendValue: layoutUrl.backendValue
                comboBox.implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                        + StudioTheme.Values.actionIndicatorWidth
                spacer.implicitWidth: StudioTheme.Values.controlLabelGap
            }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutUrl.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component aliasEditor: Component {
        id: aliasEditor
        SecondColumnLayout {
            id: layoutAlias
            property var backendValue
            property string propertyType
            property alias lineEdit: lineEdit

            signal remove

            function updateLineEditText() {
                lineEdit.text = lineEdit.backendValue.expression
            }

            LineEdit {
                id: lineEdit
                backendValue: layoutAlias.backendValue
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                writeAsExpression: true
                showTranslateCheckBox: false
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutAlias.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component textureInputEditor: Component {
        id: textureInputEditor
        SecondColumnLayout {
            id: layoutTextureInput
            property var backendValue
            property string propertyType

            signal remove

            ItemFilterComboBox {
                typeFilter: "QtQuick3D.TextureInput"
                backendValue: layoutTextureInput.backendValue
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutTextureInput.remove()
            }

            ExpandingSpacer {}
        }
    }

    property Component vectorEditor: Component {
        id: vectorEditor
        ColumnLayout {
            id: layoutVector
            property var backendValue
            property string propertyType
            property int vecSize: 0
            property var proxyValues: []
            property var spinBoxes: [boxX, boxY, boxZ, boxW]
            property bool block: false

            signal remove

            onVecSizeChanged: layoutVector.updateProxyValues()

            spacing: StudioTheme.Values.sectionRowSpacing

            function isValidValue(v) {
                return !(v === undefined || isNaN(v))
            }

            function updateExpressionFromExpression() {
                if (layoutVector.block)
                    return

                layoutVector.backendValue.expression = layoutVector.proxyValues[0].expression
                // Only the first proxy value has an expression editor enabled
            }

            function updateExpressionFromValue() {
                if (layoutVector.block)
                    return

                for (let i = 0; i < vecSize; ++i) {
                    if (!layoutVector.isValidValue(layoutVector.proxyValues[i].value))
                        return
                }

                let expStr = "Qt.vector" + layoutVector.vecSize + "d(" + layoutVector.proxyValues[0].value
                for (let j=1; j < layoutVector.vecSize; ++j)
                    expStr += ", " + layoutVector.proxyValues[j].value
                expStr += ")"

                layoutVector.backendValue.expression = expStr
            }

            function updateProxyValues() {
                if (!layoutVector.backendValue)
                    return;

                let vals = layoutVector.backendValue.getExpressionAsVector()

                layoutVector.block = true

                if (layoutVector.vecSize === vals.length) {
                    for (let j = 0; j < layoutVector.vecSize; ++j) {
                        layoutVector.proxyValues[j].setForceBound(false)
                        layoutVector.proxyValues[j].value = vals[j]
                    }
                } else {
                    for (let j = 0; j < layoutVector.vecSize; ++j) {
                        layoutVector.proxyValues[j].setForceBound(true) // Required since the backendValue is just proxied
                        layoutVector.proxyValues[j].expression = layoutVector.backendValue.expression
                    }
                }

                layoutVector.block = false
            }

            SecondColumnLayout {
                SpinBox {
                    id: boxX
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "X"
                    tooltip: "X"
                }

                Spacer {
                    implicitWidth: StudioTheme.Values.controlGap
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                SpinBox {
                    id: boxY
                    actionIndicatorVisible: false
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "Y"
                    tooltip: "Y"
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                IconIndicator {
                    icon: StudioTheme.Constants.closeCross
                    onClicked: layoutVector.remove()
                }

                ExpandingSpacer {}
            }

            SecondColumnLayout {
                visible: layoutVector.vecSize > 2
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                SpinBox {
                    id: boxZ
                    actionIndicatorVisible: false
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "Z"
                    tooltip: "Z"
                    visible: layoutVector.vecSize > 2
                }

                Spacer {
                    implicitWidth: StudioTheme.Values.controlGap
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                SpinBox {
                    id: boxW
                    actionIndicatorVisible: false
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                    visible: layoutVector.vecSize > 3
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "W"
                    tooltip: "W"
                    visible: layoutVector.vecSize > 3
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                Item {
                    height: 10
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                  + StudioTheme.Values.actionIndicatorWidth
                    visible: layoutVector.vecSize === 2 // Placeholder for last spinbox
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                ExpandingSpacer {}
            }
        }
    }

    property Component readonlyEditor: Component {
        id: readonlyEditor
        SecondColumnLayout {
            id: layoutReadonly
            property var backendValue
            property string propertyType

            signal remove

            PropertyLabel {
                tooltip: layoutReadonly.propertyType
                horizontalAlignment: Text.AlignLeft
                leftPadding: StudioTheme.Values.actionIndicatorWidth
                text: qsTr("No editor for type: ") + layoutReadonly.propertyType

                width: StudioTheme.Values.singleControlColumnWidth
                       + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            IconIndicator {
                icon: StudioTheme.Constants.closeCross
                onClicked: layoutReadonly.remove()
            }

            ExpandingSpacer {}
        }
    }

    Column {
        width: parent.width
        spacing: StudioTheme.Values.sectionRowSpacing

        Repeater {
            id: repeater
            model: root.propertiesModel

            property bool loadActive: true

            SectionLayout {
                DynamicPropertyRow {
                    id: propertyRow
                    model: root.propertiesModel
                    row: index
                }

                PropertyLabel {
                    text: propertyName
                    tooltip: propertyType
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 6
                }

                Loader {
                    id: loader
                    asynchronous: true
                    active: repeater.loadActive
                    width: loader.item ? loader.item.width : 0
                    height: loader.item ? loader.item.height : 0
                    Layout.fillWidth: true

                    sourceComponent: {
                        if (propertyType == "color")
                            return colorEditor
                        if (propertyType == "int")
                            return intEditor
                        if (propertyType == "real")
                            return realEditor
                        if (propertyType == "double")
                            return realEditor
                        if (propertyType == "string")
                            return stringEditor
                        if (propertyType == "bool")
                            return boolEditor
                        if (propertyType == "url")
                            return urlEditor
                        if (propertyType == "alias")
                            return aliasEditor
                        if (propertyType == "variant")
                            return readonlyEditor
                        if (propertyType == "var")
                            return readonlyEditor
                        if (propertyType == "TextureInput")
                            return textureInputEditor
                        if (propertyType == "vector2d" || propertyType == "vector3d" || propertyType == "vector4d")
                            return vectorEditor

                        return readonlyEditor
                    }

                    onLoaded: {
                        loader.item.backendValue = propertyRow.backendValue
                        loader.item.propertyType = propertyType
                        if (sourceComponent == vectorEditor) {
                            let vecSize = 2
                            if (propertyType == "vector3d")
                                vecSize = 3
                            else if (propertyType == "vector4d")
                                vecSize = 4
                            propertyRow.clearProxyBackendValues()

                            for (let i = 0; i < vecSize; ++i) {
                                var newProxyValue = propertyRow.createProxyBackendValue()
                                loader.item.proxyValues.push(newProxyValue)
                                newProxyValue.valueChangedQml.connect(loader.item.updateExpressionFromValue)
                                newProxyValue.expressionChangedQml.connect(loader.item.updateExpressionFromExpression)
                                loader.item.spinBoxes[i].backendValue = newProxyValue
                            }
                            propertyRow.backendValue.expressionChanged.connect(loader.item.updateProxyValues)
                            loader.item.vecSize = vecSize
                            loader.item.updateProxyValues()
                        } else if (sourceComponent == aliasEditor) {
                            loader.item.lineEdit.text = propertyRow.backendValue.expression
                            loader.item.backendValue.expressionChanged.connect(loader.item.updateLineEditText)
                        } else if (sourceComponent == colorEditor) {
                            loader.item.initEditor()
                        }
                    }

                    Connections {
                        target: loader.item
                        function onRemove() {
                            propertyRow.remove()
                        }
                    }
                }
            }
        }

        SectionLayout {
            PropertyLabel {
                text: ""
                tooltip: ""
            }

            SecondColumnLayout {
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                StudioControls.AbstractButton {
                    id: plusButton
                    buttonIcon: StudioTheme.Constants.plus
                    onClicked: {
                        cePopup.opened ? cePopup.close() : cePopup.open()
                        forceActiveFocus()
                    }
                }

                ExpandingSpacer {}
            }
        }
    }

    property T.Popup popup: T.Popup {
        id: cePopup

        onOpened: {
            cePopup.setPopupY()
            cePopup.setMainScrollViewHeight()
        }

        function setMainScrollViewHeight() {
            if (Controller.mainScrollView == null)
                return

            var mappedPos = plusButton.mapToItem(Controller.mainScrollView.contentItem,
                                                 cePopup.x, cePopup.y)
            Controller.mainScrollView.temporaryHeight = mappedPos.y + cePopup.height
                    + StudioTheme.Values.colorEditorPopupMargin
        }

        function setPopupY() {
            if (Controller.mainScrollView == null)
                return

            var mappedPos = plusButton.mapToItem(Controller.mainScrollView.contentItem,
                                               plusButton.x, plusButton.y)
            cePopup.y = Math.max(-mappedPos.y + StudioTheme.Values.colorEditorPopupMargin,
                                 cePopup.__defaultY)

            textField.text = root.propertiesModel.newPropertyName()
        }

        onClosed: Controller.mainScrollView.temporaryHeight = 0

        property real __defaultX: (Controller.mainScrollView.contentItem.width
                                   - StudioTheme.Values.colorEditorPopupWidth * 1.5) / 2

        property real __defaultY: - StudioTheme.Values.colorEditorPopupPadding
                                  - (StudioTheme.Values.colorEditorPopupSpacing * 2)
                                  - StudioTheme.Values.defaultControlHeight
                                  - StudioTheme.Values.colorEditorPopupLineHeight
                                  + plusButton.width * 0.5

        x: cePopup.__defaultX
        y: cePopup.__defaultY

        width: 270
        height: 160

        property int itemWidth: width / 2
        property int labelWidth: itemWidth - 32

        padding: StudioTheme.Values.border
        margins: 2 // If not defined margin will be -1

        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent

        contentItem: Item {
            id: content
            Column {
                spacing: StudioTheme.Values.sectionRowSpacing
                RowLayout {
                    width: cePopup.width - 8
                    PropertyLabel {
                        text: qsTr("Add New Property")
                        horizontalAlignment: Text.AlignLeft
                        leftPadding: 8
                        width: cePopup.width - closeIndicator.width - 24
                    }
                    IconIndicator {
                        id: closeIndicator
                        icon: StudioTheme.Constants.colorPopupClose
                        pixelSize: StudioTheme.Values.myIconFontSize
                        onClicked: cePopup.close()
                        Layout.alignment: Qt.AlignRight
                    }
                }
                RowLayout {
                    PropertyLabel {
                        id: textLabel
                        text: qsTr("Name")
                        width: cePopup.labelWidth
                    }
                    StudioControls.TextField {
                        id: textField
                        actionIndicator.visible: false
                        translationIndicatorVisible: false
                        width: cePopup.itemWidth
                        rightPadding: 8
                        validator: PropertyNameValidator {}

                        Binding {
                            when: !textField.acceptableInput && textField.text !== ""
                            textField.color: StudioTheme.Values.themeRedLight
                        }
                    }
                }
                RowLayout {
                    PropertyLabel {
                        text: qsTr("Type")
                        width: cePopup.labelWidth
                    }
                    StudioControls.ComboBox {
                        id: comboBox
                        actionIndicator.visible: false
                        model: ["int", "real", "double", "color", "string", "bool", "url", "alias", "signal",
                                "TextureInput", "vector2d", "vector3d", "vector4d"]
                        width: cePopup.itemWidth
                    }
                }
                Item {
                    width: 1
                    height: StudioTheme.Values.sectionRowSpacing
                }

                RowLayout {
                    width: cePopup.width

                    StudioControls.AbstractButton {
                        id: acceptButton

                        buttonIcon: qsTr("Add Property")
                        iconFont: StudioTheme.Constants.font
                        width: cePopup.width / 3
                        enabled: textField.acceptableInput

                        onClicked: {
                            root.propertiesModel.createProperty(textField.text, comboBox.currentText)
                            cePopup.close()
                        }
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
        background: Rectangle {
            color: StudioTheme.Values.themeControlBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: StudioTheme.Values.border
            MouseArea {
                // This area is to eat clicks so they do not go through the popup
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
            }
        }

        enter: Transition {}
        exit: Transition {}
    }
}
