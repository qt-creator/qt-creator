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

    ImageSpecifics {
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Animated Image")

        SectionLayout {
            Label {
                text: qsTr("Speed")
                disabledState: !backendValues.speed.isAvailable
            }
            SecondColumnLayout {
                SpinBox {
                    sliderIndicatorVisible: true
                    backendValue: backendValues.speed
                    hasSlider: true
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 100
                    Layout.preferredWidth: 140
                    enabled: backendValues.speed.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Paused")
                tooltip: qsTr("Holds whether the animated image is paused.")
                disabledState: !backendValues.paused.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.paused.isAvailable
                    text: backendValues.paused.valueToString
                    backendValue: backendValues.paused
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }

            Label {
                text: qsTr("Playing")
                tooltip: qsTr("Holds whether the animated image is playing.")
                disabledState: !backendValues.playing.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.playing.isAvailable
                    text: backendValues.playing.valueToString
                    backendValue: backendValues.playing
                    implicitWidth: 180
                }
                ExpandingSpacer {}
            }
        }
    }
}
