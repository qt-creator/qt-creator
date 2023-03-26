// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root

     readonly property bool warningVisible: alignDistribute.multiSelection &&
                                            (alignDistribute.selectionHasAnchors ||
                                            !alignDistribute.selectionExclusivlyItems ||
                                            alignDistribute.selectionContainsRootItem)

    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Alignment")

    SectionLayout {
        enabled: alignDistribute.multiSelection &&
                 !alignDistribute.selectionHasAnchors &&
                 alignDistribute.selectionExclusivlyItems &&
                 !alignDistribute.selectionContainsRootItem

        AlignDistribute {
            id: alignDistribute
            modelNodeBackendProperty: modelNodeBackend
        }

        PropertyLabel { text: qsTr("Alignment") }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            Row {
                id: horizontalAlignmentButtons
                spacing: -StudioTheme.Values.border

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignLeft
                    tooltip: qsTr("Align left edges.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.Left,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignCenterHorizontal
                    tooltip: qsTr("Align horizontal centers.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.CenterH,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignRight
                    tooltip: qsTr("Align right edges.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.Right,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }
            }

            Spacer {
                implicitWidth: StudioTheme.Values.controlGap
                               + StudioTheme.Values.controlLabelWidth
                               + StudioTheme.Values.controlGap
                               + StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                               - horizontalAlignmentButtons.implicitWidth
            }

            Row {
                spacing: -StudioTheme.Values.border

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignTop
                    tooltip: qsTr("Align top edges.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.Top,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignCenterVertical
                    tooltip: qsTr("Align vertical centers.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.CenterV,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.alignBottom
                    tooltip: qsTr("Align bottom edges.")
                    onClicked: alignDistribute.alignObjects(AlignDistribute.Bottom,
                                                            alignToComboBox.currentEnum,
                                                            keyObjectComboBox.currentText)
                }
            }
        }

        PropertyLabel { text: qsTr("Distribute objects") }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            Row {
                id: horizontalDistributionButtons
                spacing: -StudioTheme.Values.border

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeLeft
                    tooltip: qsTr("Distribute left edges.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.Left,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeCenterHorizontal
                    tooltip: qsTr("Distribute horizontal centers.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.CenterH,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeRight
                    tooltip: qsTr("Distribute right edges.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.Right,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }
            }

            Spacer {
                implicitWidth: StudioTheme.Values.controlGap
                               + StudioTheme.Values.controlLabelWidth
                               + StudioTheme.Values.controlGap
                               + StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                               - horizontalDistributionButtons.implicitWidth
            }

            Row {
                spacing: -StudioTheme.Values.border

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeTop
                    tooltip: qsTr("Distribute top edges.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.Top,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeCenterVertical
                    tooltip: qsTr("Distribute vertical centers.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.CenterV,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeBottom
                    tooltip: qsTr("Distribute bottom edges.")
                    onClicked: alignDistribute.distributeObjects(AlignDistribute.Bottom,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText)
                }
            }
        }

        PropertyLabel { text: qsTr("Distribute spacing") }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            Row {
                spacing: -StudioTheme.Values.border

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeSpacingHorizontal
                    tooltip: qsTr("Distribute spacing horizontally.")
                    onClicked: alignDistribute.distributeSpacing(AlignDistribute.X,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText,
                                                                 distanceSpinBox.value,
                                                                 buttonRow.getDistributeDirection())
                }

                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeSpacingVertical
                    tooltip: qsTr("Distribute spacing vertically.")
                    onClicked: alignDistribute.distributeSpacing(AlignDistribute.Y,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText,
                                                                 distanceSpinBox.value,
                                                                 buttonRow.getDistributeDirection())
                }
            }

            Spacer { implicitWidth: 8 }

            StudioControls.ButtonRow {
                id: buttonRow
                actionIndicatorVisible: false

                StudioControls.ButtonGroup {
                    id: group
                }

                function getDistributeDirection() {
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
                    tooltip: qsTr("Disables the distribution of spacing in pixels.")
                }

                AbstractButton {
                    id: buttonLeftToRight
                    buttonIcon: StudioTheme.Constants.distributeOriginTopLeft
                    checkable: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: qsTr("Sets the left or top border of the target area or item as the " +
                                  "starting point, depending on the distribution orientation.")
                }

                AbstractButton {
                    id: buttonCenter
                    buttonIcon: StudioTheme.Constants.distributeOriginCenter
                    checkable: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: qsTr("Sets the horizontal or vertical center of the target area or " +
                                  "item as the starting point, depending on the distribution " +
                                  "orientation.")
                }

                AbstractButton {
                    id: buttonRightToLeft
                    buttonIcon: StudioTheme.Constants.distributeOriginBottomRight
                    checkable: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: qsTr("Sets the bottom or right border of the target area or item as " +
                                  "the starting point, depending on the distribution orientation.")
                }
            }
        }

        PropertyLabel { text: qsTr("Pixel spacing") }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            DoubleSpinBox {
                id: distanceSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                stepSize: 1
                minimumValue: -1000
                maximumValue: 1000
                decimals: 0
                enabled: !buttonNone.checked
            }
        }

        ItemFilterModel {
            id: itemFilterModel
            modelNodeBackendProperty: modelNodeBackend
            selectionOnly: true
        }

        PropertyLabel { text: qsTr("Align to") }

        SecondColumnLayout {
            ComboBox {
                id: alignToComboBox

                property int currentEnum: alignTargets.get(alignToComboBox.currentIndex).value

                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                textRole: "text"
                model: ListModel {
                    id: alignTargets
                    ListElement { text: "Selection"; value: AlignDistribute.Selection }
                    ListElement { text: "Root"; value: AlignDistribute.Root }
                    ListElement { text: "Key object"; value: AlignDistribute.KeyObject }
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Key object") }

        SecondColumnLayout {
            ComboBox {
                id: keyObjectComboBox

                property string lastSelectedItem: ""

                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                enabled: alignToComboBox.currentIndex === 2
                model: itemFilterModel.itemModel

                onCompressedActivated: lastSelectedItem = keyObjectComboBox.currentText

                onModelChanged: {
                    var idx = model.indexOf(keyObjectComboBox.lastSelectedItem)
                    if (idx !== -1)
                        keyObjectComboBox.currentIndex = idx
                    else
                        lastSelectedItem = "" // TODO
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Warning")
            visible: root.warningVisible
            font.weight: Font.Bold
        }

        SecondColumnLayout {
            visible: root.warningVisible

            Spacer {
                implicitWidth: StudioTheme.Values.actionIndicatorWidth
            }

            Column {
                Text {
                    id: warningRoot
                    visible: alignDistribute.selectionContainsRootItem
                    Layout.fillWidth: true
                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: StudioTheme.Values.myFontSize
                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                    text: qsTr("- The selection contains the root component.")
                }

                Text {
                    id: warningNonVisual
                    visible: !alignDistribute.selectionExclusivlyItems
                    Layout.fillWidth: true
                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: StudioTheme.Values.myFontSize
                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                    text: qsTr("- The selection contains a non-visual component.")
                }

                Text {
                    id: warningAnchors
                    visible: alignDistribute.selectionHasAnchors
                    Layout.fillWidth: true
                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: StudioTheme.Values.myFontSize
                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                    text: qsTr("- A component in the selection uses anchors.")
                }
            }
        }
    }
}
