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
    readonly property string actionIconBinding: "\u0022"
    readonly property string addFile: "\u0023"
    readonly property string alignBottom: "\u0024"
    readonly property string alignCenterHorizontal: "\u0025"
    readonly property string alignCenterVertical: "\u0026"
    readonly property string alignLeft: "\u0027"
    readonly property string alignRight: "\u0028"
    readonly property string alignTo: "\u0029"
    readonly property string alignTop: "\u002A"
    readonly property string anchorBaseline: "\u002B"
    readonly property string anchorBottom: "\u002C"
    readonly property string anchorFill: "\u002D"
    readonly property string anchorLeft: "\u002E"
    readonly property string anchorRight: "\u002F"
    readonly property string anchorTop: "\u0030"
    readonly property string centerHorizontal: "\u0031"
    readonly property string centerVertical: "\u0032"
    readonly property string closeCross: "\u0033"
    readonly property string distributeBottom: "\u0034"
    readonly property string distributeCenterHorizontal: "\u0035"
    readonly property string distributeCenterVertical: "\u0036"
    readonly property string distributeLeft: "\u0037"
    readonly property string distributeOriginBottomRight: "\u0038"
    readonly property string distributeOriginCenter: "\u0039"
    readonly property string distributeOriginNone: "\u003A"
    readonly property string distributeOriginTopLeft: "\u003B"
    readonly property string distributeRight: "\u003C"
    readonly property string distributeSpacingHorizontal: "\u003D"
    readonly property string distributeSpacingVertical: "\u003E"
    readonly property string distributeTop: "\u003F"
    readonly property string fontStyleBold: "\u0040"
    readonly property string fontStyleItalic: "\u0041"
    readonly property string fontStyleStrikethrough: "\u0042"
    readonly property string fontStyleUnderline: "\u0043"
    readonly property string testIcon: "\u0044"
    readonly property string textAlignBottom: "\u0045"
    readonly property string textAlignCenter: "\u0046"
    readonly property string textAlignLeft: "\u0047"
    readonly property string textAlignMiddle: "\u0048"
    readonly property string textAlignRight: "\u0049"
    readonly property string textAlignTop: "\u004A"
    readonly property string tickIcon: "\u004B"
    readonly property string triState: "\u004C"
    readonly property string upDownIcon: "\u004D"
    readonly property string upDownSquare2: "\u004E"

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

    readonly property bool showActionIndicatorBackground: false
}
