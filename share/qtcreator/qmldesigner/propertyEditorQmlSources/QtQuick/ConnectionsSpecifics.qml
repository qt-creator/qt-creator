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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Connections")

        SectionLayout {
            Label {
                text: qsTr("Enabled")
                tooltip: qsTr("Sets whether the item accepts change events.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.enabled.valueToString
                    backendValue: backendValues.enabled
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Ignore unknown signals")
                tooltip: qsTr("A connection to a non-existent signal produces runtime errors. If this property is set to true, such errors are ignored")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.ignoreUnknownSignals.valueToString
                    backendValue: backendValues.ignoreUnknownSignals
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Target")
                tooltip: qsTr("Sets the object that sends the signal.")
            }
            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.Item"
                    validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                    backendValue: backendValues.target
                    Layout.fillWidth: true
                }
                ExpandingSpacer {}
            }
        }

    }
}
