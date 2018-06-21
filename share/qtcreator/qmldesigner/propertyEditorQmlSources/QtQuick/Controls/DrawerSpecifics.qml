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

    PopupSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Drawer")

        SectionLayout {
            Label {
                text: qsTr("Edge")
                tooltip: qsTr("Defines the edge of the window the drawer will open from.")
            }

            SecondColumnLayout {
                ComboBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.edge
                    scope: "Qt"
                    model: ["TopEdge", "LeftEdge", "RightEdge", "BottomEdge"]
                }
            }

            Label {
                text: qsTr("Drag Margin")
                tooltip: qsTr("Defines the distance from the screen edge within which drag actions will open the drawer.")
            }
            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.dragMargin
                    hasSlider: true
                    Layout.preferredWidth: 80
                    minimumValue: 0
                    maximumValue: 400
                    stepSize: 1
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }
        }
    }

    MarginSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    PaddingSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    FontSection {
    }
}
