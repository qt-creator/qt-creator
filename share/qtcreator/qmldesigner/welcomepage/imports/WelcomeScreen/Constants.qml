
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
pragma Singleton

import QtQuick
import StudioTheme 1.0
import projectmodel 1.0

QtObject {
    readonly property int width: 1842
    readonly property int height: 1080
    property bool communityEdition: false

    property alias fontDirectory: directoryFontLoader.fontDirectory
    property alias relativeFontDirectory: directoryFontLoader.relativeFontDirectory

    /* Edit this comment to add your custom font */
    readonly property font font: Qt.font({
                                             "family": Qt.application.font.family,
                                             "pixelSize": Qt.application.font.pixelSize
                                         })
    readonly property font largeFont: Qt.font({
                                                  "family": Qt.application.font.family,
                                                  "pixelSize": Qt.application.font.pixelSize * 1.6
                                              })

    readonly property color backgroundColor: "#c2c2c2"

    property DirectoryFontLoader directoryFontLoader: DirectoryFontLoader {
        id: directoryFontLoader
    }

    property var projectModel: ProjectModel {}

    ///THEME

    /// theme selector
    property int currentTheme: 0
    property bool defaultBrand: true
    property bool basic: true
    /// theme selector

    /// list view
    property bool isListView: false

    /// Current theme - USE THESE IN YOUR PROPERTY BINGINGS!

    ////BRAND
    property string currentBrand: if (defaultBrand) {
        //console.log ("qds brand" + currentBrand)
        currentBrand: qdsBrand
    } else if (!defaultBrand) {
        //console.log ("creatorBrand" + currentBrand)
        currentBrand: creatorBrand
    }
    ////BRAND

    ////TEXT
    property color currentGlobalText: Values.themeTextColor

    property color currentActiveGlobalText: "#ffffff"

    property string brandGlobalText: if (defaultBrand) {
        //console.log ("dark theme" + brandGlobalText)
        brandGlobalText: qdsGlobalText
    } else if (!defaultBrand) {
        //console.log ("light theme" + brandGlobalText)
        brandGlobalText: creatorGlobalText
    }
    ////TEXT

    ////BACKGROUND
    property color currentThemeBackground: Values.welcomeScreenBackground

    property string modeBarCurrent: if (currentTheme === 0) {
        //console.log ("dark theme" + modeBarCurrent)
        modeBarCurrent: modeBarDark
    } else if (currentTheme == 1) {
        //console.log ("light theme" + modeBarCurrent)
        modeBarCurrent: modeBarMid
    } else if (currentTheme == 2) {
        //console.log ("light theme" + modeBarCurrent)
        modeBarCurrent: modeBarLight
    }
    ////BACKGROUND

    ////BUTTONS
    property color currentPushButtonNormalBackground: Values.themeControlBackground

    property color currentPushButtonHoverBackground: Values.themeControlBackgroundHover

    property string currentPushButtonNormalOutline: Values.themeControlOutline

    property string currentPushButtonHoverOutline: Values.themeControlOutline

    ////THUMBNAILS
    property color currentThumbnailGridBackground: Values.themeSubPanelBackground

    property color currentNormalThumbnailBackground: Values.themeThumbnailBackground

    property color currentNormalThumbnailLabelBackground: Values.themeThumbnailLabelBackground

    property color currentHoverThumbnailLabelBackground: Values.themeControlBackgroundInteraction

    property color currentHoverThumbnailBackground: Values.themeControlBackgroundGlobalHover

    ////THUMBNAILS

    ///GLOBAL COLORS///

    ///brand
    property string creatorBrand: "#54D263"
    property string qdsBrand: "#2e769e"
    ///brand

    /// DARK THEME COLORS ///
    property string darkBackground: "#242424"
    property string modeBarDark: "#414141"

    ///globalText
    property string darkGlobalText: "#ffffff"
    property string darkActiveGlobalText: "#111111"
    property string qdsGlobalText: "#ffffff"
    property string creatorGlobalText: "#111111"
    ///globalText

    ///button
    property string darkPushButtonNormalBackground: "#323232"
    property string darkPushButtonNormalOutline: "#000000"
    property string darkPushButtonHoverBackground: "#474747"
    property string darkPushButtonHoverOutline: "#000000"
    ///button

    ///thumbnails
    property string darkThumbnailGridBackground: "#040404"
    property string darkNormalThumbnailBackground: "#292929"
    property string darkNormalThumbnailLabelBackground: "#3D3D3D"
    property string darkHoverThumbnailBackground: "#323232"
    property string darkHoverThumbnailLabelBackground: "#474747"
    ///thumbnails

    /// MID THEME COLORS ///
    property string midBackground: "#514e48"
    property string modeBarMid: "#737068"

    ///globalText
    property string midGlobalText: "#ffffff"
    property string midActiveGlobalText: "#111111"
    ///globalText

    ///button
    property string midPushButtonNormalBackground: "#43413c"
    property string midPushButtonNormalOutline: "#636058"
    property string midPushButtonHoverBackground: "#504D47"
    property string midPushButtonHoverOutline: "#737068"
    ///button

    ///thumbnails
    property string midThumbnailGridBackground: "#383732"
    property string midNormalThumbnailBackground: "#514e48"
    property string midNormalThumbnailLabelBackground: "#43413c"
    property string midHoverThumbnailBackground: "#5B5851"
    property string midHoverThumbnailLabelBackground: "#43413c"
    ///thumbnails

    /// LIGHT THEME COLORS ///
    property string lightBackground: "#EAEAEA"
    property string modeBarLight: "#D1CFCF"

    ///globalText
    property string lightGlobalText: "#111111"
    property string lightActiveGlobalText: "#ffffff"
    ///globalText

    ///button
    property string lightPushButtonNormalBackground: "#eaeaea"
    property string lightPushButtonNormalOutline: "#CACECE"
    property string lightPushButtonHoverBackground: "#E5E5E5"
    property string lightPushButtonHoverOutline: "#CACECE"
    ///button

    ///thumbnails
    property string lightThumbnailGridBackground: "#EFEFEF"
    property string lightNormalThumbnailBackground: "#F2F2F2"
    property string lightNormalThumbnailLabelBackground: "#EBEBEB"
    property string lightHoverThumbnailBackground: "#EAEAEA"
    property string lightHoverThumbnailLabelBackground: "#E1E1E1"
    ///thumbnails
}
