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
    readonly property string alias: "\u002C"
    readonly property string aliasAnimated: "\u002D"
    readonly property string aliasProperty: "\u002E"
    readonly property string alignBottom: "\u002F"
    readonly property string alignCenterHorizontal: "\u0030"
    readonly property string alignCenterVertical: "\u0031"
    readonly property string alignLeft: "\u0032"
    readonly property string alignRight: "\u0033"
    readonly property string alignTo: "\u0034"
    readonly property string alignTop: "\u0035"
    readonly property string anchorBaseline: "\u0036"
    readonly property string anchorBottom: "\u0037"
    readonly property string anchorFill: "\u0038"
    readonly property string anchorLeft: "\u0039"
    readonly property string anchorRight: "\u003A"
    readonly property string anchorTop: "\u003B"
    readonly property string animatedProperty: "\u003C"
    readonly property string annotationBubble: "\u003D"
    readonly property string annotationDecal: "\u003E"
    readonly property string assign: "\u003F"
    readonly property string centerHorizontal: "\u0040"
    readonly property string centerVertical: "\u0041"
    readonly property string closeCross: "\u0042"
    readonly property string curveDesigner: "\u0043"
    readonly property string curveEditor: "\u0044"
    readonly property string decisionNode: "\u0045"
    readonly property string deleteColumn: "\u0046"
    readonly property string deleteRow: "\u0047"
    readonly property string deleteTable: "\u0048"
    readonly property string detach: "\u0049"
    readonly property string distributeBottom: "\u004A"
    readonly property string distributeCenterHorizontal: "\u004B"
    readonly property string distributeCenterVertical: "\u004C"
    readonly property string distributeLeft: "\u004D"
    readonly property string distributeOriginBottomRight: "\u004E"
    readonly property string distributeOriginCenter: "\u004F"
    readonly property string distributeOriginNone: "\u0050"
    readonly property string distributeOriginTopLeft: "\u0051"
    readonly property string distributeRight: "\u0052"
    readonly property string distributeSpacingHorizontal: "\u0053"
    readonly property string distributeSpacingVertical: "\u0054"
    readonly property string distributeTop: "\u0055"
    readonly property string edit: "\u0056"
    readonly property string flowAction: "\u0057"
    readonly property string flowTransition: "\u0058"
    readonly property string fontStyleBold: "\u0059"
    readonly property string fontStyleItalic: "\u005A"
    readonly property string fontStyleStrikethrough: "\u005B"
    readonly property string fontStyleUnderline: "\u005C"
    readonly property string gridView: "\u005D"
    readonly property string idAliasOff: "\u005E"
    readonly property string idAliasOn: "\u005F"
    readonly property string keyframe: "\u0060"
    readonly property string linkTriangle: "\u0061"
    readonly property string linked: "\u0062"
    readonly property string listView: "\u0063"
    readonly property string lockOff: "\u0064"
    readonly property string lockOn: "\u0065"
    readonly property string mergeCells: "\u0066"
    readonly property string minus: "\u0067"
    readonly property string mirror: "\u0068"
    readonly property string pin: "\u0069"
    readonly property string plus: "\u006A"
    readonly property string promote: "\u006B"
    readonly property string redo: "\u006C"
    readonly property string rotationFill: "\u006D"
    readonly property string rotationOutline: "\u006E"
    readonly property string search: "\u006F"
    readonly property string splitColumns: "\u0070"
    readonly property string splitRows: "\u0071"
    readonly property string startNode: "\u0072"
    readonly property string testIcon: "\u0073"
    readonly property string textAlignBottom: "\u0074"
    readonly property string textAlignCenter: "\u0075"
    readonly property string textAlignLeft: "\u0076"
    readonly property string textAlignMiddle: "\u0077"
    readonly property string textAlignRight: "\u0078"
    readonly property string textAlignTop: "\u0079"
    readonly property string textBulletList: "\u007A"
    readonly property string textFullJustification: "\u007B"
    readonly property string textNumberedList: "\u007C"
    readonly property string tickIcon: "\u007D"
    readonly property string triState: "\u007E"
    readonly property string unLinked: "\u007F"
    readonly property string undo: "\u0080"
    readonly property string unpin: "\u0081"
    readonly property string upDownIcon: "\u0082"
    readonly property string upDownSquare2: "\u0083"
    readonly property string visibilityOff: "\u0084"
    readonly property string visibilityOn: "\u0085"
    readonly property string wildcard: "\u0086"
    readonly property string zoomAll: "\u0087"
    readonly property string zoomIn: "\u0088"
    readonly property string zoomOut: "\u0089"
    readonly property string zoomSelection: "\u008A"

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
}
