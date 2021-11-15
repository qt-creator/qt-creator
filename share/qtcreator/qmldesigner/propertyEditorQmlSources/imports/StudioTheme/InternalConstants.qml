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
    readonly property string download: "\u005D"
    readonly property string edit: "\u005E"
    readonly property string eyeDropper: "\u005F"
    readonly property string favorite: "\u0060"
    readonly property string flowAction: "\u0061"
    readonly property string flowTransition: "\u0062"
    readonly property string fontStyleBold: "\u0063"
    readonly property string fontStyleItalic: "\u0064"
    readonly property string fontStyleStrikethrough: "\u0065"
    readonly property string fontStyleUnderline: "\u0066"
    readonly property string gradient: "\u0067"
    readonly property string gridView: "\u0068"
    readonly property string idAliasOff: "\u0069"
    readonly property string idAliasOn: "\u006A"
    readonly property string infinity: "\u006B"
    readonly property string keyframe: "\u006C"
    readonly property string linkTriangle: "\u006D"
    readonly property string linked: "\u006E"
    readonly property string listView: "\u006F"
    readonly property string lockOff: "\u0070"
    readonly property string lockOn: "\u0071"
    readonly property string mergeCells: "\u0072"
    readonly property string minus: "\u0073"
    readonly property string mirror: "\u0074"
    readonly property string orientation: "\u0075"
    readonly property string paddingEdge: "\u0076"
    readonly property string paddingFrame: "\u0077"
    readonly property string pasteStyle: "\u0078"
    readonly property string pause: "\u0079"
    readonly property string pin: "\u007A"
    readonly property string play: "\u007B"
    readonly property string plus: "\u007C"
    readonly property string promote: "\u007D"
    readonly property string readOnly: "\u007E"
    readonly property string redo: "\u007F"
    readonly property string rotationFill: "\u0080"
    readonly property string rotationOutline: "\u0081"
    readonly property string search: "\u0082"
    readonly property string sectionToggle: "\u0083"
    readonly property string splitColumns: "\u0084"
    readonly property string splitRows: "\u0085"
    readonly property string startNode: "\u0086"
    readonly property string testIcon: "\u0087"
    readonly property string textAlignBottom: "\u0088"
    readonly property string textAlignCenter: "\u0089"
    readonly property string textAlignJustified: "\u008A"
    readonly property string textAlignLeft: "\u008B"
    readonly property string textAlignMiddle: "\u008C"
    readonly property string textAlignRight: "\u008D"
    readonly property string textAlignTop: "\u008E"
    readonly property string textBulletList: "\u008F"
    readonly property string textFullJustification: "\u0090"
    readonly property string textNumberedList: "\u0091"
    readonly property string tickIcon: "\u0092"
    readonly property string translationCreateFiles: "\u0093"
    readonly property string translationCreateReport: "\u0094"
    readonly property string translationExport: "\u0095"
    readonly property string translationImport: "\u0096"
    readonly property string translationSelectLanguages: "\u0097"
    readonly property string translationTest: "\u0098"
    readonly property string transparent: "\u0099"
    readonly property string triState: "\u009A"
    readonly property string triangleArcA: "\u009B"
    readonly property string triangleArcB: "\u009D"
    readonly property string triangleCornerA: "\u009E"
    readonly property string triangleCornerB: "\u009F"
    readonly property string unLinked: "\u00A0"
    readonly property string undo: "\u00A1"
    readonly property string unpin: "\u00A2"
    readonly property string upDownIcon: "\u00A3"
    readonly property string upDownSquare2: "\u00A4"
    readonly property string visibilityOff: "\u00A5"
    readonly property string visibilityOn: "\u00A6"
    readonly property string wildcard: "\u00A7"
    readonly property string wizardsAutomotive: "\u00A8"
    readonly property string wizardsDesktop: "\u00A9"
    readonly property string wizardsGeneric: "\u00AA"
    readonly property string wizardsMcuEmpty: "\u00AB"
    readonly property string wizardsMcuGraph: "\u00AC"
    readonly property string wizardsMobile: "\u00AE"
    readonly property string wizardsUnknown: "\u00AF"
    readonly property string zoomAll: "\u00B0"
    readonly property string zoomIn: "\u00B1"
    readonly property string zoomOut: "\u00B2"
    readonly property string zoomSelection: "\u00B3"

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
