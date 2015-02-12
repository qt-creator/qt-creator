/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
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
