// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick

QtObject {
    id: root

    property real smallFontSize: Values.smallFontSize
    property real baseFontSize: Values.baseFontSize
    property real mediumFontSize: Values.mediumFontSize
    property real bigFontSize: Values.bigFontSize

    property real smallIconFontSize: Values.smallIconFontSize
    property real baseIconFontSize: Values.baseIconFontSize
    property real mediumIconFontSize: Values.mediumIconFontSize
    property real bigIconFontSize: Values.bigIconFontSize

    property real borderWidth: Values.border
    property real radius: Values.radius

    property size smallControlSize: Qt.size(Values.smallRectWidth,
                                            Values.smallRectWidth)
    property size squareControlSize: Qt.size(root.controlSize.height,
                                             root.controlSize.height)
    property size controlSize: Qt.size(Values.defaultControlWidth,
                                       Values.defaultControlHeight)

    property size smallIconSize: Qt.size(Values.spinControlIconSizeMulti,
                                         Values.spinControlIconSizeMulti)

    // TODO only used once
    property size spinBoxIndicatorSize: Qt.size(Values.spinBoxIndicatorWidth,
                                                Values.spinBoxIndicatorHeight)

    property size actionIndicatorSize: Qt.size(Values.actionIndicatorWidth,
                                               Values.actionIndicatorHeight)

    property size indicatorIconSize: Qt.size(Values.iconAreaWidth,
                                              Values.height)

    property size radioButtonIndicatorSize: Qt.size(Values.radioButtonIndicatorWidth,
                                                    Values.radioButtonIndicatorHeight)

    // Special properties
    property real maxTextAreaPopupHeight: Values.maxTextAreaPopupHeight
    property real maxComboBoxPopupHeight: Values.maxComboBoxPopupHeight
    property real inputHorizontalPadding: Values.inputHorizontalPadding
    property real controlSpacing: Values.checkBoxSpacing
    property real dialogPadding: Values.dialogPadding
    property real dialogButtonSpacing: Values.dialogButtonSpacing

    property real contextMenuLabelSpacing: Values.contextMenuLabelSpacing
    property real contextMenuHorizontalPadding: Values.contextMenuHorizontalPadding

    property real sliderTrackHeight: Values.sliderTrackHeight
    property size sliderHandleSize: Qt.size(Values.sliderHandleWidth,
                                            Values.sliderHandleHeight)
    property real sliderFontSize: Values.sliderFontSize
    property real sliderPadding: Values.sliderPadding
    property real sliderMargin: Values.sliderMargin
    property size sliderPointerSize: Qt.size(Values.sliderPointerWidth,
                                             Values.sliderPointerHeight)

    property real sectionColumnSpacing: Values.sectionColumnSpacing
    property real sectionRowSpacing: Values.sectionRowSpacing
    property real sectionHeadHeight: Values.sectionHeadHeight
    property real sectionHeadSpacerHeight: Values.sectionHeadSpacerHeight

    property real scrollBarThickness: 4//Values.scrollBarThickness
    property real scrollBarThicknessHover: 6//Values.scrollBarThicknessHover
    property real scrollBarActivePadding: Values.scrollBarActivePadding
    property real scrollBarInactivePadding: Values.scrollBarInactivePadding

    property real dialogScreenMargin: Values.dialogScreenMargin

    // Special colors
    property color interaction: Values.themeInteraction
    property color interactionHover: Values.themeInteractionHover
    property color interactionGlobalHover: "#ffB0E1FC"
    // TODO needs to removed in the future
    property color thumbnailLabelBackground: Values.themeThumbnailLabelBackground

    // Colors
    component BackgroundColors: QtObject {
        property color idle: Values.themeControlBackground
        property color interaction: Values.themeControlBackgroundInteraction
        property color globalHover: Values.themeControlBackground_toolbarHover
        property color hover: Values.themeControlBackground_toolbarHover
        property color disabled: Values.themeControlBackgroundDisabled
    }

    property BackgroundColors background: BackgroundColors {}

    component BorderColors: QtObject {
        property color idle: Values.themeControlOutline
        property color interaction: Values.themeControlOutlineInteraction
        property color hover: Values.controlOutline_toolbarHover
        property color disabled: Values.themeControlBackground
    }

    property BorderColors border: BorderColors {}

    component TextColors: QtObject {
        property color idle: Values.themeTextColor
        property color interaction: Values.themeTextSelectedTextColor
        property color hover: Values.themeTextColor
        property color disabled: Values.themeTextColorDisabled
        property color selection: Values.themeTextSelectionColor
        property color selectedText: Values.themeTextSelectedTextColor
        property color placeholder: Values.themePlaceholderTextColor
        property color placeholderHover: Values.themePlaceholderTextColor
        property color placeholderInteraction: Values.themePlaceholderTextColorInteraction
    }

    property TextColors text: TextColors {}

    component IconColors: QtObject {
        property color idle: Values.themeIconColor
        property color interaction: Values.themeIconColor
        property color selected: Values.themeIconColorSelected
        property color hover: Values.themeIconColorHover
        property color disabled: Values.themeIconColorDisabled
    }

    property IconColors icon: IconColors {}

    // Link- and InfinityLoopIndicator
    component IndicatorColors: QtObject {
        property color idle: Values.themeLinkIndicatorColor
        property color interaction: Values.themeLinkIndicatorColorHover
        property color hover: Values.themeLinkIndicatorColorInteraction
        property color disabled: Values.themeLinkIndicatorColorDisabled
    }

    property IndicatorColors indicator: IndicatorColors {}

    component SliderColors: QtObject {
        property color activeTrack: Values.themeSliderActiveTrack
        property color activeTrackHover: Values.themeSliderActiveTrackHover
        property color activeTrackFocus: Values.themeSliderActiveTrackFocus
        property color inactiveTrack: Values.themeSliderInactiveTrack
        property color inactiveTrackHover: Values.themeSliderInactiveTrackHover
        property color inactiveTrackFocus: Values.themeSliderInactiveTrackFocus
        property color handle: Values.themeSliderHandle
        property color handleHover: Values.themeSliderHandleHover
        property color handleFocus: Values.themeSliderHandleFocus
        property color handleInteraction: Values.themeSliderHandleInteraction
    }

    property SliderColors slider: SliderColors {}

    component ScrollBarColors: QtObject {
        property color track: Values.themeScrollBarTrack
        property color handle: Values.themeScrollBarHandle_idle
        property color handleHover: Values.themeScrollBarHandle
    }

    property ScrollBarColors scrollBar: ScrollBarColors {}

    component SectionColors: QtObject {
        property color head: Values.themeSectionHeadBackground
    }

    property SectionColors section: SectionColors {}

    component PanelColors: QtObject {
        property color background: Values.themePanelBackground
    }

    property PanelColors panel: PanelColors {}

    component PopupColors: QtObject {
        property color background: Values.themePopupBackground
        property color overlay: Values.themePopupOverlayColor
    }

    property PopupColors popup: PopupColors {}

    component DialogColors: QtObject {
        property color background: Values.themeDialogBackground
        property color border: Values.themeDialogOutline
        property color overlay: Values.themeDialogBackground
    }

    property DialogColors dialog: DialogColors {}

    component ToolTipColors: QtObject {
        property color background: Values.themeToolTipBackground
        property color border: Values.themeToolTipOutline
        property color text: Values.themeToolTipText
    }

    property ToolTipColors toolTip: ToolTipColors {}
}
