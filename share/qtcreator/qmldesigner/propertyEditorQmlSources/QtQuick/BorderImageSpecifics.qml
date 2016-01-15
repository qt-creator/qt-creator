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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Border Image")

        SectionLayout {
            Label {
                text: qsTr("Source")
            }

            SecondColumnLayout {
                UrlChooser {
                    backendValue: backendValues.source
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Border Left")
            }

            SecondColumnLayout {

                SpinBox {
                    backendValue: backendValues.border_left
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Border Right")
            }

            SecondColumnLayout {

                SpinBox {
                    backendValue: backendValues.border_right
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Border Top")
            }

            SecondColumnLayout {

                SpinBox {
                    backendValue: backendValues.border_top
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Border Bottom")
            }

            SecondColumnLayout {

                SpinBox {
                    backendValue: backendValues.border_bottom
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Horizontal Fill mode")
            }

            SecondColumnLayout {
                ComboBox {
                    model: ["Stretch", "Repeat", "Round"]
                    backendValue: backendValues.horizontalTileMode
                    implicitWidth: 180
                    Layout.fillWidth: true
                    scope: "BorderImage"
                }
            }

            Label {
                text: qsTr("Vertical Fill mode")
            }

            SecondColumnLayout {
                ComboBox {
                    model: ["Stretch", "Repeat", "Round"]
                    backendValue: backendValues.verticalTileMode
                    implicitWidth: 180
                    Layout.fillWidth: true
                    scope: "BorderImage"
                }

            }


            Label {
                text: qsTr("Source size")
            }

            SecondColumnLayout {
                Label {
                    text: "W"
                    width: 12
                }

                SpinBox {
                    backendValue: backendValues.sourceSize_width
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                Label {
                    text: "H"
                    width: 12
                }

                SpinBox {
                    backendValue: backendValues.sourceSize_height
                    minimumValue: -2000
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }
        }
    }
}
