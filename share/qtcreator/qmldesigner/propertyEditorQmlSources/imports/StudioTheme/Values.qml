// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick
import QtQuickDesignerTheme

QtObject {
    id: values

    property real baseHeight: 29

    property real topLevelComboWidth: 210
    property real topLevelComboHeight: 36
    property real topLevelComboIcon: 20

    property real viewBarComboWidth: 210
    property real viewBarComboHeight: 29
    property real viewBarComboIcon: 16

    property real smallFont: 8
    property real miniIcon: 10
    property real baseFont: 12
    property real mediumFont: 14
    property real bigFont: 16

    property real smallIconFont: 8
    property real baseIconFont: 12
    property real mediumIconFont: 18
    property real bigIconFont: 26

    property real scaleFactor: 1.0

    property real height: Math.round(values.baseHeight * values.scaleFactor)

    property real myFontSize: values.baseFontSize // TODO: rename all refs to myFontSize -> baseFontSize then remove myFontSize

    property real smallFontSize: Math.round(values.smallFont * values.scaleFactor)
    property real baseFontSize: Math.round(values.baseFont * values.scaleFactor)
    property real mediumFontSize: Math.round(values.mediumFont * values.scaleFactor)
    property real bigFontSize: Math.round(values.bigFont * values.scaleFactor)

    property real myIconFontSize: values.baseIconFontSize  // TODO: rename all refs to myIconFontSize -> baseIconFontSize then remove myIconFontSize

    property real smallIconFontSize: Math.round(values.smallIconFont * values.scaleFactor)
    property real baseIconFontSize: Math.round(values.baseIconFont * values.scaleFactor)
    property real mediumIconFontSize: Math.round(values.mediumIconFont * values.scaleFactor)
    property real bigIconFontSize: Math.round(values.bigIconFont * values.scaleFactor)

    property real squareComponentWidth: values.height
    property real smallRectWidth: values.height * 0.75// / 2 * 1.5

    property real inputWidth: values.height * 4

    property real sliderHeight: values.height * 0.75// / 2 * 1.5 // TODO:Have a look at -> sliderAreaHeight: Data.Values.height/2*1.5

    property real sliderControlSize: 12
    property real sliderControlSizeMulti: values.sliderControlSize * values.scaleFactor

    property int dragThreshold: 10 // px
    property real spinControlIconSize: 8
    property real spinControlIconSizeMulti: values.spinControlIconSize * values.scaleFactor

    property real sliderTrackHeight: values.height / 3
    property real sliderHandleWidth: values.sliderTrackHeight * 0.5
    property real sliderHandleHeight: values.sliderTrackHeight * 1.8
    property real sliderFontSize: Math.round(8 * values.scaleFactor)
    property real sliderPadding: Math.round(6 * values.scaleFactor)
    property real sliderMargin: Math.round(3 * values.scaleFactor)

    property real sliderPointerWidth: Math.round(7 * values.scaleFactor)
    property real sliderPointerHeight: Math.round(2 * values.scaleFactor)

    property real checkBoxSpacing: Math.round(6 * values.scaleFactor)

    property real radioButtonSpacing: values.checkBoxSpacing
    //property real radioButtonWidth: values.height
    //property real radioButtonHeight: values.height
    property real radioButtonIndicatorWidth: Math.round(14 * values.scaleFactor)
    property real radioButtonIndicatorHeight: Math.round(14 * values.scaleFactor)

    property real switchSpacing: values.checkBoxSpacing

    property real columnWidth: 225 + (175 * (values.scaleFactor * 2))

    property real marginTopBottom: 4
    property real border: 1
    property real borderHover: 3
    property real radius: 0 //adding specific radiuses

    property real smallRadius: 4

    property real maxComboBoxPopupHeight: Math.round(300 * values.scaleFactor)
    property real maxTextAreaPopupHeight: Math.round(150 * values.scaleFactor)

    property real contextMenuLabelSpacing: Math.round(30 * values.scaleFactor)
    property real contextMenuHorizontalPadding: Math.round(6 * values.scaleFactor)

    property real inputHorizontalPadding: Math.round(6 * values.scaleFactor)
    property real typeLabelVerticalShift: Math.round(6 * values.scaleFactor)

    property real scrollBarThickness: 8
    property real scrollBarThicknessHover: 10
    property real scrollBarActivePadding: 1
    property real scrollBarInactivePadding: 2

    property real toolTipHeight: 25
    property int toolTipDelay: 1000

    // Controls hover animation params
    property int hoverDuration: 500
    property int hoverEasing: Easing.OutExpo

    // Layout sizes
    property real sectionColumnSpacing: 20 // distance between label and sliderControlSize
    property real sectionRowSpacing: 5
    property real sectionHeadGap: 15
    property real sectionHeadHeight: 21 // tab and section
    property real sectionHeadSpacerHeight: 10

    property real sectionPadding: 10
    property real sectionGridSpacing: 5

    property real controlLabelWidth: 15
    property real controlLabelGap: 5

    property real controlGap: 5 // TODO different name
    property real twoControlColumnGap: values.controlLabelGap
                                       + values.controlLabelWidth
                                       + values.controlGap

    property real columnGap: 10

    property real iconAreaWidth: Math.round(21 * values.scaleFactor)

    property real linkControlWidth: values.iconAreaWidth
    property real linkControlHeight: values.height

    property real infinityControlWidth: values.iconAreaWidth
    property real infinityControlHeight: values.height

    property real transform3DSectionSpacing: 15

    // Control sizes

    property real defaultControlWidth: values.squareComponentWidth * 5
    property real defaultControlHeight: values.height

    property real actionIndicatorWidth: values.iconAreaWidth //StudioTheme.Values.squareComponentWidth
    property real actionIndicatorHeight: values.height

    property real spinBoxIndicatorWidth: values.smallRectWidth - 2 * values.border
    property real spinBoxIndicatorHeight: values.height / 2 - values.border

    property real sliderIndicatorWidth: values.squareComponentWidth
    property real sliderIndicatorHeight: values.height

    property real translationIndicatorWidth: values.squareComponentWidth
    property real translationIndicatorHeight: values.height

    property real checkIndicatorWidth: values.squareComponentWidth
    property real checkIndicatorHeight: values.height

    property real singleControlColumnWidth: 2 * values.twoControlColumnWidth
                                            + values.twoControlColumnGap
                                            + values.actionIndicatorWidth

    property real twoControlColumnWidthMin: 3 * values.height - 2 * values.border - 10
    property real twoControlColumnWidthMax: 3 * values.twoControlColumnWidthMin
    property real twoControlColumnWidth: values.twoControlColumnWidthMin

    property real controlColumnWithoutControlsWidth: 2 * (values.actionIndicatorWidth
                                                          + values.twoControlColumnGap)
                                                    + values.linkControlWidth // there could be an issue here with the new style

    property real controlColumnWidth: values.controlColumnWithoutControlsWidth
                                      + 2 * values.twoControlColumnWidth

    property real controlColumnWidthMin: values.controlColumnWithoutControlsWidth
                                         + 2 * values.twoControlColumnWidthMin

    property real propertyLabelWidthMin: 80
    property real propertyLabelWidthMax: 120
    property real propertyLabelWidth: values.propertyLabelWidthMin

    property real sectionLeftPadding: 8
    property real sectionLayoutRightPadding: values.scrollBarThickness + 6

    property real columnFactor: values.propertyLabelWidthMin
                                / (values.propertyLabelWidthMin + values.controlColumnWidthMin)

    property real dialogScreenMargin: Math.round(160 * values.scaleFactor)

    function responsiveResize(width) {
        var tmpWidth = width - values.sectionColumnSpacing
                       - values.sectionLeftPadding - values.sectionLayoutRightPadding
        var labelColumnWidth = Math.round(tmpWidth * values.columnFactor)
        labelColumnWidth = Math.max(Math.min(values.propertyLabelWidthMax, labelColumnWidth),
                                    values.propertyLabelWidthMin)

        var controlColumnWidth = tmpWidth - labelColumnWidth
        var controlWidth = Math.round((controlColumnWidth - values.controlColumnWithoutControlsWidth) * 0.5)
        controlWidth = Math.max(Math.min(values.twoControlColumnWidthMax, controlWidth),
                                values.twoControlColumnWidthMin)

        values.propertyLabelWidth = labelColumnWidth
        values.twoControlColumnWidth = controlWidth
    }

    // Color Editor Popup
    property real colorEditorPopupWidth: 4 * values.colorEditorPopupSpinBoxWidth
                                         + 3 * values.controlGap
                                         + 2 * values.colorEditorPopupPadding
    property real colorEditorPopupHeight: 800
    property real colorEditorPopupPadding: 10
    property real colorEditorPopupMargin: 20

    property real colorEditorPopupSpacing: 10
    property real colorEditorPopupLineHeight: 60

    property real hueSliderHeight: 20
    property real hueSliderHandleWidth: 10

    property real colorEditorPopupComboBoxWidth: 110
    property real colorEditorPopupSpinBoxWidth: 54

    // Popup Window
    property real titleBarHeight: values.height + 20
    property real popupMargin: 10

    // Toolbar
    property real toolbarHeight: 41
    property real doubleToolbarHeight: values.toolbarHeight * 2
    property real toolbarSpacing: 10

    property real toolbarColumnSpacing: 12
    property real toolbarHorizontalMargin: 10
    property real toolbarVerticalMargin: 6

    // Dialog
    property real dialogPadding: 12
    property real dialogButtonSpacing: 10
    property real dialogButtonPadding: 4

    // Collection Editor
    property real collectionItemTextSideMargin: 10
    property real collectionItemTextMargin: 5
    property real collectionItemTextPadding: 5
    property real collectionTableHorizontalMargin: 10
    property real collectionTableVerticalMargin: 10
    property real collectionCellMinimumWidth: 60
    property real collectionCellMinimumHeight: 20

    // NEW NEW NEW
    readonly property int flowMargin: 7
    readonly property int flowSpacing: 7 // Odd so cursor has a center location
    readonly property int flowPillMargin: 4
    readonly property int flowPillHeight: 20
    readonly property int flowPillRadius: 4

    // Theme Colors

    property bool isLightTheme: values.themeControlBackground.hsvValue > values.themeTextColor.hsvValue

    //NEW QtDS 4.0

    //Top & View toolbar colors

    //backgrounds
    property color themeControlBackground_toolbarIdle: Theme.color(Theme.DScontrolBackground_toolbarIdle)
    property color themeControlBackground_toolbarHover: Theme.color(Theme.DScontrolBackground_toolbarHover)
    property color themeControlBackground_topToolbarHover: Theme.color(Theme.DScontrolBackground_topToolbarHover)
    property color themeToolbarBackground: Theme.color(Theme.DStoolbarBackground)
    property color themeConnectionEditorButtonBackground_hover: Theme.color(Theme.DSconnectionEditorButtonBackground_hover)

    //outlines
    property color controlOutline_toolbarIdle: Theme.color(Theme.DScontrolOutline_topToolbarIdle)
    property color controlOutline_toolbarHover: Theme.color(Theme.DScontrolOutline_topToolbarHover)
    property color themeConnectionEditorButtonBorder_hover: Theme.color(Theme.DSconnectionEditorButtonBorder_hover)

    //icons
    property color themeToolbarIcon_blocked: Theme.color(Theme.DStoolbarIcon_blocked)

    //primary buttons
    property color themePrimaryButton_hoverHighlight: Theme.color(Theme.DSprimaryButton_hoverHighlight)

    //states
    property color themeStateControlBackgroundColor_hover: Theme.color(Theme.DSstateControlBackgroundColor_hover)
    property color themeStateBackgroundColor_hover: Theme.color(Theme.DSstateBackgroundColor_hover)
    property color themeStateControlBackgroundColor_globalHover: Theme.color(Theme.DSstateControlBackgroundColor_globalHover)
    property color themeThumbnailBackground_baseState: Theme.color(Theme.DSthumbnailBackground_baseState)

    //task bar
    property color themeStatusbarBackground:Theme.color(Theme.DSstatusbarBackground)
    property color themecontrolBackground_statusbarIdle:Theme.color(Theme.DScontrolBackground_statusbarIdle)
    property color themecontrolBackground_statusbarHover:Theme.color(Theme.DSControlBackground_statusbarHover)

    //run project button
    property color themeIdleGreen: Theme.color(Theme.DSidleGreen)
    property color themeRunningGreen: Theme.color(Theme.DSrunningGreen)

    //popoutWindow (connections)
    property color themePopoutBackground: Theme.color(Theme.DSpopoutBackground)
    property color themePopoutControlBackground_idle: Theme.color(Theme.DSpopoutControlBackground_idle)
    property color themePopoutControlBackground_hover: Theme.color(Theme.DSpopoutControlBackground_hover)
    property color themePopoutControlBackground_globalHover: Theme.color(Theme.DSpopoutControlBackground_globalHover)
    property color themePopoutControlBackground_interaction: Theme.color(Theme.DSpopoutControlBackground_interaction)
    property color themePopoutControlBackground_disabled: Theme.color(Theme.DSpopoutControlBackground_disabled)

    property color themePopoutPopupBackground: Theme.color(Theme.DSpopoutPopupBackground)

    property color themePopoutControlBorder_idle: Theme.color(Theme.DSpopoutControlBorder_idle)
    property color themePopoutControlBorder_hover: Theme.color(Theme.DSpopoutControlBorder_hover)
    property color themePopoutControlBorder_interaction: Theme.color(Theme.DSpopoutControlBorder_interaction)
    property color themePopoutControlBorder_disabled: Theme.color(Theme.DSpopoutControlBorder_disabled)

    property color themePopoutButtonBackground_idle: Theme.color(Theme.DSpopoutButtonBackground_idle)
    property color themePopoutButtonBackground_hover: Theme.color(Theme.DSpopoutButtonBackground_hover)
    property color themePopoutButtonBackground_interaction: Theme.color(Theme.DSpopoutButtonBackground_interaction)
    property color themePopoutButtonBackground_disabled: Theme.color(Theme.DSpopoutButtonBackground_disabled)

    property color themePopoutButtonBorder_idle: Theme.color(Theme.DSpopoutButtonBorder_idle)
    property color themePopoutButtonBorder_hover: Theme.color(Theme.DSpopoutButtonBorder_hover)
    property color themePopoutButtonBorder_interaction: Theme.color(Theme.DSpopoutButtonBorder_interaction)

    //END NEW COLORS QtDS 4.0

    property color themePanelBackground: Theme.color(Theme.DSpanelBackground)

    property color themeGreenLight: Theme.color(Theme.DSgreenLight)
    property color themeAmberLight: Theme.color(Theme.DSamberLight)
    property color themeRedLight: Theme.color(Theme.DSredLight)

    property color themeInteraction: Theme.color(Theme.DSinteraction)
    property color themeError: Theme.color(Theme.DSerrorColor)
    property color themeWarning: Theme.color(Theme.DSwarningColor)
    property color themeDisabled: Theme.color(Theme.DSdisabledColor)

    property color themeInteractionHover: Theme.color(Theme.DSinteractionHover)

    property color themeAliasIconChecked: Theme.color(Theme.DSnavigatorAliasIconChecked)

    // Control colors
    property color themeControlBackground: Theme.color(Theme.DScontrolBackground)
    property color themeControlBackgroundInteraction: Theme.color(Theme.DScontrolBackgroundInteraction)
    property color themeControlBackgroundDisabled: Theme.color(Theme.DScontrolBackgroundDisabled)
    property color themeControlBackgroundGlobalHover: Theme.color(Theme.DScontrolBackgroundGlobalHover)
    property color themeControlBackgroundHover: Theme.color(Theme.DScontrolBackgroundHover)

    property color themeControlOutline: Theme.color(Theme.DScontrolOutline)
    property color themeControlOutlineInteraction: Theme.color(Theme.DScontrolOutlineInteraction)
    property color themeControlOutlineDisabled: Theme.color(Theme.DScontrolOutlineDisabled)

    // Panels & Panes
    property color themeBackgroundColorNormal: Theme.color(Theme.DSBackgroundColorNormal)
    property color themeBackgroundColorAlternate: Theme.color(Theme.DSBackgroundColorAlternate)
    property color themeConnectionCodeEditor: Theme.color(Theme.DSconnectionCodeEditor)
    property color themeConnectionEditorMicroToolbar: Theme.color(Theme.DSconnectionEditorMicroToolbar)

    // Text colors
    property color themeTextColor: Theme.color(Theme.DStextColor)
    property color themeTextColorDisabled: Theme.color(Theme.DStextColorDisabled)
    property color themeTextSelectionColor: Theme.color(Theme.DStextSelectionColor)
    property color themeTextSelectedTextColor: Theme.color(Theme.DStextSelectedTextColor)
    property color themeTextColorDisabledMCU: Theme.color(Theme.DStextColorDisabled)

    property color themePlaceholderTextColor: Theme.color(Theme.DSplaceholderTextColor)
    property color themePlaceholderTextColorInteraction: Theme.color(Theme.DSplaceholderTextColorInteraction)

    // Icon colors
    property color themeIconColor: Theme.color(Theme.DSiconColor)
    property color themeIconColorHover: Theme.color(Theme.DSiconColorHover)
    property color themeIconColorInteraction: Theme.color(Theme.DSiconColorInteraction)
    property color themeIconColorDisabled: Theme.color(Theme.DSiconColorDisabled)
    property color themeIconColorSelected: Theme.color(Theme.DSiconColorSelected)

    property color themeLinkIndicatorColor: Theme.color(Theme.DSlinkIndicatorColor)
    property color themeLinkIndicatorColorHover: Theme.color(Theme.DSlinkIndicatorColorHover)
    property color themeLinkIndicatorColorInteraction: Theme.color(Theme.DSlinkIndicatorColorInteraction)
    property color themeLinkIndicatorColorDisabled: Theme.color(Theme.DSlinkIndicatorColorDisabled)

    property color themeInfiniteLoopIndicatorColor: Theme.color(Theme.DSlinkIndicatorColor)
    property color themeInfiniteLoopIndicatorColorHover: Theme.color(Theme.DSlinkIndicatorColorHover)
    property color themeInfiniteLoopIndicatorColorInteraction: Theme.color(Theme.DSlinkIndicatorColorInteraction)

    // Popup background color (ComboBox, SpinBox, TextArea)
    property color themePopupBackground: Theme.color(Theme.DSpopupBackground)
    // GradientPopupDialog modal overlay color
    property color themePopupOverlayColor: Theme.color(Theme.DSpopupOverlayColor)

    // ToolTip (UrlChooser)
    property color themeToolTipBackground: Theme.color(Theme.DStoolTipBackground)
    property color themeToolTipOutline: Theme.color(Theme.DStoolTipOutline)
    property color themeToolTipText: Theme.color(Theme.DStoolTipText)

    // Slider colors
    property color themeSliderActiveTrack: Theme.color(Theme.DSsliderActiveTrack)
    property color themeSliderActiveTrackHover: Theme.color(Theme.DSsliderActiveTrackHover)
    property color themeSliderActiveTrackFocus: Theme.color(Theme.DSsliderActiveTrackFocus)
    property color themeSliderInactiveTrack: Theme.color(Theme.DSsliderInactiveTrack)
    property color themeSliderInactiveTrackHover: Theme.color(Theme.DSsliderInactiveTrackHover)
    property color themeSliderInactiveTrackFocus: Theme.color(Theme.DSsliderInactiveTrackFocus)
    property color themeSliderHandle: Theme.color(Theme.DSsliderHandle)
    property color themeSliderHandleHover: Theme.color(Theme.DSsliderHandleHover)
    property color themeSliderHandleFocus: Theme.color(Theme.DSsliderHandleFocus)
    property color themeSliderHandleInteraction: Theme.color(Theme.DSsliderHandleInteraction)

    property color themeScrollBarTrack: Theme.color(Theme.DSscrollBarTrack)
    property color themeScrollBarHandle: Theme.color(Theme.DSscrollBarHandle)
    property color themeScrollBarHandle_idle: Theme.color(Theme.DSscrollBarHandle_idle)

    property color themeSectionHeadBackground: Theme.color(Theme.DSsectionHeadBackground)

    property color themeTabActiveBackground: Theme.color(Theme.DStabActiveBackground)
    property color themeTabActiveText: Theme.color(Theme.DStabActiveText)
    property color themeTabInactiveBackground: Theme.color(Theme.DStabInactiveBackground)
    property color themeTabInactiveText: Theme.color(Theme.DStabInactiveText)


    // State Editor
    property color themeStateSeparator: Theme.color(Theme.DSstateSeparatorColor)
    property color themeStateBackground: Theme.color(Theme.DSstateBackgroundColor)
    property color themeStatePreviewOutline: Theme.color(Theme.DSstatePreviewOutline)

    // State Editor *new*
    property color themeStatePanelBackground: Theme.color(Theme.DSstatePanelBackground)
    property color themeStateHighlight: Theme.color(Theme.DSstateHighlight)

    property color themeUnimportedModuleColor: Theme.color(Theme.DSUnimportedModuleColor)

    // Taken out of Constants.js
    property color themeChangedStateText: Theme.color(Theme.DSchangedStateText)

    // 3D
    property color theme3DAxisXColor: Theme.color(Theme.DS3DAxisXColor)
    property color theme3DAxisYColor: Theme.color(Theme.DS3DAxisYColor)
    property color theme3DAxisZColor: Theme.color(Theme.DS3DAxisZColor)

    property color themeActionBinding: Theme.color(Theme.DSactionBinding)
    property color themeActionAlias: Theme.color(Theme.DSactionAlias)
    property color themeActionKeyframe: Theme.color(Theme.DSactionKeyframe)
    property color themeActionJIT: Theme.color(Theme.DSactionJIT)

    property color themeListItemBackground: Theme.color(Theme.DSnavigatorItemBackground)
    property color themeListItemBackgroundHover: Theme.color(Theme.DSnavigatorItemBackgroundHover)
    property color themeListItemBackgroundPress: Theme.color(Theme.DSnavigatorItemBackgroundSelected)
    property color themeListItemText: Theme.color(Theme.DSnavigatorText)
    property color themeListItemTextHover: Theme.color(Theme.DSnavigatorTextHover)
    property color themeListItemTextPress: Theme.color(Theme.DSnavigatorTextSelected)

    // Welcome Page
    property color welcomeScreenBackground: Theme.color(Theme.DSwelcomeScreenBackground)
    property color themeSubPanelBackground: Theme.color(Theme.DSsubPanelBackground)
    property color themeThumbnailBackground: Theme.color(Theme.DSthumbnailBackground)
    property color themeThumbnailLabelBackground: Theme.color(Theme.DSthumbnailLabelBackground)

    // Dialog
    property color themeDialogBackground: values.themeThumbnailBackground
    property color themeDialogOutline: values.themeInteraction

    // Expression Builder
    property color themePillDefaultBackgroundIdle: Theme.color(Theme.DSpillDefaultBackgroundIdle)
    property color themePillDefaultBackgroundHover: Theme.color(Theme.DSpillDefaultBackgroundHover)
    property color themePillOperatorBackgroundIdle: Theme.color(Theme.DSpillOperatorBackgroundIdle)
    property color themePillOperatorBackgroundHover: Theme.color(Theme.DSpillOperatorBackgroundHover)
    property color themePillLiteralBackgroundIdle: Theme.color(Theme.DSpillLiteralBackgroundIdle)
    property color themePillLiteralBackgroundHover: Theme.color(Theme.DSpillLiteralBackgroundHover)

    property color themePillShadowBackground: values.themeInteraction

    property color themePillOutline: "#ffffffff"

    property color themePillText: Theme.color(Theme.DSpillText)
    property color themePillTextSelected: Theme.color(Theme.DSpillTextSelected)
    property color themePillTextEdit: Theme.color(Theme.DspillTextEdit)

    // Control Style Mapping
    property ControlStyle controlStyle: DefaultStyle {}
    property ControlStyle connectionPopupControlStyle: ConnectionPopupControlStyle {}
    property ControlStyle connectionPopupButtonStyle: ConnectionPopupButtonStyle {}
    property ControlStyle toolbarStyle: ToolbarStyle {}
    property ControlStyle primaryToolbarStyle: PrimaryButtonStyle {}
    property ControlStyle toolbarButtonStyle: TopToolbarButtonStyle {}
    property ControlStyle microToolbarButtonStyle: MicroToolbarButtonStyle {}
    property ControlStyle viewBarButtonStyle: ViewBarButtonStyle {}
    property ControlStyle viewBarControlStyle: ViewBarControlStyle {}
    property ControlStyle statusbarButtonStyle: StatusBarButtonStyle {}
    property ControlStyle statusbarControlStyle: StatusBarControlStyle {}
    property ControlStyle statesControlStyle: StatesControlStyle {}
    property ControlStyle searchControlStyle: SearchControlStyle {}
    property ControlStyle viewStyle: ViewStyle {}
}
