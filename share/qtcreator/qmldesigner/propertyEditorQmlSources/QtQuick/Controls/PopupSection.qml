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

Section {
    caption: qsTr("Popup")

    SectionLayout {
        Label {
            text: qsTr("Size")
        }

        SecondColumnLayout {
            Label {
                text: "W"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.width
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
            }

            Label {
                text: "H"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.height
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Visibility")
        }

        SecondColumnLayout {

            CheckBox {
                text: qsTr("Is visible")
                backendValue: backendValues.visible
                Layout.preferredWidth: 100
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
            text: qsTr("Behavior")
        }

        SecondColumnLayout {

            CheckBox {
                text: qsTr("Modal")
                backendValue: backendValues.modal
                tooltip: qsTr("Defines the modality of the popup.")

                Layout.preferredWidth: 100
            }

            Item {
                width: 10
                height: 10
            }

            CheckBox {
                text: qsTr("Dim")
                tooltip: qsTr("Defines whether the popup dims the background.")
                backendValue: backendValues.dim
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
                backendValue: backendValues.opacity
                hasSlider: true
                Layout.preferredWidth: 80
                minimumValue: 0
                maximumValue: 1
                stepSize: 0.1
                decimals: 2
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Scale")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.scale
                hasSlider: true
                Layout.preferredWidth: 80
                minimumValue: 0
                maximumValue: 1
                stepSize: 0.1
                decimals: 2
            }
            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Spacing")
            tooltip: qsTr("Spacing between internal elements of the control.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.spacing
                Layout.fillWidth: true
            }
        }
    }
}
