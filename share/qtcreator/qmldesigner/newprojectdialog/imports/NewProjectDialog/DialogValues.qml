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
import QtQml

import StudioTheme as StudioTheme

QtObject {
    readonly property int dialogWidth: 1522
    readonly property int dialogHeight: 940
    readonly property int projectViewMinimumWidth: 600
    readonly property int projectViewMinimumHeight: projectViewHeight
    readonly property int dialogContentHeight: projectViewHeight + 300  // i.e. dialog without header and footer
    readonly property int loadedPanesWidth: detailsPaneWidth + stylesPaneWidth
    readonly property int detailsPaneWidth: 330 + detailsPanePadding * 2// + 10 // 50
    readonly property int dialogTitleTextHeight: 47
    /* detailsScrollableContentHeight - the full height that may need to be scrolled to be fully
       visible, if the dialog box is too small. */
    readonly property int detailsScrollableContentHeight: 428
    readonly property int stylesPaneWidth: styleImageWidth + stylesPanePadding * 2 + styleImageBorderWidth * 2 // i.e. 240px
    readonly property int detailsPanePadding: 18
    readonly property int stylesPanePadding: 18
    readonly property int defaultPadding: 18

    readonly property int styleImageWidth: 200
    readonly property int styleImageBorderWidth: 2
    readonly property int footerHeight: 73
    readonly property int projectItemWidth: 90
    readonly property int projectItemHeight: 144
    readonly property int projectViewHeight: projectItemHeight * 2
    readonly property int projectViewHeaderHeight: 38

    readonly property int dialogButtonWidth: 100

    readonly property int loadedPanesHeight: dialogContentHeight
    readonly property int detailsPaneHeight: dialogContentHeight

    readonly property string darkPaneColor: StudioTheme.Values.themeBackgroundColorNormal
    readonly property string lightPaneColor: StudioTheme.Values.themeBackgroundColorAlternate

    readonly property string textColor: StudioTheme.Values.themeTextColor
    readonly property string textColorInteraction: StudioTheme.Values.themeInteraction
    readonly property string dividerlineColor: StudioTheme.Values.themeTextColorDisabled
    readonly property string textError: StudioTheme.Values.themeError
    readonly property string textWarning: StudioTheme.Values.themeWarning

    readonly property real defaultPixelSize: 14
    readonly property real defaultLineHeight: 21
    readonly property real viewHeaderPixelSize: 16
    readonly property real viewHeaderLineHeight: 24
    readonly property real paneTitlePixelSize: 18
    readonly property real paneTitleLineHeight: 27
    readonly property int dialogTitlePixelSize: 32
    readonly property int dialogTitleLineHeight: 49

    // for a spacer item
    function narrowSpacing(value, layoutSpacing = DialogValues.defaultPadding) {
        /* e.g. if we want narrow spacing value = 11, then for the spacer item residing inside a
                layout with spacing set to 18, we need to realize the fact that by adding the spacer
                item, we already have 18 * 2 spacing added implicitly (i.e. spacing before the spacer
                item and spacing after it). So we have to subtract 2 x layout spacing before setting
                our own, narrower, spacing.
               */
        return -layoutSpacing -layoutSpacing + value
    }
}
