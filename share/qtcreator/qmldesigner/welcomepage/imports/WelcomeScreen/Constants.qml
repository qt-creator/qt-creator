// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton

import QtQuick
import StudioTheme 1.0
import projectmodel 1.0

QtObject {
    id: root

    property int loadingProgress: 100
    readonly property int width: 1842
    readonly property int height: 1080
    property bool communityEdition: projectModel.communityVersion
    property bool enterpriseEdition: projectModel.enterpriseVersion

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

    property var projectModel: ProjectModel {}

    // Responsive grid stuff
    readonly property int minColumnCount: 2
    readonly property int minThumbnailSize: 244
    readonly property int gridSpacing: 8
    readonly property int scrollBarWidth: 10
    readonly property int gridMargins: 20
    readonly property int scrollBarTrackSize: 10

    property int thumbnailSize: root.adaptiveThumbnailSize
    property int gridCellSize: root.adaptiveThumbnailSize + root.gridSpacing
    property int adaptiveThumbnailSize: root.minThumbnailSize // default to minimum size

    function responsiveResize(width) {
        var columnCount = Math.max(Math.floor(width / (root.minThumbnailSize + root.gridSpacing)),
                                   root.minColumnCount)
        root.adaptiveThumbnailSize = Math.floor((width + root.gridSpacing) / columnCount - root.gridSpacing)
    }

    // Thumbnail margins and sizes
    property int thumbnailMargin: 10
    property int imageBottomMargin: 90
    property int titleBackgroundTopMargin: 5
    property int titleHeight: 30
    property int titleHeightMultiline: 52
    property int titleMargin: 5

    /// THEME

    /// theme selector
    property int currentTheme: 0
    property bool defaultBrand: true
    property bool basic: true

    /// list view
    property bool isListView: false

    /// Current theme - USE THESE IN YOUR PROPERTY BINDINGS!

    /// TRAFFIC LIGHT SYSTEM
    property color greenLight: Values.themeGreenLight
    property color amberLight: Values.themeAmberLight
    property color redLight: Values.themeRedLight

    /// BRAND
    property string currentBrand: root.defaultBrand ? root.qdsBrand : root.creatorBrand

    /// TEXT
    property color currentGlobalText: Values.themeTextColor
    property color currentActiveGlobalText: "#ffffff"
    property string brandGlobalText: root.defaultBrand ? root.qdsGlobalText : root.creatorGlobalText

    /// BACKGROUND
    property color currentThemeBackground: Values.welcomeScreenBackground

    /// DIALOG
    property color currentDialogBackground: Values.themeThumbnailBackground
    property color currentDialogBorder: root.currentBrand

    /// BUTTONS
    property color currentPushButtonNormalBackground: Values.themeControlBackground
    property color currentPushButtonHoverBackground: Values.themeControlBackgroundHover

    property color currentPushButtonNormalOutline: Values.themeControlOutline
    property color currentPushButtonHoverOutline: Values.themeControlOutline

    /// THUMBNAILS
    property color currentThumbnailGridBackground: Values.themeSubPanelBackground

    property color currentNormalThumbnailBackground: Values.themeThumbnailBackground
    property color currentHoverThumbnailBackground: Values.themeControlBackgroundGlobalHover

    property color currentNormalThumbnailLabelBackground: Values.themeThumbnailLabelBackground
    property color currentHoverThumbnailLabelBackground: Values.themeControlBackgroundInteraction

    /// TOOLTIP
    property color currentToolTipBackground: Values.themeToolTipBackground
    property color currentToolTipOutline: Values.themeToolTipOutline
    property color currentToolTipText: Values.themeToolTipText

    property color currentScrollBarTrack: Values.themeScrollBarTrack
    property color currentScrollBarHandle: Values.themeScrollBarHandle
    property color currentScrollBarHandle_idle: Values.themeScrollBarHandle_idle

    /// GLOBAL COLORS

    /// brand
    property string creatorBrand: "#54D263"
    property string qdsBrand: "#57B9FC"


    /// DARK THEME COLORS
    property string darkBackground: "#242424"
    property string modeBarDark: "#414141"

    /// globalText
    property string darkGlobalText: "#ffffff"
    property string darkActiveGlobalText: "#111111"
    property string qdsGlobalText: "#ffffff"
    property string creatorGlobalText: "#111111"

    /// button
    property string darkPushButtonNormalBackground: "#323232"
    property string darkPushButtonNormalOutline: "#000000"
    property string darkPushButtonHoverBackground: "#474747"
    property string darkPushButtonHoverOutline: "#000000"

    /// thumbnails
    property string darkThumbnailGridBackground: "#040404"
    property string darkNormalThumbnailBackground: "#292929"
    property string darkNormalThumbnailLabelBackground: "#3D3D3D"
    property string darkHoverThumbnailBackground: "#323232"
    property string darkHoverThumbnailLabelBackground: "#474747"


    /// MID THEME COLORS
    property string midBackground: "#514e48"
    property string modeBarMid: "#737068"

    /// globalText
    property string midGlobalText: "#ffffff"
    property string midActiveGlobalText: "#111111"

    /// button
    property string midPushButtonNormalBackground: "#43413c"
    property string midPushButtonNormalOutline: "#636058"
    property string midPushButtonHoverBackground: "#504D47"
    property string midPushButtonHoverOutline: "#737068"

    /// thumbnails
    property string midThumbnailGridBackground: "#383732"
    property string midNormalThumbnailBackground: "#514e48"
    property string midNormalThumbnailLabelBackground: "#43413c"
    property string midHoverThumbnailBackground: "#5B5851"
    property string midHoverThumbnailLabelBackground: "#43413c"


    /// LIGHT THEME COLORS
    property string lightBackground: "#EAEAEA"
    property string modeBarLight: "#D1CFCF"

    /// globalText
    property string lightGlobalText: "#111111"
    property string lightActiveGlobalText: "#ffffff"

    /// button
    property string lightPushButtonNormalBackground: "#eaeaea"
    property string lightPushButtonNormalOutline: "#CACECE"
    property string lightPushButtonHoverBackground: "#E5E5E5"
    property string lightPushButtonHoverOutline: "#CACECE"

    /// thumbnails
    property string lightThumbnailGridBackground: "#EFEFEF"
    property string lightNormalThumbnailBackground: "#F2F2F2"
    property string lightNormalThumbnailLabelBackground: "#EBEBEB"
    property string lightHoverThumbnailBackground: "#EAEAEA"
    property string lightHoverThumbnailLabelBackground: "#E1E1E1"
}
