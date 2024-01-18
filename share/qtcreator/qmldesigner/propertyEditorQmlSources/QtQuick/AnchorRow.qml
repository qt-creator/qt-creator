// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

SectionLayout {
    id: anchorRow

    opacity: enabled ? 1 : 0.5

    property variant anchorMargin

    property alias iconSource: iconLabel.icon
    property alias targetName: targetComboBox.targetName
    property alias currentText: targetComboBox.currentText
    property alias relativeTarget: buttonRow.relativeTarget
    property alias buttonRow: buttonRow

    property bool verticalAnchor
    property bool invertRelativeTargets: false
    property bool showAlternativeTargets: true

    signal targetChanged
    signal sameEdgeButtonClicked
    signal centerButtonClicked
    signal oppositeEdgeButtonClicked


    PropertyLabel { text: qsTr("Target") }

    SecondColumnLayout {
        IconLabel {
            id: iconLabel
            implicitWidth: StudioTheme.Values.actionIndicatorWidth
            horizontalAlignment: Text.AlignLeft
        }

        TargetComboBox {
            id: targetComboBox

            implicitWidth: StudioTheme.Values.singleControlColumnWidth
            onCurrentTextChanged: anchorRow.targetChanged()
        }

        ExpandingSpacer {}
    }

    PropertyLabel { text: qsTr("Margin") }

    SecondColumnLayout {
        SpinBox {
            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
            minimumValue: -0xffff
            maximumValue: 0xffff
            backendValue: anchorMargin
            realDragRange: 5000
        }

        Spacer {
            implicitWidth: StudioTheme.Values.twoControlColumnGap
                           + StudioTheme.Values.actionIndicatorWidth
                           + (StudioTheme.Values.twoControlColumnWidth - buttonRow.width)
        }

        StudioControls.ButtonRow {
            id: buttonRow

            property variant relativeTarget: anchorBackend.relativeAnchorTargetTop

            actionIndicatorVisible: false

            onRelativeTargetChanged: {
                buttonSameEdge.checked = false
                buttonCenter.checked = false
                buttonOppositeEdge.checked = false

                if (relativeTarget === AnchorBindingProxy.SameEdge) {
                    if (!invertRelativeTargets) {
                        buttonSameEdge.checked = true
                    } else {
                        buttonOppositeEdge.checked = true
                    }
                } else if (relativeTarget === AnchorBindingProxy.OppositeEdge) {
                    if (!invertRelativeTargets) {
                        buttonOppositeEdge.checked = true
                    } else {
                        buttonSameEdge.checked = true
                    }
                } else if (relativeTarget === AnchorBindingProxy.Center) {
                    buttonCenter.checked = true
                }
            }

            StudioControls.ButtonGroup { id: group }

            AbstractButton {
                id: buttonSameEdge
                buttonIcon: verticalAnchor ? StudioTheme.Constants.anchorTop
                                           : StudioTheme.Constants.anchorLeft
                checkable: true
                autoExclusive: true
                StudioControls.ButtonGroup.group: group
                tooltip: verticalAnchor ? qsTr("Anchor to the top of the target.")
                                        : qsTr("Anchor to the left of the target.")
                onClicked: {
                    if (!invertRelativeTargets)
                        sameEdgeButtonClicked()
                    else
                        oppositeEdgeButtonClicked()
                }
            }

            AbstractButton {
                id: buttonCenter
                buttonIcon: verticalAnchor ? StudioTheme.Constants.centerVertical
                                           : StudioTheme.Constants.centerHorizontal
                checkable: true
                autoExclusive: true
                StudioControls.ButtonGroup.group: group
                tooltip: verticalAnchor ? qsTr("Anchor to the vertical center of the target.")
                                        : qsTr("Anchor to the horizontal center of the target.")
                onClicked: centerButtonClicked()
            }

            AbstractButton {
                id: buttonOppositeEdge
                buttonIcon: verticalAnchor ? StudioTheme.Constants.anchorBottom
                                           : StudioTheme.Constants.anchorRight
                checkable: true
                autoExclusive: true
                StudioControls.ButtonGroup.group: group
                tooltip: verticalAnchor ? qsTr("Anchor to the bottom of the target.")
                                        : qsTr("Anchor to the right of the target.")
                onClicked: {
                    if (!invertRelativeTargets)
                        oppositeEdgeButtonClicked()
                    else
                        sameEdgeButtonClicked()
                }
            }
        }

        ExpandingSpacer {}
    }
}
