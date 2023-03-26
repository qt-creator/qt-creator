// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Label {
    id: label

    property alias tooltip: toolTipArea.tooltip
    // workaround because PictureSpecifics.qml still use this
    property alias toolTip: toolTipArea.tooltip

    width: Math.max(Math.min(240, parent.width - 280), 50)
    color: ((label.disabledState || label.disabledStateSoft)
            ? StudioTheme.Values.themeTextColorDisabled
            : StudioTheme.Values.themeTextColor)

    elide: Text.ElideRight

    font.pixelSize: StudioTheme.Values.myFontSize

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width

    leftPadding: label.disabledState ? 10 : 0
    rightPadding: label.disabledState ? 10 : 0

    //Label can be disabled fully (with [] and padding), or in a soft way: only with tooltip and color change.
    property bool disabledState: false
    property bool disabledStateSoft: false

    Text {
        text: "["
        color: StudioTheme.Values.themeTextColor//StudioTheme.Values.themeDisabledTextColor
        visible: label.disabledState
    }

    Text {
        text: "]"
        color: StudioTheme.Values.themeTextColor//StudioTheme.Values.themeDisabledTextColor//
        visible: label.disabledState
        x: label.contentWidth + 10 + contentWidth
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        tooltip: ((label.disabledState || label.disabledStateSoft)
                  ? qsTr("This property is not available in this configuration.")
                  : label.text)
    }
}
