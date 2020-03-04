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
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12
import QtQuickDesignerTheme 1.0
import StudioTheme 1.0 as StudioTheme

Label {
    id: label

    property alias tooltip: toolTipArea.tooltip
    // workaround because PictureSpecifics.qml still use this
    property alias toolTip: toolTipArea.tooltip

    width: Math.max(Math.min(240, parent.width - 280), 50)
    color: label.disabledState ? StudioTheme.Values.themeDisabledTextColor : StudioTheme.Values.themeTextColor

    elide: Text.ElideRight

    font.pixelSize: StudioTheme.Values.myFontSize

    Layout.preferredWidth: width
    Layout.minimumWidth: width
    Layout.maximumWidth: width

    leftPadding: label.disabledState ? 10 : 0
    rightPadding: label.disabledState ? 10 : 0

    property bool disabledState: false

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
        tooltip:  label.disabledState ? qsTr("This property is not available in this configuration.") : label.text
    }
}
