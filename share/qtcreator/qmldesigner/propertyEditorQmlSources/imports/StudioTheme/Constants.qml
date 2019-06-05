/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

pragma Singleton
import QtQuick 2.10

QtObject {
    readonly property int width: 1920
    readonly property int height: 1080
    readonly property FontLoader mySystemFont: FontLoader {
        name: "Arial"
    }
    readonly property FontLoader controlIcons: FontLoader {
        source: "icons.ttf"
    }

    readonly property string actionIcon: "\u0021"
    readonly property string anchorBottom: "\u0022"
    readonly property string anchorFill: "\u0023"
    readonly property string anchorLeft: "\u0024"
    readonly property string anchorRight: "\u0025"
    readonly property string anchorTop: "\u0026"
    readonly property string centerHorizontal: "\u0027"
    readonly property string centerVertical: "\u0028"
    readonly property string closeCross: "\u0029"
    readonly property string fontStyleBold: "\u002A"
    readonly property string fontStyleItalic: "\u002B"
    readonly property string fontStyleStrikethrough: "\u002C"
    readonly property string fontStyleUnderline: "\u002D"
    readonly property string textAlignCenter: "\u002E"
    readonly property string textAlignLeft: "\u002F"
    readonly property string textAlignRight: "\u0030"
    readonly property string tickIcon: "\u0031"
    readonly property string upDownIcon: "\u0032"
    readonly property string upDownSquare2: "\u0033"

    readonly property font iconFont: Qt.font({
                                                 "family": controlIcons.name,
                                                 "pixelSize": 12
                                             })

    readonly property font font: Qt.font({
                                             "family": mySystemFont.name,
                                             "pointSize": Qt.application.font.pixelSize
                                         })

    readonly property font largeFont: Qt.font({
                                                  "family": mySystemFont.name,
                                                  "pointSize": Qt.application.font.pixelSize * 1.6
                                              })

    readonly property color backgroundColor: "#c2c2c2"
}
