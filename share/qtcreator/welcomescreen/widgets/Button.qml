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
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Button {
    id: button
    style: touchStyle
    iconSource: ""
    Component {
        id: touchStyle
        ButtonStyle {
            padding.left: button.iconSource != "" ? 38 : 14
            padding.right: 14
            background: Item {
                anchors.fill: parent
                implicitWidth: 160
                implicitHeight: 30

                Image {
                    id: icon
                    x: 11
                    y: 8
                    z: 1
                    height: 16
                    width: 16
                    source: button.iconSource
                    visible: button.iconSource != ""
                }

                Rectangle {
                    id: rectangle
                    anchors.fill: parent
                    color: (button.checked || button.pressed)
                           ? creatorTheme.Welcome_ForegroundPrimaryColor
                           : (button.hovered
                              ? creatorTheme.Welcome_HoverColor
                              : creatorTheme.Welcome_ButtonBackgroundColor)
                    border.width: 1
                    border.color: (button.checked || button.pressed)
                                  ? creatorTheme.Welcome_ForegroundPrimaryColor
                                  : creatorTheme.Welcome_ForegroundSecondaryColor
                    radius: (creatorTheme.WidgetStyle === 'StyleFlat') ? 0 : 4
                }
            }

            label: Text {
                id: text
                renderType: Text.NativeRendering
                verticalAlignment: Text.AlignVCenter
                text: button.text
                color: (button.checked || button.pressed)
                       ? creatorTheme.Welcome_BackgroundColor
                       : creatorTheme.Welcome_TextColor
                font.pixelSize: 15
                font.bold: false
                smooth: true
            }
        }
    }
}
