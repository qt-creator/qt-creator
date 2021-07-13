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
