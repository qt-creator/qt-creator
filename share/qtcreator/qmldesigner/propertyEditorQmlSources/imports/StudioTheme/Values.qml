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
import QtQuick 2.12
import QtQuickDesignerTheme 1.0

QtObject {
    id: values

    property real baseHeight: 20
    property real baseFont: 12
    property real baseIconFont: 10

    property real scaleFactor: 1.1

    property real height: Math.round(values.baseHeight * values.scaleFactor)
    property real myFontSize: Math.round(values.baseFont * values.scaleFactor)
    property real myIconFontSize: Math.round(values.baseIconFont * values.scaleFactor)

    property real squareComponentWidth: values.height
    property real smallRectWidth: values.height / 2 * 1.5

    property real inputWidth: values.height * 4

    property real sliderHeight: values.height / 2 * 1.5 // TODO:Have a look at -> sliderAreaHeight: Data.Values.height/2*1.5

    property real sliderControlSize: 12
    property real sliderControlSizeMulti: values.sliderControlSize * values.scaleFactor

    property int dragLength: 400 // px
    property real spinControlIconSize: 8
    property real spinControlIconSizeMulti: values.spinControlIconSize * values.scaleFactor

    property real sliderTrackHeight: values.height / 4
    property real sliderHandleHeight: values.sliderTrackHeight * 2
    property real sliderHandleWidth: values.sliderTrackHeight
    property real sliderFontSize: Math.round(8 * values.scaleFactor)
    property real sliderPadding: Math.round(6 * values.scaleFactor)
    property real sliderMargin: Math.round(3 * values.scaleFactor)

    property real sliderPointerWidth: Math.round(7 * values.scaleFactor)
    property real sliderPointerHeight: Math.round(2 * values.scaleFactor)

    property real checkBoxSpacing: Math.round(6 * values.scaleFactor)

    property real columnWidth: 225 + (175 * (values.scaleFactor * 2))

    property real marginTopBottom: 4
    property real border: 1

    property real maxComboBoxPopupHeight: Math.round(300 * values.scaleFactor)
    property real maxTextAreaPopupHeight: Math.round(150 * values.scaleFactor)

    property real contextMenuLabelSpacing: Math.round(30 * values.scaleFactor)
    property real contextMenuHorizontalPadding: Math.round(6 * values.scaleFactor)

    property real inputHorizontalPadding: Math.round(4 * values.scaleFactor)

    // Theme Colors

    // COLORS NOW COME FROM THE THEME FILES
    property string themeControlBackground: Theme.color(Theme.DScontrolBackground)
    property string themeControlOutline: Theme.color(Theme.DScontrolOutline)
    property string themeTextColor: Theme.color(Theme.DStextColor)
    property string themeDisabledTextColor: Theme.color(Theme.DSdisabledTextColor)
    property string themePanelBackground: Theme.color(Theme.DSpanelBackground)
    property string themeHoverHighlight: Theme.color(Theme.DShoverHighlight)
    property string themeColumnBackground: Theme.color(Theme.DScolumnBackground)
    property string themeFocusEdit: Theme.color(Theme.DSfocusEdit)
    property string themeFocusDrag: Theme.color(Theme.DSfocusDrag)
    property string themeControlBackgroundPressed: Theme.color(Theme.DScontrolBackgroundPressed)
    property string themeControlBackgroundChecked: Theme.color(Theme.DScontrolBackgroundChecked)
    property string themeInteraction: Theme.color(Theme.DSinteraction)
    property string themeSliderActiveTrack: Theme.color(Theme.DSsliderActiveTrack)
    property string themeSliderInactiveTrack: Theme.color(Theme.DSsliderInactiveTrack)
    property string themeSliderHandle: Theme.color(Theme.DSsliderHandle)
    property string themeSliderActiveTrackHover: Theme.color(Theme.DSactiveTrackHover)
    property string themeSliderInactiveTrackHover: Theme.color(Theme.DSsliderInactiveTrackHover)
    property string themeSliderHandleHover: Theme.color(Theme.DSsliderHandleHover)
    property string themeSliderActiveTrackFocus: Theme.color(Theme.DSsliderActiveTrackFocus)
    property string themeSliderInactiveTrackFocus:Theme.color(Theme.DSsliderInactiveTrackFocus)
    property string themeSliderHandleFocus: Theme.color(Theme.DSsliderHandleFocus)
    property string themeErrorColor: Theme.color(Theme.DSerrorColor)

    // NEW NEW NEW NEW NEW
    property string themeControlBackgroundDisabled: Theme.color(Theme.DScontrolBackgroundDisabled)
    property string themeControlOutlineDisabled: Theme.color(Theme.DScontrolOutlineDisabled)
    property string themeTextColorDisabled: Theme.color(Theme.DStextColorDisabled)
    property string themeTextSelectionColor: Theme.color(Theme.DStextSelectionColor)
    property string themeTextSelectedTextColor:Theme.color(Theme.DStextSelectedTextColor)
    property string themeScrollBarTrack: Theme.color(Theme.DSscrollBarTrack)
    property string themeScrollBarHandle: Theme.color(Theme.DSscrollBarHandle)
    property string themeControlBackgroundInteraction: Theme.color(Theme.DScontrolBackgroundInteraction) // TODO Name. Right now themeFocusEdit is used for all 'edit' states. Is that correct? Different color!
    property string themeTranslationIndicatorBorder: Theme.color(Theme.DStranlsationIndicatorBorder)
    property string themeSectionHeadBackground: Theme.color(Theme.DSsectionHeadBackground)

    // Taken out of Constants.js
    property string themeChangedStateText: Theme.color(Theme.DSchangedStateText)
}
