// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils_global.h"

#include <QBrush> // QGradientStops
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QPalette;
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class ThemePrivate;

class QTCREATOR_UTILS_EXPORT Theme : public QObject
{
    Q_OBJECT
public:
    Theme(const QString &id, QObject *parent = nullptr);
    ~Theme() override;

    enum Color {
        BackgroundColorAlternate,
        BackgroundColorDark,
        BackgroundColorHover,
        BackgroundColorNormal,
        BackgroundColorSelected,
        BackgroundColorDisabled,
        BadgeLabelBackgroundColorChecked,
        BadgeLabelBackgroundColorUnchecked,
        BadgeLabelTextColorChecked,
        BadgeLabelTextColorUnchecked,
        ComboBoxArrowColor,
        ComboBoxArrowColorDisabled,
        ComboBoxTextColor,
        DetailsButtonBackgroundColorHover,
        DetailsWidgetBackgroundColor,
        DockWidgetResizeHandleColor,
        DoubleTabWidget1stSeparatorColor,
        DoubleTabWidget1stTabActiveTextColor,
        DoubleTabWidget1stTabBackgroundColor,
        DoubleTabWidget1stTabInactiveTextColor,
        DoubleTabWidget2ndSeparatorColor,
        DoubleTabWidget2ndTabActiveTextColor,
        DoubleTabWidget2ndTabBackgroundColor,
        DoubleTabWidget2ndTabInactiveTextColor,
        EditorPlaceholderColor,
        FancyToolBarSeparatorColor,
        FancyTabBarBackgroundColor,
        FancyTabBarSelectedBackgroundColor,
        FancyTabWidgetDisabledSelectedTextColor,
        FancyTabWidgetDisabledUnselectedTextColor,
        FancyTabWidgetEnabledSelectedTextColor,
        FancyTabWidgetEnabledUnselectedTextColor,
        FancyToolButtonHoverColor,
        FancyToolButtonSelectedColor,
        FutureProgressBackgroundColor,
        InfoBarBackground,
        InfoBarText, // TODO: Deprecate. Unused.
        MenuBarEmptyAreaBackgroundColor,
        MenuBarItemBackgroundColor,
        MenuBarItemTextColorDisabled,
        MenuBarItemTextColorNormal,
        MenuItemTextColorDisabled,
        MenuItemTextColorNormal,
        MiniProjectTargetSelectorBackgroundColor, // TODO: Deprecate. -> Utils::StyleHelper().baseColor()
        MiniProjectTargetSelectorBorderColor,
        MiniProjectTargetSelectorSummaryBackgroundColor, // TODO: Deprecate. -> Utils::StyleHelper().baseColor()
        MiniProjectTargetSelectorTextColor,
        OutputPaneButtonFlashColor,
        OutputPaneToggleButtonTextColorChecked,
        OutputPaneToggleButtonTextColorUnchecked,
        PanelStatusBarBackgroundColor,
        PanelsWidgetSeparatorLineColor, // TODO: Deprecate. Unused.
        PanelTextColorDark,
        PanelTextColorMid,
        PanelTextColorLight,
        ProgressBarColorError,
        ProgressBarColorFinished,
        ProgressBarColorNormal,
        ProgressBarTitleColor,
        ProgressBarBackgroundColor,
        SplitterColor,
        TextColorDisabled,
        TextColorError,
        TextColorHighlightBackground,
        TextColorLink,
        TextColorLinkVisited,
        TextColorNormal,
        ToggleButtonBackgroundColor,
        ToolBarBackgroundColor,
        TreeViewArrowColorNormal,
        TreeViewArrowColorSelected,

        /* Palette for QPalette */

        PaletteWindow,
        PaletteWindowText,
        PaletteBase,
        PaletteAlternateBase,
        PaletteToolTipBase,
        PaletteToolTipText,
        PaletteText,
        PaletteButton,
        PaletteButtonText,
        PaletteBrightText,
        PaletteHighlight,
        PaletteHighlightedText,
        PaletteLink,
        PaletteLinkVisited,

        PaletteLight,
        PaletteMidlight,
        PaletteDark,
        PaletteMid,
        PaletteShadow,

        PaletteWindowDisabled,
        PaletteWindowTextDisabled,
        PaletteBaseDisabled,
        PaletteAlternateBaseDisabled,
        PaletteToolTipBaseDisabled,
        PaletteToolTipTextDisabled,
        PaletteTextDisabled,
        PaletteButtonDisabled,
        PaletteButtonTextDisabled,
        PaletteBrightTextDisabled,
        PaletteHighlightDisabled,
        PaletteHighlightedTextDisabled,
        PaletteLinkDisabled,
        PaletteLinkVisitedDisabled,

        PaletteLightDisabled,
        PaletteMidlightDisabled,
        PaletteDarkDisabled,
        PaletteMidDisabled,
        PaletteShadowDisabled,

        PalettePlaceholderText,
        PalettePlaceholderTextDisabled,

        /* Icons */

        IconsBaseColor,
        IconsDisabledColor,
        IconsInfoColor,
        IconsInfoToolBarColor,
        IconsWarningColor,
        IconsWarningToolBarColor,
        IconsErrorColor,
        IconsErrorToolBarColor,
        IconsRunColor,
        IconsRunToolBarColor,
        IconsStopColor,
        IconsStopToolBarColor,
        IconsInterruptColor,
        IconsInterruptToolBarColor,
        IconsDebugColor,
        IconsNavigationArrowsColor,
        IconsBuildHammerHandleColor,
        IconsBuildHammerHeadColor,
        IconsModeWelcomeActiveColor,
        IconsModeEditActiveColor,
        IconsModeDesignActiveColor,
        IconsModeDebugActiveColor,
        IconsModeProjectActiveColor,
        IconsModeAnalyzeActiveColor,
        IconsModeHelpActiveColor,

        /* Code model Icons */

        IconsCodeModelKeywordColor,
        IconsCodeModelClassColor,
        IconsCodeModelStructColor,
        IconsCodeModelFunctionColor,
        IconsCodeModelVariableColor,
        IconsCodeModelEnumColor,
        IconsCodeModelMacroColor,
        IconsCodeModelAttributeColor,
        IconsCodeModelUniformColor,
        IconsCodeModelVaryingColor,
        IconsCodeModelOverlayBackgroundColor,
        IconsCodeModelOverlayForegroundColor,

        /* Code model text marks */

        CodeModel_Error_TextMarkColor,
        CodeModel_Warning_TextMarkColor,

        /* Output panes */

        OutputPanes_DebugTextColor,
        OutputPanes_ErrorMessageTextColor,
        OutputPanes_MessageOutput,
        OutputPanes_NormalMessageTextColor,
        OutputPanes_StdErrTextColor,
        OutputPanes_StdOutTextColor,
        OutputPanes_WarningMessageTextColor,
        OutputPanes_TestPassTextColor,
        OutputPanes_TestFailTextColor,
        OutputPanes_TestXFailTextColor,
        OutputPanes_TestXPassTextColor,
        OutputPanes_TestSkipTextColor,
        OutputPanes_TestWarnTextColor,
        OutputPanes_TestFatalTextColor,
        OutputPanes_TestDebugTextColor,

        /* Debugger Log Window */

        Debugger_LogWindow_LogInput,
        Debugger_LogWindow_LogStatus,
        Debugger_LogWindow_LogTime,

        /* Debugger Watch Item */

        Debugger_WatchItem_ValueNormal,
        Debugger_WatchItem_ValueInvalid,
        Debugger_WatchItem_ValueChanged,

        /* Welcome Plugin */

        Welcome_TextColor,
        Welcome_ForegroundPrimaryColor,
        Welcome_ForegroundSecondaryColor,
        Welcome_BackgroundPrimaryColor,
        Welcome_BackgroundSecondaryColor,
        Welcome_HoverColor,
        Welcome_AccentColor,
        Welcome_LinkColor,
        Welcome_DisabledLinkColor,

        /* Timeline Library */
        Timeline_TextColor,
        Timeline_BackgroundColor1,
        Timeline_BackgroundColor2,
        Timeline_DividerColor,
        Timeline_HighlightColor,
        Timeline_PanelBackgroundColor,
        Timeline_PanelHeaderColor,
        Timeline_HandleColor,
        Timeline_RangeColor,

        /* VcsBase Plugin */
        VcsBase_FileStatusUnknown_TextColor,
        VcsBase_FileAdded_TextColor,
        VcsBase_FileModified_TextColor,
        VcsBase_FileDeleted_TextColor,
        VcsBase_FileRenamed_TextColor,
        VcsBase_FileUnmerged_TextColor,

        /* Bookmarks Plugin */
        Bookmarks_TextMarkColor,

        /* TextEditor Plugin */
        TextEditor_SearchResult_ScrollBarColor,
        TextEditor_CurrentLine_ScrollBarColor,

        /* Debugger Plugin */
        Debugger_Breakpoint_TextMarkColor,

        /* ProjectExplorer Plugin */
        ProjectExplorer_TaskError_TextMarkColor,
        ProjectExplorer_TaskWarn_TextMarkColor,

        /* QmlDesigner Plugin */
        QmlDesigner_BackgroundColor,
        QmlDesigner_HighlightColor,
        QmlDesigner_FormEditorSelectionColor,
        QmlDesigner_FormEditorForegroundColor,
        QmlDesigner_BackgroundColorDarker,
        QmlDesigner_BackgroundColorDarkAlternate,
        QmlDesigner_TabLight,
        QmlDesigner_TabDark,
        QmlDesigner_ButtonColor,
        QmlDesigner_BorderColor,
        QmlDesigner_FormeditorBackgroundColor,
        QmlDesigner_AlternateBackgroundColor,
        QmlDesigner_ScrollBarHandleColor,

        /* Palette for DS Controls */
        DSpanelBackground,
        DSinteraction,
        DSerrorColor,
        DSwarningColor,
        DSdisabledColor,
        DSinteractionHover,
        DScontrolBackground,
        DScontrolBackgroundInteraction,
        DScontrolBackgroundDisabled,
        DScontrolBackgroundGlobalHover,
        DScontrolBackgroundHover,
        DScontrolOutline,
        DScontrolOutlineInteraction,
        DScontrolOutlineDisabled,
        DStextColor,
        DStextColorDisabled,
        DStextSelectionColor,
        DStextSelectedTextColor,

        /*NEW for QtDS 4.0*/
        DScontrolBackground_toolbarIdle,
        DScontrolBackground_toolbarHover,
        DStoolbarBackground,
        DStoolbarIcon_blocked,
        DSthumbnailBackground_baseState,
        DSidleGreen,
        DSrunningGreen,
        DSstatusbarBackground,
        DScontrolBackground_statusbarIdle,
        DScontrolBackground_topToolbarHover,
        DSControlBackground_statusbarHover,
        DScontrolOutline_topToolbarIdle,
        DScontrolOutline_topToolbarHover,
        DSprimaryButton_hoverHighlight,
        DSstateBackgroundColor_hover,
        DSstateControlBackgroundColor_hover,
        DSstateControlBackgroundColor_globalHover,
        DSplaceholderTextColor,
        DSplaceholderTextColorInteraction,
        DSpopoutBackground,
        DSpopoutControlBackground_idle,
        DSpopoutControlBackground_hover,
        DSpopoutControlBackground_globalHover,
        DSpopoutControlBackground_interaction,
        DSpopoutControlBackground_disabled,
        DSpopoutPopupBackground,
        DSpopoutControlBorder_idle,
        DSpopoutControlBorder_hover,
        DSpopoutControlBorder_interaction,
        DSpopoutControlBorder_disabled,
        DSpopoutButtonBackground_idle,
        DSpopoutButtonBackground_hover,
        DSpopoutButtonBackground_interaction,
        DSpopoutButtonBackground_disabled,
        DSpopoutButtonBorder_idle,
        DSpopoutButtonBorder_hover,
        DSpopoutButtonBorder_interaction,
        DSpopoutButtonBorder_disabled,
        DSscrollBarTrack,
        DSscrollBarHandle,
        DSscrollBarHandle_idle,
        DSconnectionCodeEditor,
        DSpillText,
        DSpillTextSelected,
        DspillTextEdit,
        DSpillDefaultBackgroundIdle,
        DSpillDefaultBackgroundHover,
        DSpillOperatorBackgroundIdle,
        DSpillOperatorBackgroundHover,
        DSpillLiteralBackgroundIdle,
        DSpillLiteralBackgroundHover,


        /*Legacy QtDS*/
        DSiconColor,
        DSiconColorHover,
        DSiconColorInteraction,
        DSiconColorDisabled,
        DSiconColorSelected,
        DSlinkIndicatorColor,
        DSlinkIndicatorColorHover,
        DSlinkIndicatorColorInteraction,
        DSlinkIndicatorColorDisabled,
        DSpopupBackground,
        DSpopupOverlayColor,
        DSsliderActiveTrack,
        DSsliderActiveTrackHover,
        DSsliderActiveTrackFocus,
        DSsliderInactiveTrack,
        DSsliderInactiveTrackHover,
        DSsliderInactiveTrackFocus,
        DSsliderHandle,
        DSsliderHandleHover,
        DSsliderHandleFocus,
        DSsliderHandleInteraction,
        DSsectionHeadBackground,
        DSstateDefaultHighlight,
        DSstateSeparatorColor,
        DSstateBackgroundColor,
        DSstatePreviewOutline,
        DSchangedStateText,
        DS3DAxisXColor,
        DS3DAxisYColor,
        DS3DAxisZColor,
        DSactionBinding,
        DSactionAlias,
        DSactionKeyframe,
        DSactionJIT,

        DStableHeaderBackground,
        DStableHeaderText,

        DSdockContainerBackground,
        DSdockContainerSplitter,
        DSdockAreaBackground,

        DSdockWidgetBackground,
        DSdockWidgetSplitter,
        DSdockWidgetTitleBar,

        DStitleBarText,
        DStitleBarIcon,
        DStitleBarButtonHover,
        DStitleBarButtonPress,

        DStabContainerBackground,
        DStabSplitter,

        DStabInactiveBackground,
        DStabInactiveText,
        DStabInactiveIcon,
        DStabInactiveButtonHover,
        DStabInactiveButtonPress,

        DStabActiveBackground,
        DStabActiveText,
        DStabActiveIcon,
        DStabActiveButtonHover,
        DStabActiveButtonPress,

        DStabFocusBackground,
        DStabFocusText,
        DStabFocusIcon,
        DStabFocusButtonHover,
        DStabFocusButtonPress,

        DSnavigatorBranch,
        DSnavigatorBranchIndicator,
        DSnavigatorItemBackground,
        DSnavigatorItemBackgroundHover,
        DSnavigatorItemBackgroundSelected,
        DSnavigatorText,
        DSnavigatorTextHover,
        DSnavigatorTextSelected,
        DSnavigatorIcon,
        DSnavigatorIconHover,
        DSnavigatorIconSelected,
        DSnavigatorAliasIconChecked,
        DSnavigatorDropIndicatorBackground,
        DSnavigatorDropIndicatorOutline,

        DSheaderViewBackground,
        DStableViewAlternateBackground,

        DStoolTipBackground,
        DStoolTipOutline,
        DStoolTipText,

        DSBackgroundColorNormal,
        DSBackgroundColorAlternate,

        DSUnimportedModuleColor,

        DSwelcomeScreenBackground,
        DSsubPanelBackground,
        DSthumbnailBackground,
        DSthumbnailLabelBackground,

        DSgreenLight,
        DSamberLight,
        DSredLight,

        DSstatePanelBackground,
        DSstateHighlight,

        TerminalForeground,
        TerminalBackground,
        TerminalSelection,
        TerminalFindMatch,

        TerminalAnsi0,
        TerminalAnsi1,
        TerminalAnsi2,
        TerminalAnsi3,
        TerminalAnsi4,
        TerminalAnsi5,
        TerminalAnsi6,
        TerminalAnsi7,
        TerminalAnsi8,
        TerminalAnsi9,
        TerminalAnsi10,
        TerminalAnsi11,
        TerminalAnsi12,
        TerminalAnsi13,
        TerminalAnsi14,
        TerminalAnsi15,
    };

    enum ImageFile {
        IconOverlayCSource,
        IconOverlayCppHeader,
        IconOverlayCppSource,
        IconOverlayPri,
        IconOverlayPrf,
        IconOverlayPro,
        StandardPixmapFileIcon,
        StandardPixmapDirIcon
    };

    enum Flag {
        DrawTargetSelectorBottom,
        DrawSearchResultWidgetFrame,
        DrawIndicatorBranch,
        DrawToolBarHighlights,
        DrawToolBarBorders,
        ComboBoxDrawTextShadow,
        DerivePaletteFromTheme,
        ApplyThemePaletteGlobally,
        FlatToolBars,
        FlatSideBarIcons,
        FlatProjectsMode,
        FlatMenuBar,
        ToolBarIconShadow,
        WindowColorAsBase,
        DarkUserInterface,
        QDSTheme
    };

    Q_ENUM(Color)
    Q_ENUM(ImageFile)
    Q_ENUM(Flag)

    Q_INVOKABLE bool flag(Utils::Theme::Flag f) const;
    Q_INVOKABLE QColor color(Utils::Theme::Color role) const;
    QString imageFile(ImageFile imageFile, const QString &fallBack) const;
    QPalette palette() const;
    QStringList preferredStyles() const;
    QString defaultTextEditorColorScheme() const;

    QString id() const;
    QString filePath() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void readSettings(QSettings &settings);

    static bool systemUsesDarkMode();
    static QPalette initialPalette();

    static void setInitialPalette(Theme *initTheme);

    static void setHelpMenu(QMenu *menu);

protected:
    Theme(Theme *originTheme, QObject *parent = nullptr);
    ThemePrivate *d;

private:
    friend QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
    friend QTCREATOR_UTILS_EXPORT Theme *proxyTheme();
    QPair<QColor, QString> readNamedColor(const QString &color) const;
};

QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
QTCREATOR_UTILS_EXPORT Theme *proxyTheme();

} // namespace Utils
