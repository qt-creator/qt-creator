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
        caption: qsTr("Grid")


        SectionLayout {

            Label {
                text: qsTr("Columns")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.columns
                    minimumValue: 0
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Rows")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.rows
                    minimumValue: 0
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Flow")
            }

            SecondColumnLayout {
                ComboBox {
                    model: ["LeftToRight", "TopToBottom"]
                    backendValue: backendValues.flow
                    Layout.fillWidth: true
                    scope: "Grid"
                }
            }

            Label {
                text: qsTr("Layout Direction")
            }

            SecondColumnLayout {
                ComboBox {
                    model: ["LeftToRight", "RightToLeft"]
                    backendValue: backendValues.layoutDirection
                    Layout.fillWidth: true
                    scope: "Qt"
                }

            }

            Label {
                text: qsTr("Spacing")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.spacing
                    minimumValue: -4000
                    maximumValue: 4000
                    decimals: 0
                }

                ExpandingSpacer {
                }
            }
        }
    }
}
