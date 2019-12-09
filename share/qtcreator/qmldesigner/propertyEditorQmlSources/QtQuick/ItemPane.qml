/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: itemPane
    width: 320
    height: 400
    color: Theme.qmlDesignerBackgroundColorDarkAlternate()
    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    ScrollView {
        anchors.fill: parent
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

        Column {
            y: -1
            width: itemPane.width
            Section {
                z: 2
                caption: qsTr("Type")

                anchors.left: parent.left
                anchors.right: parent.right

                SectionLayout {
                    Label {
                        text: qsTr("Type")

                    }

                    SecondColumnLayout {
                        z: 2

                        RoundedPanel {
                            Layout.fillWidth: true
                            height: 24

                            Label {
                                x: 6
                                anchors.fill: parent
                                anchors.leftMargin: 16

                                text: backendValues.className.value
                                verticalAlignment: Text.AlignVCenter
                            }
                            ToolTipArea {
                                anchors.fill: parent
                                onDoubleClicked: {
                                    typeLineEdit.text = backendValues.className.value
                                    typeLineEdit.visible = ! typeLineEdit.visible
                                    typeLineEdit.forceActiveFocus()
                                }
                                tooltip: qsTr("Change the type of this item.")
                                enabled: !modelNodeBackend.multiSelection
                            }

                            ExpressionTextField {
                                z: 2
                                id: typeLineEdit
                                completeOnlyTypes: true

                                anchors.fill: parent

                                visible: false

                                showButtons: false
                                fixedSize: true

                                onEditingFinished: {
                                    if (visible)
                                        changeTypeName(typeLineEdit.text.trim())
                                    visible = false
                                }
                            }

                        }
                        Item {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                        }
                    }

                    Label {
                        text: qsTr("id")
                    }

                    SecondColumnLayout {
                        LineEdit {
                            id: lineEdit

                            backendValue: backendValues.id
                            placeholderText: qsTr("id")
                            text: backendValues.id.value
                            Layout.fillWidth: true
                            width: 240
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false
                            enabled: !modelNodeBackend.multiSelection
                        }
                        // workaround: without this item the lineedit does not shrink to the
                        // right size after resizing to a wider width

                        Image {
                            visible: !modelNodeBackend.multiSelection
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            source: hasAliasExport ? "image://icons/alias-export-checked" : "image://icons/alias-export-unchecked"
                            ToolTipArea {
                                enabled: !modelNodeBackend.multiSelection
                                anchors.fill: parent
                                onClicked: toogleExportAlias()
                                tooltip: qsTr("Toggles whether this item is exported as an alias property of the root item.")
                            }
                        }
                    }
                }
            }

            GeometrySection {
            }

            Section {
                anchors.left: parent.left
                anchors.right: parent.right

                caption: qsTr("Visibility")

                SectionLayout {
                    rows: 2
                    Label {
                        text: qsTr("Visibility")
                    }

                    SecondColumnLayout {

                        CheckBox {
                            text: qsTr("Is Visible")
                            backendValue: backendValues.visible
                        }

                        Item {
                            width: 10
                            height: 10

                        }

                        CheckBox {
                            text: qsTr("Clip")
                            backendValue: backendValues.clip
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: qsTr("Opacity")
                    }

                    SecondColumnLayout {
                        SpinBox {
                            sliderIndicatorVisible: true
                            backendValue: backendValues.opacity
                            decimals: 2

                            minimumValue: 0
                            maximumValue: 1
                            hasSlider: true
                            stepSize: 0.1
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Item {
                height: 4
                width: 4
            }

            TabView {
                anchors.left: parent.left
                anchors.right: parent.right
                frameVisible: false

                id: tabView
                height: Math.max(layoutSectionHeight, specficsHeight)

                property int layoutSectionHeight: 400
                property int specficsOneHeight: 0
                property int specficsTwoHeight: 0

                property int specficsHeight: Math.max(specficsOneHeight, specficsTwoHeight)

                property int extraHeight: 40

                Tab {
                    title: backendValues.className.value

                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            visible: theSource !== ""

                            id: specificsTwo;
                            sourceComponent: specificQmlComponent

                            property string theSource: specificQmlData

                            onTheSourceChanged: {
                                active = false
                                active = true
                            }

                            property int loaderHeight: specificsTwo.item.height + tabView.extraHeight
                            onLoaderHeightChanged: tabView.specficsTwoHeight = loaderHeight

                            onLoaded: {
                                tabView.specficsTwoHeight = loaderHeight
                            }
                        }

                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right

                            id: specificsOne;
                            source: specificsUrl;

                            property int loaderHeight: specificsOne.item.height + tabView.extraHeight
                            onLoaderHeightChanged: tabView.specficsOneHeight = loaderHeight

                            onLoaded: {
                                tabView.specficsOneHeight = loaderHeight
                            }
                        }
                    }
                }

                Tab {
                    title: qsTr("Layout")
                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LayoutSection {
                            property int childRectHeight: childrenRect.height
                            onChildRectHeightChanged: {
                                tabView.layoutSectionHeight = childRectHeight + tabView.extraHeight
                            }
                        }

                        MarginSection {
                            visible: anchorBackend.isInLayout
                            backendValueTopMargin: backendValues.Layout_topMargin
                            backendValueBottomMargin: backendValues.Layout_bottomMargin
                            backendValueLeftMargin: backendValues.Layout_leftMargin
                            backendValueRightMargin: backendValues.Layout_rightMargin
                            backendValueMargins: backendValues.Layout_margins
                        }

                        Section {
                            visible: !anchorBackend.isInLayout
                            anchors.left: parent.left
                            anchors.right: parent.right
                            caption: qsTr("Align")

                            ColumnLayout {
                                width: parent.width
                                enabled: alignDistribute.multiSelection &&
                                         !alignDistribute.selectionHasAnchors &&
                                         alignDistribute.selectionExclusivlyItems &&
                                         !alignDistribute.selectionContainsRootItem

                                AlignDistribute {
                                    id: alignDistribute
                                    modelNodeBackendProperty: modelNodeBackend
                                }

                                Label {
                                    text: qsTr("Align objects")
                                    width: 120
                                }
                                RowLayout {
                                    Row {
                                        spacing: -StudioTheme.Values.border
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignLeft
                                            tooltip: qsTr("Align objects to left edge")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.Left,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignCenterHorizontal
                                            tooltip: qsTr("Align objects horizontal center")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.CenterH,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignRight
                                            tooltip: qsTr("Align objects to right edge")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.Right,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                    }

                                    Row {
                                        spacing: -StudioTheme.Values.border
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignTop
                                            tooltip: qsTr("Align objects to top edge")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.Top,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignCenterVertical
                                            tooltip: qsTr("Align objects vertical center")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.CenterV,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.alignBottom
                                            tooltip: qsTr("Align objects to bottom edge")
                                            onClicked: alignDistribute.alignObjects(AlignDistribute.Bottom,
                                                                                    alignToComboBox.currentEnum,
                                                                                    keyObjectComboBox.currentText)
                                        }
                                    }
                                }

                                Label {
                                    text: qsTr("Distribute objects")
                                    width: 120
                                }
                                RowLayout {
                                    Row {
                                        spacing: -StudioTheme.Values.border
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeLeft
                                            tooltip: qsTr("Distribute objects left edge")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.Left,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeCenterHorizontal
                                            tooltip: qsTr("Distribute objects horizontal center")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.CenterH,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeRight
                                            tooltip: qsTr("Distribute objects right edge")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.Right,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                    }

                                    Row {
                                        spacing: -StudioTheme.Values.border
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeTop
                                            tooltip: qsTr("Distribute objects top edge")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.Top,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeCenterVertical
                                            tooltip: qsTr("Distribute objects vertical center")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.CenterV,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeBottom
                                            tooltip: qsTr("Distribute objects bottom edge")
                                            onClicked: alignDistribute.distributeObjects(AlignDistribute.Bottom,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText)
                                        }
                                    }
                                }

                                Label {
                                    text: qsTr("Distribute spacing")
                                    width: 120
                                }
                                RowLayout {
                                    Row {
                                        spacing: -StudioTheme.Values.border
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeSpacingHorizontal
                                            tooltip: qsTr("Distribute spacing horizontal")
                                            onClicked: alignDistribute.distributeSpacing(AlignDistribute.X,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText,
                                                                                         distanceSpinBox.realValue,
                                                                                         buttonRow.getDistributeDirection())
                                        }
                                        AbstractButton {
                                            buttonIcon: StudioTheme.Constants.distributeSpacingVertical
                                            tooltip: qsTr("Distribute spacing vertical")
                                            onClicked: alignDistribute.distributeSpacing(AlignDistribute.Y,
                                                                                         alignToComboBox.currentEnum,
                                                                                         keyObjectComboBox.currentText,
                                                                                         distanceSpinBox.realValue,
                                                                                         buttonRow.getDistributeDirection())
                                        }
                                    }

                                    StudioControls.ButtonRow {
                                        id: buttonRow
                                        actionIndicatorVisible: false

                                        StudioControls.ButtonGroup {
                                            id: group
                                        }

                                        function getDistributeDirection()
                                        {
                                            if (buttonLeftToRight.checked)
                                                return AlignDistribute.TopLeft
                                            else if (buttonCenter.checked)
                                                return AlignDistribute.Center
                                            else if (buttonRightToLeft.checked)
                                                return AlignDistribute.BottomRight
                                            else
                                                return AlignDistribute.None
                                        }

                                        AbstractButton {
                                            id: buttonNone
                                            checked: true // default state
                                            buttonIcon: StudioTheme.Constants.distributeOriginNone
                                            checkable: true
                                            StudioControls.ButtonGroup.group: group
                                        }
                                        AbstractButton {
                                            id: buttonLeftToRight
                                            buttonIcon: StudioTheme.Constants.distributeOriginTopLeft
                                            checkable: true
                                            StudioControls.ButtonGroup.group: group
                                        }
                                        AbstractButton {
                                            id: buttonCenter
                                            buttonIcon: StudioTheme.Constants.distributeOriginCenter
                                            checkable: true
                                            StudioControls.ButtonGroup.group: group
                                        }
                                        AbstractButton {
                                            id: buttonRightToLeft
                                            buttonIcon: StudioTheme.Constants.distributeOriginBottomRight
                                            checkable: true
                                            StudioControls.ButtonGroup.group: group
                                        }

                                        StudioControls.RealSpinBox {
                                            id: distanceSpinBox
                                            width: 64
                                            actionIndicatorVisible: false
                                            realFrom: -1000
                                            realTo: 1000
                                            enabled: !buttonNone.checked
                                        }
                                    }
                                }

                                SectionLayout {
                                    columns: 2

                                    ItemFilterModel {
                                        id: itemFilterModel
                                        modelNodeBackendProperty: modelNodeBackend
                                        selectionOnly: true
                                    }

                                    Label {
                                        text: qsTr("Align to")
                                    }
                                    ComboBox {
                                        id: alignToComboBox
                                        Layout.fillWidth: true
                                        property int currentEnum: alignTargets.get(alignToComboBox.currentIndex).value
                                        textRole: "text"
                                        model: ListModel {
                                            id: alignTargets
                                            ListElement { text: "Selection"; value: AlignDistribute.Selection }
                                            ListElement { text: "Root"; value: AlignDistribute.Root }
                                            ListElement { text: "Key object"; value: AlignDistribute.KeyObject }
                                        }
                                    }

                                    Label {
                                        text: qsTr("Key object")
                                    }
                                    ComboBox {
                                        id: keyObjectComboBox
                                        enabled: alignToComboBox.currentIndex === 2
                                        model: itemFilterModel.itemModel
                                        Layout.fillWidth: true
                                        property string lastSelectedItem: ""
                                        onCompressedActivated: lastSelectedItem = keyObjectComboBox.currentText
                                        onModelChanged: {
                                            var idx = model.indexOf(keyObjectComboBox.lastSelectedItem)
                                            if (idx !== -1)
                                                keyObjectComboBox.currentIndex = idx
                                            else
                                                lastSelectedItem = "" // TODO
                                        }
                                    }
                                }

                                SectionLayout {
                                    columns: 1
                                    Layout.topMargin: 30
                                    visible: alignDistribute.multiSelection &&
                                             (alignDistribute.selectionHasAnchors ||
                                             !alignDistribute.selectionExclusivlyItems ||
                                             alignDistribute.selectionContainsRootItem)

                                    Text {
                                        id: warningTitle
                                        font.family: StudioTheme.Constants.font.family
                                        font.pixelSize: StudioTheme.Values.myFontSize
                                        font.weight: Font.Bold
                                        color: StudioTheme.Values.themeTextColor
                                        text: qsTr("Warning")
                                    }
                                    Text {
                                        id: warningRoot
                                        visible: alignDistribute.selectionContainsRootItem
                                        Layout.fillWidth: true
                                        font.family: StudioTheme.Constants.font.family
                                        font.pixelSize: StudioTheme.Values.myFontSize
                                        color: StudioTheme.Values.themeTextColor
                                        wrapMode: Text.WordWrap
                                        text: qsTr("- The selection contains the root item.")
                                    }
                                    Text {
                                        id: warningNonVisual
                                        visible: !alignDistribute.selectionExclusivlyItems
                                        Layout.fillWidth: true
                                        font.family: StudioTheme.Constants.font.family
                                        font.pixelSize: StudioTheme.Values.myFontSize
                                        color: StudioTheme.Values.themeTextColor
                                        wrapMode: Text.WordWrap
                                        text: qsTr("- The selection contains a non visual item.")
                                    }
                                    Text {
                                        id: warningAnchors
                                        visible: alignDistribute.selectionHasAnchors
                                        Layout.fillWidth: true
                                        font.family: StudioTheme.Constants.font.family
                                        font.pixelSize: StudioTheme.Values.myFontSize
                                        color: StudioTheme.Values.themeTextColor
                                        wrapMode: Text.WordWrap
                                        text: qsTr("- An item in the selection uses anchors.")
                                    }
                                }
                            }
                        }
                    }
                }

                Tab {
                    anchors.fill: parent
                    title: qsTr("Advanced")
                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        AdvancedSection {
                        }
                        LayerSection {
                        }
                    }
                }
            }
        }
    }
}
