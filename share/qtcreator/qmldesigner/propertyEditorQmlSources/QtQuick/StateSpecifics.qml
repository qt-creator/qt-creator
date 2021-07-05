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
        caption: qsTr("State")

        SectionLayout {
            Label {
                text: qsTr("When")
                tooltip: qsTr("Sets when the state should be applied.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.when.valueToString
                    backendValue: backendValues.when
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Name")
                tooltip: qsTr("The name of the state.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.name
                    Layout.fillWidth: true
                    showTranslateCheckBox: false
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Extend")
                tooltip: qsTr("The state that this state extends.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.extend
                    Layout.fillWidth: true
                    showTranslateCheckBox: false
                }
                ExpandingSpacer {}
            }
        }

    }
}
