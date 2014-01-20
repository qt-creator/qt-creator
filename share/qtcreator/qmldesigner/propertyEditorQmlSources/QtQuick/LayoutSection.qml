/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
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

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Layout")

    ColumnLayout {
        width: parent.width
        Label {
            text: qsTr("Anchors")
        }

        AnchorButtons {
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            enabled: anchorBackend.topAnchored;
            opacity: enabled ? 1 : 0.5

            IconLabel {
                source:  "../HelperWidgets/images/anchor-top.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.topTarget
                    onCurrentTextChanged: {
                        anchorBackend.topTarget = currentText
                    }
                }

                Text {
                    text: "Margin"
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_topMargin
                    }

//                    ButtonRow {
//                        exclusive: true
//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-top.png"

//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-bottom.png"
//                        }
//                    }
                }
            }
        }

        RowLayout {
            enabled: anchorBackend.bottomAnchored;
            opacity: enabled ? 1 : 0.5
            anchors.left: parent.left
            anchors.right: parent.right

            IconLabel {
                source:  "../HelperWidgets/images/anchor-bottom.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.bottomTarget
                    onCurrentTextChanged: {
                        anchorBackend.bottomTarget = currentText
                    }
                }

                Text {
                    text: qsTr("Margin")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_bottomMargin

                    }

//                    ButtonRow {
//                        exclusive: true
//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-top.png"
//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-bottom.png"
//                        }
//                    }
                }
            }
        }

        RowLayout {
            enabled: anchorBackend.leftAnchored;
            opacity: enabled ? 1 : 0.5
            anchors.left: parent.left
            anchors.right: parent.right

            IconLabel {
                source:  "../HelperWidgets/images/anchor-left.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.leftTarget
                    onCurrentTextChanged: {
                        anchorBackend.leftTarget = currentText
                    }
                }

                Text {
                    text: qsTr("Margin")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_leftMargin
                    }

//                    ButtonRow {
//                        exclusive: true

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-left.png"
//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-right.png"
//                        }
//                    }
                }
            }
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            enabled: anchorBackend.rightAnchored;
            opacity: enabled ? 1 : 0.5

            IconLabel {
                source:  "../HelperWidgets/images/anchor-right.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.rightTarget
                    onCurrentTextChanged: {
                        anchorBackend.rightTarget = currentText
                    }
                }

                Text {
                    text: qsTr("Margin")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_rightMargin
                    }

//                    ButtonRow {
//                        exclusive: true

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-left.png"
//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-right.png"
//                        }
//                    }
                }
            }
        }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            enabled: anchorBackend.horizontalCentered;
            opacity: enabled ? 1 : 0.5

            IconLabel {
                source:  "../HelperWidgets/images/anchor-horizontal.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.horizontalTarget
                    onCurrentTextChanged: {
                        anchorBackend.horizontalTarget = currentText
                    }
                }

                Text {
                    text: qsTr("Margin")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_horizontalCenterOffset
                    }

//                    ButtonRow {
//                        exclusive: true

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-left.png"
//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-right.png"
//                        }
//                    }
                }
            }
        }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            enabled: anchorBackend.verticalCentered;
            opacity: enabled ? 1 : 0.5

            IconLabel {
                source:  "../HelperWidgets/images/anchor-vertical.png"
                Layout.alignment: Qt.AlignTop
            }

            GridLayout {
                Layout.fillWidth: true
                rows: 2
                columns: 2

                Text {
                    text: qsTr("Target")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                TargetComboBox {
                    targetName: anchorBackend.verticalTarget
                    onCurrentTextChanged: {
                        anchorBackend.verticalTarget = currentText
                    }
                }

                Text {
                    text: qsTr("Margin")
                    color: "#eee"
                    elide: Text.ElideRight
                    Layout.minimumWidth: 40
                }

                RowLayout {
                    SpinBox {
                        maximumValue: 0xffff
                        minimumValue: -0xffff
                        backendValue: backendValues.anchors_verticalCenterOffset
                    }

//                    ButtonRow {
//                        exclusive: true

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-top.png"
//                        }

//                        ButtonRowButton {
//                            iconSource: "../HelperWidgets/images/anchor-bottom.png"
//                        }
//                    }
                }
            }
        }
    }
}
