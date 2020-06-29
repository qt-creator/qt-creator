/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
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

        Label {
            text: qsTr("Distribute objects")
            width: 120
        }
        RowLayout {
            Row {
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

        Label {
            text: qsTr("Distribute spacing")
            width: 120
        }
        RowLayout {
            Row {
                spacing: -StudioTheme.Values.border
                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeSpacingHorizontal
                    tooltip: qsTr("Distribute spacing horizontally.")
                    onClicked: alignDistribute.distributeSpacing(AlignDistribute.X,
                                                                 alignToComboBox.currentEnum,
                                                                 keyObjectComboBox.currentText,
                                                                 distanceSpinBox.realValue,
                                                                 buttonRow.getDistributeDirection())
                }
                AbstractButton {
                    buttonIcon: StudioTheme.Constants.distributeSpacingVertical
                    tooltip: qsTr("Distribute spacing vertically.")
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
