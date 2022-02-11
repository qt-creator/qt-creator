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
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property string tooltip: ""

    signal clicked()

    implicitWidth: 29
    implicitHeight: 29
    color: mouseArea.containsMouse && enabled
           ? StudioTheme.Values.themeControlBackgroundHover
           : StudioTheme.Values.themeControlBackground

    Behavior on color {
        ColorAnimation {
            duration: StudioTheme.Values.hoverDuration
            easing.type: StudioTheme.Values.hoverEasing
        }
    }

    Label { // + sign
        text: StudioTheme.Constants.plus
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        color: root.enabled ? StudioTheme.Values.themeIconColor
                            : StudioTheme.Values.themeIconColorDisabled
        scale: mouseArea.containsMouse ? 1.4 : 1

        Behavior on scale {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutExpo
            }
        }
    }

    HelperWidgets.ToolTipArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
        tooltip: root.tooltip
    }
}
