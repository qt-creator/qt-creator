/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2
import QtQuickDesignerTheme 1.0

Rectangle {
    id: root

    width: 640
    height: 200
    color: Theme.color(Theme.BackgroundColorDark)

    property string bezierCurve

    onBezierCurveChanged: bezierCanvas.bezierCurve = bezierCurve

    signal okClicked
    signal cancelClicked

    focus: true

    Keys.onPressed: {
        if (event.key === Qt.Key_Escape)
            root.cancelClicked()
        else if (event.key === Qt.Key_Return)
            root.okClicked()
    }

    TextInput {
        id: textEdit

        anchors.left: parent.left
        anchors.bottom: parent.bottom
        color: Theme.color(Theme.PanelTextColorLight)
        horizontalAlignment: Text.AlignHCenter
        anchors.bottomMargin: 2
        anchors.leftMargin: 10
        onEditingFinished: bezierCanvas.bezierCurve = text
    }

    RowLayout {
        id: contentLayout
        anchors.fill: parent

        anchors.rightMargin: 10
        anchors.leftMargin: 8
        anchors.topMargin: 10
        anchors.bottomMargin: 20

        Item {
            Layout.preferredWidth: 215
            Layout.preferredHeight: 220

            Layout.alignment: Qt.AlignLeft | Qt.AlignTop

            BezierCanvas {
                id: bezierCanvas
                scale: 2
                transformOrigin: Item.TopLeft
                bezierLineWidth: 1
                handleLineWidth: 0.8
                onBezierCurveChanged: {
                    textEdit.text = bezierCanvas.bezierCurve
                    root.bezierCurve = bezierCanvas.bezierCurve
                }
            }
        }


        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.bottomMargin: 10
            Layout.leftMargin: 5

            color: Theme.qmlDesignerBackgroundColorDarker()

            ScrollView {
                anchors.fill: parent

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                GridView {
                    interactive: true

                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    anchors.rightMargin: 2

                    clip: true

                    ScrollBar.vertical: ScrollBar {
                        x: 1
                        width: 10
                        readonly property color scrollbarColor: Theme.color(Theme.BackgroundColorDark)
                        readonly property color scrollBarHandleColor: Theme.qmlDesignerButtonColor()
                        contentItem: Item {
                            width: 4
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 0
                                color: Theme.qmlDesignerButtonColor()
                            }
                        }
                        background: Rectangle {
                            color: Theme.color(Theme.BackgroundColorDark)
                            border.width: 2
                            border.color: Theme.qmlDesignerBackgroundColorDarker()
                        }
                    }

                    model: ListModel {

                        ListElement {
                            name: "linear"
                            curve: "0.250, 0.250, 0.750, 0.750"
                        }

                        ListElement {
                            name: "ease in out"
                            curve: "0.420, 0.000, 0.580, 1.000"
                        }

                        ListElement {
                            name: "ease"
                            curve: "0.250, 0.100, 0.250, 1.00"
                        }

                        ListElement {
                            name: "ease in"
                            curve: "0.420, 0.00, 1.00, 1.00"
                        }

                        ListElement {
                            name: "ease out"
                            curve: "0.00, 0.00, 0.580, 1.00"
                        }

                        ListElement {
                            name: "quad in out"
                            curve: "0.455, 0.030, 0.515, 0.955"
                        }

                        ListElement {
                            name: "quad in"
                            curve: "0.550, 0.085, 0.680, 0.530"
                        }

                        ListElement {
                            name: "quad out"
                            curve: "0.250, 0.460, 0.450, 0.940"
                        }

                        ListElement {
                            name: "quad ease"
                            curve: "0.550, 0.085, 0.680, 0.530"
                        }

                        ListElement {
                            name: "cubic in out"
                            curve: "0.645, 0.045, 0.355, 1.00"
                        }

                        ListElement {
                            name: "cubic in"
                            curve: "0.550, 0.055, 0.675, 0.190"
                        }

                        ListElement {
                            name: "cubic out"
                            curve: "0.215, 0.610, 0.355, 1.00"
                        }

                        ListElement {
                            name: "quart in out"
                            curve: "0.770, 0.000, 0.175, 1.00"
                        }

                        ListElement {
                            name: "quart in"
                            curve: "0.895, 0.030, 0.685, 0.220"
                        }

                        ListElement {
                            name: "quart out"
                            curve: "0.165, 0.840, 0.440, 1.00"
                        }

                        ListElement {
                            name: "quint in out"
                            curve: "0.860, 0.000, 0.070, 1.00"
                        }

                        ListElement {
                            name: "quint in"
                            curve: "0.755, 0.050, 0.855, 0.060"
                        }

                        ListElement {
                            name: "quint out"
                            curve: "0.230, 1.000, 0.320, 1.00"
                        }

                        ListElement {
                            name: "expo in out"
                            curve: "1.000, 0.000, 0.000, 1.000"
                        }

                        ListElement {
                            name: "expo in"
                            curve: "0.950, 0.050, 0.795, 0.035"
                        }

                        ListElement {
                            name: "expo out"
                            curve: "0.190, 1.000, 0.220, 1.000"
                        }

                        ListElement {
                            name: "circ in out"
                            curve: "0.785, 0.135, 0.150, 0.860"
                        }

                        ListElement {
                            name: "circ in"
                            curve: "0.600, 0.040, 0.980, 0.335"
                        }

                        ListElement {
                            name: "circ out"
                            curve: "0.075, 0.820, 0.165, 1.00"
                        }

                        ListElement {
                            name: "back in out"
                            curve: "0.680, -0.550, 0.265, 1.550"
                        }

                        ListElement {
                            name: "back in"
                            curve: "0.600, -0.280, 0.735, 0.045"
                        }

                        ListElement {
                            name: "back out"
                            curve: "0.175, 0.885, 0.320, 1.275"
                        }

                        ListElement {
                            name: "standard"
                            curve: "0.40,0.00,0.20,1.00"
                        }

                        ListElement {
                            name: "deceleration"
                            curve: "0.00,0.00,0.20,1.00"
                        }

                        ListElement {
                            name: "acceleration"
                            curve: "0.40,0.00,1.00,1.00"
                        }

                        ListElement {
                            name: "sharp"
                            curve: "0.40,0.00,0.60,1.00"
                        }

                        ListElement {
                            name: "sine in out"
                            curve: "0.445, 0.05, 0.55, 0.95"
                        }

                        ListElement {
                            name: "sine in"
                            curve: "0.470, 0.00, 0.745, 0.715"
                        }

                        ListElement {
                            name: "sine out"
                            curve: "0.39, 0.575, 0.565, 1.00"
                        }

                    }

                    cellHeight: 68
                    cellWidth: 68
                    delegate:  Rectangle {
                        width: 64
                        height: 64
                        color: Theme.color(Theme.BackgroundColorDark)

                        Rectangle {
                             color: Theme.qmlDesignerBackgroundColorDarkAlternate()
                             width: 64
                             anchors.bottom: parent.bottom
                             height: 11
                        }

                        BezierCanvas {
                            x: 4
                            y: 0
                            scale: 0.5
                            transformOrigin: Item.TopLeft
                            caption: name
                            bezierCurve: curve
                            interactive: false
                            onClicked: bezierCanvas.bezierCurve = curve
                            color: Theme.color(Theme.BackgroundColorDark)
                        }
                    }
                }

            }
        }
    }

    RowLayout {
        height: 30
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        anchors.rightMargin: 10
        Layout.bottomMargin: 0

        StyledButton {
            text: qsTr("Cancel")
            onClicked: root.cancelClicked()
        }

        StyledButton {
            text: qsTr("Apply")
            onClicked: root.okClicked()
        }
    }
}




