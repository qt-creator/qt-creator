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

    property FontLoader controlIcons: FontLoader {
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
    readonly property string bevelAll: "\u003F"
    readonly property string bevelCorner: "\u0040"
    readonly property string centerHorizontal: "\u0041"
    readonly property string centerVertical: "\u0042"
    readonly property string closeCross: "\u0043"
    readonly property string colorPopupClose: "\u0044"
    readonly property string columnsAndRows: "\u0045"
    readonly property string copyStyle: "\u0046"
    readonly property string cornerA: "\u0047"
    readonly property string cornerB: "\u0048"
    readonly property string cornersAll: "\u0049"
    readonly property string curveDesigner: "\u004A"
    readonly property string curveEditor: "\u004B"
    readonly property string decisionNode: "\u004C"
    readonly property string deleteColumn: "\u004D"
    readonly property string deleteRow: "\u004E"
    readonly property string deleteTable: "\u004F"
    readonly property string detach: "\u0050"
    readonly property string distributeBottom: "\u0051"
    readonly property string distributeCenterHorizontal: "\u0052"
    readonly property string distributeCenterVertical: "\u0053"
    readonly property string distributeLeft: "\u0054"
    readonly property string distributeOriginBottomRight: "\u0055"
    readonly property string distributeOriginCenter: "\u0056"
    readonly property string distributeOriginNone: "\u0057"
    readonly property string distributeOriginTopLeft: "\u0058"
    readonly property string distributeRight: "\u0059"
    readonly property string distributeSpacingHorizontal: "\u005A"
    readonly property string distributeSpacingVertical: "\u005B"
    readonly property string distributeTop: "\u005C"
    readonly property string edit: "\u005D"
    readonly property string eyeDropper: "\u005E"
    readonly property string flowAction: "\u005F"
    readonly property string flowTransition: "\u0060"
    readonly property string fontStyleBold: "\u0061"
    readonly property string fontStyleItalic: "\u0062"
    readonly property string fontStyleStrikethrough: "\u0063"
    readonly property string fontStyleUnderline: "\u0064"
    readonly property string gradient: "\u0065"
    readonly property string gridView: "\u0066"
    readonly property string idAliasOff: "\u0067"
    readonly property string idAliasOn: "\u0068"
    readonly property string infinity: "\u0069"
    readonly property string keyframe: "\u006A"
    readonly property string linkTriangle: "\u006B"
    readonly property string linked: "\u006C"
    readonly property string listView: "\u006D"
    readonly property string lockOff: "\u006E"
    readonly property string lockOn: "\u006F"
    readonly property string mergeCells: "\u0070"
    readonly property string minus: "\u0071"
    readonly property string mirror: "\u0072"
    readonly property string orientation: "\u0073"
    readonly property string paddingEdge: "\u0074"
    readonly property string paddingFrame: "\u0075"
    readonly property string pasteStyle: "\u0076"
    readonly property string pause: "\u0077"
    readonly property string pin: "\u0078"
    readonly property string play: "\u0079"
    readonly property string plus: "\u007A"
    readonly property string promote: "\u007B"
    readonly property string redo: "\u007C"
    readonly property string rotationFill: "\u007D"
    readonly property string rotationOutline: "\u007E"
    readonly property string search: "\u007F"
    readonly property string sectionToggle: "\u0080"
    readonly property string splitColumns: "\u0081"
    readonly property string splitRows: "\u0082"
    readonly property string startNode: "\u0083"
    readonly property string testIcon: "\u0084"
    readonly property string textAlignBottom: "\u0085"
    readonly property string textAlignCenter: "\u0086"
    readonly property string textAlignLeft: "\u0087"
    readonly property string textAlignMiddle: "\u0088"
    readonly property string textAlignRight: "\u0089"
    readonly property string textAlignTop: "\u008A"
    readonly property string textBulletList: "\u008B"
    readonly property string textFullJustification: "\u008C"
    readonly property string textNumberedList: "\u008D"
    readonly property string tickIcon: "\u008E"
    readonly property string transparent: "\u008F"
    readonly property string triState: "\u0090"
    readonly property string triangleArcA: "\u0091"
    readonly property string triangleArcB: "\u0092"
    readonly property string triangleCornerA: "\u0093"
    readonly property string triangleCornerB: "\u0094"
    readonly property string unLinked: "\u0095"
    readonly property string undo: "\u0096"
    readonly property string unpin: "\u0097"
    readonly property string upDownIcon: "\u0098"
    readonly property string upDownSquare2: "\u0099"
    readonly property string visibilityOff: "\u009A"
    readonly property string visibilityOn: "\u009B"
    readonly property string wildcard: "\u009C"
    readonly property string zoomAll: "\u009D"
    readonly property string zoomIn: "\u009E"
    readonly property string zoomOut: "\u009F"
    readonly property string zoomSelection: "\u00A0"

    readonly property font iconFont: Qt.font({
                                                 "family": controlIcons.name,
                                                 "pixelSize": 12
                                             })

    readonly property font font: Qt.font({
                                             "family": "Arial",
                                             "pointSize": Qt.application.font.pixelSize
                                         })

    readonly property font largeFont: Qt.font({
                                                  "family": "Arial",
                                                  "pointSize": Qt.application.font.pixelSize * 1.6
                                              })
}
