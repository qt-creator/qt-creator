/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    ImageSection {
        caption: qsTr("Image")
    }

    Section {
        caption: qsTr("Animated image")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Speed")
                blockedByTemplate: !backendValues.speed.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    sliderIndicatorVisible: true
                    backendValue: backendValues.speed
                    hasSlider: true
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 100
                    enabled: backendValues.speed.isAvailable
                }

                // TODO convert to % and add % label after the spin box

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Playing")
                tooltip: qsTr("Whether the animation is playing or paused.")
                blockedByTemplate: !backendValues.playing.isAvailable && !backendValues.paused.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: StudioTheme.Constants.play
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.playing
                    enabled: backendValue.isAvailable
                    fontFamily: StudioTheme.Constants.iconFont.family
                    fontPixelSize: StudioTheme.Values.myIconFontSize
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: StudioTheme.Constants.pause
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.paused
                    enabled: backendValue.isAvailable
                    fontFamily: StudioTheme.Constants.iconFont.family
                    fontPixelSize: StudioTheme.Values.myIconFontSize
                }

                ExpandingSpacer {}
            }
        }
    }
}
