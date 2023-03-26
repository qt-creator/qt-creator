// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.Label {
    id: label

    property alias tooltip: toolTipArea.tooltip

    property bool blockedByContext: false
    property bool blockedByTemplate: false // MCU

    width: StudioTheme.Values.propertyLabelWidth
    color: StudioTheme.Values.themeTextColor
    font.pixelSize: StudioTheme.Values.myFontSize
    elide: Text.ElideRight

    horizontalAlignment: Text.AlignRight

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        tooltip: label.blockedByTemplate
                    ? qsTr("This property is not available in this configuration.")
                    : label.text
    }

    states: [
        State {
            name: "disabled"
            when: !label.enabled && !(label.blockedByContext || label.blockedByTemplate)
            PropertyChanges {
                target: label
                color: StudioTheme.Values.themeTextColorDisabled
            }
        },
        State {
            name: "blockedByContext"
            when: label.blockedByContext
            PropertyChanges {
                target: label
                color: StudioTheme.Values.themeTextColorDisabled
            }
        },
        State {
            name: "blockedByTemplate"
            when: label.blockedByTemplate
            PropertyChanges {
                target: label
                color: StudioTheme.Values.themeTextColorDisabledMCU
                font.strikeout: true
            }
        }
    ]
}
