/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import StudioTheme 1.0 as StudioTheme

Section {
    id: root

    property bool blockedByContext: backendValues.display.enumeration === "TextOnly"

    caption: qsTr("Icon")
    width: parent.width

    SectionLayout {
        // We deliberately kept the "name" property out as it is only properly supported by linux
        // based operating systems out of the box.

        PropertyLabel {
            text: qsTr("Source")
            blockedByTemplate: !backendValues.icon_source.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.icon_source
                enabled: backendValues.icon_source.isAvailable && !root.blockedByContext
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Color")
            blockedByTemplate: !backendValues.icon_color.isAvailable
            enabled: !root.blockedByContext
        }

        ColorEditor {
            backendValue: backendValues.icon_color
            supportGradient: false
            enabled: backendValues.icon_color.isAvailable && !root.blockedByContext
        }

        PropertyLabel {
            text: qsTr("Size")
            blockedByTemplate: !backendValues.icon_width.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: backendValues.icon_width.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: qsTr("Width")
                enabled: backendValues.icon_width.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: backendValues.icon_height.isAvailable && !root.blockedByContext
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Height")
                enabled: backendValues.icon_height.isAvailable && !root.blockedByContext
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cache")
            tooltip: qsTr("Whether the icon should be cached.")
            blockedByTemplate: !backendValues.icon_cache.isAvailable
            enabled: !root.blockedByContext
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.icon_cache.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.icon_cache
                enabled: backendValues.icon_cache.isAvailable && !root.blockedByContext
            }

            ExpandingSpacer {}
        }

    }
}
