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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1

Item {
    id: flickable

    width: 640
    height: 420
    property alias fontComboBox: fontComboBox
    property alias rowLayout1: rowLayout1
    property alias button2: button2
    property alias editableCombo: editableCombo

    property int tabPosition: tabPositionGroup.current === r2 ? Qt.BottomEdge : Qt.TopEdge

    RowLayout {
        id: contentRow
        anchors.fill:parent
        anchors.margins: 8
        spacing: 16
        ColumnLayout {
            id: firstColumn
            Layout.minimumWidth: implicitWidth
            Layout.fillWidth: false
            RowLayout {
                id: buttonrow

                Button {
                    id: button1
                    text: "Button 1"
                    tooltip:"This is an interesting tool tip"
                    Layout.fillWidth: true
                }

                Button {
                    id:button2
                    text:"Button 2"
                    Layout.fillWidth: true

                }
            }
            ComboBox {
                id: combo
                model: choices
                currentIndex: 2
                Layout.fillWidth: true
            }
            ComboBox {
                id: fontComboBox
                Layout.fillWidth: true
                currentIndex: 47
            }
            ComboBox {
                id: editableCombo
                editable: true
                model: choices
                Layout.fillWidth: true
                currentIndex: 2
            }
            RowLayout {
                SpinBox {
                    id: t1
                    Layout.fillWidth: true
                    minimumValue: -50
                    value: -20
                }
                SpinBox {
                    id: t2
                    Layout.fillWidth: true
                }
            }
            TextField {
                id: t3
                placeholderText: "This is a placeholder for a TextField"
                Layout.fillWidth: true
            }
            ProgressBar {
                // normalize value [0.0 .. 1.0]
                value: (slider.value - slider.minimumValue) / (slider.maximumValue - slider.minimumValue)
                Layout.fillWidth: true
            }
            ProgressBar {
                indeterminate: true
                Layout.fillWidth: true
            }
            Slider {
                id: slider
                value: 0.5
                Layout.fillWidth: true
                tickmarksEnabled: tickmarkCheck.checked
                stepSize: tickmarksEnabled ? 0.1 : 0
            }
            MouseArea {
                id: busyCheck
                Layout.fillWidth: true
                Layout.fillHeight: true
                hoverEnabled:true
                Layout.preferredHeight: busyIndicator.height
                BusyIndicator {
                    id: busyIndicator
                    running: busyCheck.containsMouse
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
        ColumnLayout {
            id: rightcol
            Layout.fillWidth: true
            anchors {
                top: parent.top
                bottom: parent.bottom
            }

            GroupBox {
                id: group1
                title: "CheckBox"
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        id: frameCheckbox
                        text: "Text frame"
                        checked: true
                        Layout.minimumWidth: 100
                    }
                    CheckBox {
                        id: tickmarkCheck
                        text: "Tickmarks"
                        checked: false
                        Layout.minimumWidth: 100
                    }
                    CheckBox {
                        id: wrapCheck
                        text: "Word wrap"
                        checked: true
                        Layout.minimumWidth: 100
                    }
                }
            }
            GroupBox {
                id: group2
                title:"Tab Position"
                Layout.fillWidth: true
                RowLayout {
                    id: rowLayout1

                    RadioButton {
                        id: r1
                        text: "Top"
                        checked: true
                        exclusiveGroup: tabPositionGroup
                        Layout.minimumWidth: 100
                    }
                    RadioButton {
                        id: r2
                        text: "Bottom"
                        exclusiveGroup: tabPositionGroup
                        Layout.minimumWidth: 100
                    }
                }
            }

            TextArea {
                id: area
                frameVisible: frameCheckbox.checked
                text: loremIpsum + loremIpsum
                textFormat: Qt.RichText
                wrapMode: wrapCheck.checked ? TextEdit.WordWrap : TextEdit.NoWrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                //menu: editmenu
            }
        }
    }
}
