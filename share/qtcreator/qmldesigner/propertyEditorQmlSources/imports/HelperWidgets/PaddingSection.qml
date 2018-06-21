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
    caption: qsTr("Padding")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        Label {
            text: qsTr("Vertical")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Top")
                tooltip: qsTr("Padding between the content and the top edge of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.topPadding
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Bottom")
                tooltip: qsTr("Padding between the content and the bottom edge of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.bottomPadding
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Horizontal")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Left")
                tooltip: qsTr("Padding between the content and the left edge of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.leftPadding
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Right")
                tooltip: qsTr("Padding between the content and the right edge of the item.")
                width: 42
            }
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.rightPadding
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Padding")
            tooltip: qsTr("Padding between the content and the edges of the items.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.padding
                Layout.fillWidth: true
            }
        }
    }
}
