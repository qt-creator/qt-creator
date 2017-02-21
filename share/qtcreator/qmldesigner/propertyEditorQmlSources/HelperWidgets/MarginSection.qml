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
import QtQuick.Controls 1.0 as Controls

Section {
    caption: qsTr("Margin")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        Label {
            text: qsTr("Vertical")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Top")
                tooltip: qsTr("The margin above the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.topMargin
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Bottom")
                tooltip: qsTr("The margin below the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.bottomMargin
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Horizontal")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Left")
                tooltip: qsTr("The margin left of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.leftMargin
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Right")
                tooltip: qsTr("The margin right of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.rightMargin
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Margins")
            tooltip: qsTr("The margins around the item.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.margins
                Layout.fillWidth: true
            }
        }
    }
}
