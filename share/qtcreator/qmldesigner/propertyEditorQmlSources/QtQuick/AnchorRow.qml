/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

RowLayout {
    id: anchorRow

    anchors.left: parent.left
    anchors.right: parent.right
    opacity: enabled ? 1 : 0.5

    property alias iconSource: icon.source

    property variant anchorMargin

    property alias targetName: targetComboBox.targetName
    property alias currentText: targetComboBox.currentText

    property alias relativeTarget: buttonRow.relativeTarget

    signal targetChanged

    property bool verticalAnchor: true

    property bool invertRelativeTargets: false

    property bool showAlternativeTargets: true

    signal sameEdgeButtonClicked
    signal centerButtonClicked
    signal oppositeEdgeButtonClicked


    IconLabel {
        id: icon
        Layout.alignment: Qt.AlignTop
    }

    GridLayout {
        Layout.fillWidth: true
        rows: 2
        columns: 2

        Controls.Label {
            text: qsTr("Target")
            color: "#eee"
            elide: Text.ElideRight
            Layout.minimumWidth: Math.min(60, preferredWidth + 10)
        }

        TargetComboBox {
            id: targetComboBox
            onCurrentTextChanged: {
                anchorRow.targetChanged();
            }
        }

        Controls.Label {
            text: "Margin"
            color: "#eee"
            elide: Text.ElideRight
            Layout.minimumWidth: Math.min(60, preferredWidth + 10)
        }

        RowLayout {
            SpinBox {
                maximumValue: 0xffff
                minimumValue: -0xffff
                backendValue: anchorMargin
            }

            ButtonRow {
                id: buttonRow

                property variant relativeTarget: anchorBackend.relativeAnchorTargetTop

                onRelativeTargetChanged: {
                    if (relativeTarget == AnchorBindingProxy.SameEdge) {
                        if (!invertRelativeTargets) {
                            buttonRow.initalChecked = 0
                            buttonRow.checkedIndex = 0
                        } else {
                            buttonRow.initalChecked = 2
                            buttonRow.checkedIndex = 2
                        }
                    } else if (relativeTarget == AnchorBindingProxy.OppositeEdge) {
                        if (!invertRelativeTargets) {
                            buttonRow.initalChecked = 2
                            buttonRow.checkedIndex = 2
                        } else {
                            buttonRow.initalChecked = 0
                            buttonRow.checkedIndex = 0
                        }
                    } else if (relativeTarget == AnchorBindingProxy.Center) {
                        buttonRow.initalChecked = 1
                        buttonRow.checkedIndex = 1
                    }
                }

                exclusive: true
                ButtonRowButton {
                    iconSource: verticalAnchor ? "../HelperWidgets/images/anchor-top.png" : "../HelperWidgets/images/anchor-left.png"
                    onClicked: {
                        if (!invertRelativeTargets)
                            sameEdgeButtonClicked();
                        else
                            oppositeEdgeButtonClicked();
                    }
                }

                ButtonRowButton {
                    iconSource: verticalAnchor ? "../HelperWidgets/images/anchor-vertical.png" : "../HelperWidgets/images/anchor-horizontal.png"

                    onClicked: centerButtonClicked();
                }

                ButtonRowButton {
                    iconSource: verticalAnchor ? "../HelperWidgets/images/anchor-bottom.png" : "../HelperWidgets/images/anchor-right.png"
                    onClicked: {
                        if (!invertRelativeTargets)
                            oppositeEdgeButtonClicked();
                        else
                            sameEdgeButtonClicked();
                    }
                }
            }
        }
    }
}
