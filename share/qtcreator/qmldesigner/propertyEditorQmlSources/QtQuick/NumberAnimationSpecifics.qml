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

import HelperWidgets 2.0
import QtQuick 2.1
import QtQuick.Layouts 1.1
Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Number Animation")
        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            Label {
                text: qsTr("From")
                tooltip: qsTr("Sets the starting value for the animation.")
            }

            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -1
                    backendValue: backendValues.from
                    Layout.fillWidth: true
                    Layout.maximumWidth: 100
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("To")
                tooltip: qsTr("Sets the end value for the animation.")
            }

            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -1
                    backendValue: backendValues.to
                    Layout.fillWidth: true
                    Layout.maximumWidth: 100
                }

                ExpandingSpacer {
                }
            }
        }
    }

    AnimationTargetSection {

    }

    AnimationSection {
        showEasingCurve: true
    }
}

