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

QtObject {
    id: values

    property real baseHeight: 20
    property real baseFont: 12
    property real baseIconFont: 10

    property real scaleFactor: 1

    property real height: Math.round(values.baseHeight * values.scaleFactor)
    property real myFontSize: Math.round(values.baseFont * values.scaleFactor)
    property real myIconFontSize: Math.round(values.baseIconFont * values.scaleFactor)

    property real squareComponentWidth: values.height
    property real smallRectWidth: values.height / 2 * 1.5

    property real inputWidth: values.height * 4

    property real sliderHeight: values.height / 2 * 1.5 // TODO:Have a look at -> sliderAreaHeight: Data.Values.height/2*1.5

    property real sliderControlSize: 12
    property real sliderControlSizeMulti: sliderControlSize * scaleFactor

    property real spinControlIconSize: 8
    property real spinControlIconSizeMulti: spinControlIconSize * scaleFactor

    property real sliderTrackHeight: values.height / 4
    property real sliderHandleHeight: values.sliderTrackHeight * 2
    property real sliderHandleWidth: values.sliderTrackHeight
    property real sliderFontSize: Math.round(8 * values.scaleFactor)
    property real sliderPadding: Math.round(6 * values.scaleFactor)
    property real sliderMargin: Math.round(3 * values.scaleFactor)

    property real sliderPointerWidth: Math.round(7 * values.scaleFactor)
    property real sliderPointerHeight: Math.round(2 * values.scaleFactor)

    property real checkBoxSpacing: 6 // TODO Does look strange with scale factor applied

    property real columnWidth: 225 + (175 * (values.scaleFactor * 2))

    property real marginTopBottom: 4
    property real border: 1

    property real maxComboBoxPopupHeight: 300
    property real maxTextAreaPopupHeight: 150

    property real contextMenuLabelSpacing: 30
    property real contextMenuHorizontalPadding: 6

    property real inputHorizontalPadding: Math.round(4 * values.scaleFactor)

    // Theme Colors

    // Dark Theme Defaults
    property string themeControlBackground: "#242424"
    property string themeControlOutline: "#404040"
    property string themeTextColor: "#ffffff"

    property string themePanelBackground: "#2a2a2a"
    property string themeHoverHighlight: "#313131"
    property string themeColumnBackground: "#363636"
    property string themeFocusEdit: "#606060"
    property string themeFocusDrag: "#565656"

    property string themeControlBackgroundPressed: "#606060"
    property string themeControlBackgroundChecked: "#565656"

    property string themeInteraction: "#029de0"

    property string themeSliderActiveTrack: "#606060"
    property string themeSliderInactiveTrack: "#404040"
    property string themeSliderHandle: "#505050"

    property string themeSliderActiveTrackHover: "#7f7f7f"
    property string themeSliderInactiveTrackHover: "#505050"
    property string themeSliderHandleHover: "#606060"

    property string themeSliderActiveTrackFocus: "#aaaaaa"
    property string themeSliderInactiveTrackFocus: "#606060"
    property string themeSliderHandleFocus: values.themeInteraction

    // NEW NEW NEW NEW NEW
    property string themeControlBackgroundDisabled: "#363636"
    property string themeControlOutlineDisabled: "#404040"
    property string themeTextColorDisabled: "#606060"

    property string themeTextSelectionColor: "#029de0"
    property string themeTextSelectedTextColor: "#ffffff"

    property string themeScrollBarTrack: "#404040"
    property string themeScrollBarHandle: "#505050"

    property string themeControlBackgroundInteraction: "#404040" // TODO Name. Right now themeFocusEdit is used for all 'edit' states. Is that correct? Different color!

    property string themeTranslationIndicatorBorder: "#7f7f7f"
}
