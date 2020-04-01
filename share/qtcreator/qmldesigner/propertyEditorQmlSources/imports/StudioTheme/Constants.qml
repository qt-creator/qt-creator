/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    readonly property string addColumnAfter: "\u0023"
    readonly property string addColumnBefore: "\u0024"
    readonly property string addFile: "\u0025"
    readonly property string addRowAfter: "\u0026"
    readonly property string addRowBefore: "\u0027"
    readonly property string addTable: "\u0028"
    readonly property string alignBottom: "\u0029"
    readonly property string alignCenterHorizontal: "\u002A"
    readonly property string alignCenterVertical: "\u002B"
    readonly property string alignLeft: "\u002C"
    readonly property string alignRight: "\u002D"
    readonly property string alignTo: "\u002E"
    readonly property string alignTop: "\u002F"
    readonly property string anchorBaseline: "\u0030"
    readonly property string anchorBottom: "\u0031"
    readonly property string anchorFill: "\u0032"
    readonly property string anchorLeft: "\u0033"
    readonly property string anchorRight: "\u0034"
    readonly property string anchorTop: "\u0035"
    readonly property string annotationBubble: "\u0036"
    readonly property string annotationDecal: "\u0037"
    readonly property string centerHorizontal: "\u0038"
    readonly property string centerVertical: "\u0039"
    readonly property string closeCross: "\u003A"
    readonly property string deleteColumn: "\u003B"
    readonly property string deleteRow: "\u003C"
    readonly property string deleteTable: "\u003D"
    readonly property string distributeBottom: "\u003E"
    readonly property string distributeCenterHorizontal: "\u003F"
    readonly property string distributeCenterVertical: "\u0040"
    readonly property string distributeLeft: "\u0041"
    readonly property string distributeOriginBottomRight: "\u0042"
    readonly property string distributeOriginCenter: "\u0043"
    readonly property string distributeOriginNone: "\u0044"
    readonly property string distributeOriginTopLeft: "\u0045"
    readonly property string distributeRight: "\u0046"
    readonly property string distributeSpacingHorizontal: "\u0047"
    readonly property string distributeSpacingVertical: "\u0048"
    readonly property string distributeTop: "\u0049"
    readonly property string edit: "\u004A"
    readonly property string fontStyleBold: "\u004B"
    readonly property string fontStyleItalic: "\u004C"
    readonly property string fontStyleStrikethrough: "\u004D"
    readonly property string fontStyleUnderline: "\u004E"
    readonly property string mergeCells: "\u004F"
    readonly property string redo: "\u0050"
    readonly property string splitColumns: "\u0051"
    readonly property string splitRows: "\u0052"
    readonly property string testIcon: "\u0053"
    readonly property string textAlignBottom: "\u0054"
    readonly property string textAlignCenter: "\u0055"
    readonly property string textAlignLeft: "\u0056"
    readonly property string textAlignMiddle: "\u0057"
    readonly property string textAlignRight: "\u0058"
    readonly property string textAlignTop: "\u0059"
    readonly property string textBulletList: "\u005A"
    readonly property string textFullJustification: "\u005B"
    readonly property string textNumberedList: "\u005C"
    readonly property string tickIcon: "\u005D"
    readonly property string triState: "\u005E"
    readonly property string undo: "\u005F"
    readonly property string upDownIcon: "\u0060"
    readonly property string upDownSquare2: "\u0061"

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
