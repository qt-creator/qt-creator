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

    objectName: "internalConstantsObject"

    readonly property string actionIcon: "\u0021"
    readonly property string actionIconBinding: "\u0022"
    readonly property string addColumnAfter: "\u0023"
    readonly property string addColumnBefore: "\u0024"
    readonly property string addFile: "\u0025"
    readonly property string addRowAfter: "\u0026"
    readonly property string addRowBefore: "\u0027"
    readonly property string addTable: "\u0028"
    readonly property string adsClose: "\u0029"
    readonly property string adsDetach: "\u002A"
    readonly property string adsDropDown: "\u002B"
    readonly property string aliasAnimated: "\u002C"
    readonly property string aliasProperty: "\u002D"
    readonly property string alignBottom: "\u002E"
    readonly property string alignCenterHorizontal: "\u002F"
    readonly property string alignCenterVertical: "\u0030"
    readonly property string alignLeft: "\u0031"
    readonly property string alignRight: "\u0032"
    readonly property string alignTo: "\u0033"
    readonly property string alignTop: "\u0034"
    readonly property string anchorBaseline: "\u0035"
    readonly property string anchorBottom: "\u0036"
    readonly property string anchorFill: "\u0037"
    readonly property string anchorLeft: "\u0038"
    readonly property string anchorRight: "\u0039"
    readonly property string anchorTop: "\u003A"
    readonly property string animatedProperty: "\u003B"
    readonly property string annotationBubble: "\u003C"
    readonly property string annotationDecal: "\u003D"
    readonly property string assign: "\u003E"
    readonly property string centerHorizontal: "\u003F"
    readonly property string centerVertical: "\u0040"
    readonly property string closeCross: "\u0041"
    readonly property string curveDesigner: "\u0042"
    readonly property string curveEditor: "\u0043"
    readonly property string decisionNode: "\u0044"
    readonly property string deleteColumn: "\u0045"
    readonly property string deleteRow: "\u0046"
    readonly property string deleteTable: "\u0047"
    readonly property string detach: "\u0048"
    readonly property string distributeBottom: "\u0049"
    readonly property string distributeCenterHorizontal: "\u004A"
    readonly property string distributeCenterVertical: "\u004B"
    readonly property string distributeLeft: "\u004C"
    readonly property string distributeOriginBottomRight: "\u004D"
    readonly property string distributeOriginCenter: "\u004E"
    readonly property string distributeOriginNone: "\u004F"
    readonly property string distributeOriginTopLeft: "\u0050"
    readonly property string distributeRight: "\u0051"
    readonly property string distributeSpacingHorizontal: "\u0052"
    readonly property string distributeSpacingVertical: "\u0053"
    readonly property string distributeTop: "\u0054"
    readonly property string edit: "\u0055"
    readonly property string fontStyleBold: "\u0056"
    readonly property string fontStyleItalic: "\u0057"
    readonly property string fontStyleStrikethrough: "\u0058"
    readonly property string fontStyleUnderline: "\u0059"
    readonly property string mergeCells: "\u005A"
    readonly property string redo: "\u005B"
    readonly property string splitColumns: "\u005C"
    readonly property string splitRows: "\u005D"
    readonly property string startNode: "\u005E"
    readonly property string testIcon: "\u005F"
    readonly property string textAlignBottom: "\u0060"
    readonly property string textAlignCenter: "\u0061"
    readonly property string textAlignLeft: "\u0062"
    readonly property string textAlignMiddle: "\u0063"
    readonly property string textAlignRight: "\u0064"
    readonly property string textAlignTop: "\u0065"
    readonly property string textBulletList: "\u0066"
    readonly property string textFullJustification: "\u0067"
    readonly property string textNumberedList: "\u0068"
    readonly property string tickIcon: "\u0069"
    readonly property string triState: "\u006A"
    readonly property string undo: "\u006B"
    readonly property string upDownIcon: "\u006C"
    readonly property string upDownSquare2: "\u006D"
    readonly property string wildcard: "\u006E"

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
