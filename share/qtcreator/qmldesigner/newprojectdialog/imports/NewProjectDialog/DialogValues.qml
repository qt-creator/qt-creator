// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQml

import StudioTheme as StudioTheme

QtObject {
    id: root

    readonly property int dialogWidth: 1522
    readonly property int dialogHeight: 940
    readonly property int presetViewMinimumWidth: 600
    readonly property int presetViewMinimumHeight: root.gridCellHeight
    readonly property int dialogContentHeight: root.presetViewHeight + 300 // i.e. dialog without header and footer
    readonly property int loadedPanesWidth: root.detailsPaneWidth + root.stylesPaneWidth
    readonly property int detailsPaneWidth: 330 + root.detailsPanePadding * 2
    readonly property int dialogTitleTextHeight: 85
    readonly property int paneTitleTextHeight: 47
    readonly property int logoWidth: 85
    readonly property int logoHeight: 85

    readonly property int stylesPaneWidth: root.styleImageWidth + root.stylesPanePadding * 2
                                           + root.styleImageBorderWidth * 2 // i.e. 240px
    readonly property int detailsPanePadding: 18
    readonly property int stylesPanePadding: 18
    readonly property int defaultPadding: 18
    readonly property int dialogLeftPadding: 35

    readonly property int styleListItemHeight: root.styleImageHeight + root.styleTextHeight
                                               + 2 * root.styleImageBorderWidth
                                               + root.styleListItemBottomMargin
                                               + root.styleListItemSpacing
    readonly property int styleListItemBottomMargin: 10
    readonly property int styleListItemSpacing: 4
    readonly property int styleImageWidth: 200
    readonly property int styleImageHeight: 262
    readonly property int styleImageBorderWidth: 2
    readonly property int styleTextHeight: 18

    readonly property int footerHeight: 73
    readonly property int presetItemWidth: 136
    readonly property int presetItemHeight: 110
    property int presetViewHeight: root.presetItemHeight * 2 + root.gridSpacing + root.gridMargins * 2
    readonly property int presetViewHeaderHeight: 38

    readonly property int gridMargins: 20
    readonly property int gridCellWidth: root.presetItemWidth + root.gridSpacing
    readonly property int gridCellHeight: root.presetItemHeight + root.gridSpacing
    readonly property int gridSpacing: 2

    readonly property int dialogButtonWidth: 100

    // This is for internal popup dialogs
    readonly property int popupDialogWidth: 270
    readonly property int popupDialogPadding: 12

    readonly property int loadedPanesHeight: root.dialogContentHeight
    readonly property int detailsPaneHeight: root.dialogContentHeight

    readonly property string darkPaneColor: StudioTheme.Values.themeBackgroundColorNormal
    readonly property string lightPaneColor: StudioTheme.Values.themeBackgroundColorAlternate

    readonly property string textColor: StudioTheme.Values.themeTextColor
    readonly property string textColorInteraction: StudioTheme.Values.themeInteraction
    readonly property string dividerlineColor: StudioTheme.Values.themeTextColorDisabled
    readonly property string textError: StudioTheme.Values.themeError
    readonly property string textWarning: StudioTheme.Values.themeWarning
    readonly property string presetItemBackgroundHover: StudioTheme.Values.themeControlBackgroundGlobalHover
    readonly property string presetItemBackgroundHoverInteraction: StudioTheme.Values.themeControlBackgroundInteraction

    readonly property real defaultPixelSize: 14
    readonly property real defaultLineHeight: 21
    readonly property real viewHeaderPixelSize: 16
    readonly property real viewHeaderLineHeight: 24
    readonly property real paneTitlePixelSize: 18
    readonly property real paneTitleLineHeight: 27
    readonly property int dialogTitlePixelSize: 38
    readonly property int dialogTitleLineHeight: 49

    readonly property string brandTextColor: "#2e769e"

    // for a spacer item
    function narrowSpacing(value, layoutSpacing = DialogValues.defaultPadding) {
        /* e.g. if we want narrow spacing value = 11, then for the spacer item residing inside a
                layout with spacing set to 18, we need to realize the fact that by adding the spacer
                item, we already have 18 * 2 spacing added implicitly (i.e. spacing before the spacer
                item and spacing after it). So we have to subtract 2 x layout spacing before setting
                our own, narrower, spacing.
               */
        return -layoutSpacing - layoutSpacing + value
    }
}
