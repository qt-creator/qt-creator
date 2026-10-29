// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils_global.h"

#include <utils/result.h>

#include <QBrush> // QGradientStops
#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QMenu;
class QPalette;
class QSettings;
QT_END_NAMESPACE

namespace Utils {

namespace StyleHelper {
enum class ToolbarStyle;
}

class ThemePrivate;

class QTCREATOR_UTILS_EXPORT Theme : public QObject
{
    Q_OBJECT
public:
    Theme(const QString &id, QObject *parent = nullptr);
    ~Theme() override;

    enum Color {
        BackgroundColorDark,
        BackgroundColorHover,
        BackgroundColorNormal,
        BackgroundColorSelected,
        BackgroundColorDisabled,
        BadgeLabelBackgroundColorChecked,
        BadgeLabelBackgroundColorUnchecked,
        BadgeLabelTextColorChecked,
        BadgeLabelTextColorUnchecked,
        ComboBoxTextColor,
        DetailsWidgetBackgroundColor,
        DockWidgetResizeHandleColor,
        EditorPlaceholderColor,
        FancyToolBarSeparatorColor,
        FancyTabBarSelectedBackgroundColor,
        FancyTabWidgetDisabledSelectedTextColor,
        FancyTabWidgetDisabledUnselectedTextColor,
        FancyTabWidgetEnabledSelectedTextColor,
        FancyTabWidgetEnabledUnselectedTextColor,
        FancyToolButtonHoverColor,
        FancyToolButtonSelectedColor,
        FancyToolButtonHighlightColor,
        InfoBarBackground,
        InfoBarText,
        MenuBarItemTextColorDisabled,
        MenuBarItemTextColorNormal,
        MenuItemTextColorDisabled,
        MenuItemTextColorNormal,
        MiniProjectTargetSelectorBackgroundColor,
        MiniProjectTargetSelectorBorderColor,
        MiniProjectTargetSelectorTextColor,
        OutputPaneButtonFlashColor,
        OutputPaneToggleButtonTextColorChecked,
        OutputPaneToggleButtonTextColorUnchecked,
        PanelStatusBarBackgroundColor,
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
        TextColorNormal,

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
        PaletteAccent,
        PaletteAccentDisabled,

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
        CodeModel_Info_TextMarkColor,

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
        OutputPanes_SearchResultBackgroundColor,

        /* Debugger Log Window */

        Debugger_LogWindow_LogInput,
        Debugger_LogWindow_LogStatus,
        Debugger_LogWindow_LogTime,

        /* Debugger Watch Item */

        Debugger_WatchItem_ValueNormal,
        Debugger_WatchItem_ValueInvalid,
        Debugger_WatchItem_ValueChanged,

        /* Qt Creator Color Tokens */

        Token_Basic_Black,
        Token_Basic_White,
        Token_Accent_Default,
        Token_Accent_Muted,
        Token_Accent_Subtle,
        Token_Background_Default,
        Token_Background_Muted,
        Token_Background_Subtle,
        Token_Foreground_Default,
        Token_Foreground_Muted,
        Token_Foreground_Subtle,
        Token_Text_Default,
        Token_Text_Muted,
        Token_Text_Subtle,
        Token_Text_Accent,
        Token_Text_On_Accent,
        Token_Stroke_Strong,
        Token_Stroke_Muted,
        Token_Stroke_Subtle,
        Token_Notification_Alert_Default,
        Token_Notification_Alert_Muted,
        Token_Notification_Alert_Subtle,
        Token_Notification_Success_Default,
        Token_Notification_Success_Muted,
        Token_Notification_Success_Subtle,
        Token_Notification_Neutral_Default,
        Token_Notification_Neutral_Muted,
        Token_Notification_Neutral_Subtle,
        Token_Notification_Danger_Default,
        Token_Notification_Danger_Muted,
        Token_Notification_Danger_Subtle,
        Token_Gradient01_Start,
        Token_Gradient01_End,
        Token_Gradient02_Start,
        Token_Gradient02_End,

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
        VcsBase_FileUntracked_TextColor,
        VcsBase_FileAdded_TextColor,
        VcsBase_FileModified_TextColor,
        VcsBase_FileDeleted_TextColor,
        VcsBase_FileRenamed_TextColor,
        VcsBase_FileUnmerged_TextColor,

        /* Bookmarks Plugin */
        Bookmarks_TextMarkColor,

        /* TextEditor Plugin */
        TextEditor_SearchResult_ScrollBarColor,
        TextEditor_Selection_ScrollBarColor,
        TextEditor_CurrentLine_ScrollBarColor,
        TextEditor_MergeConflictCurrent_ScrollBarColor,
        TextEditor_MergeConflictIncoming_ScrollBarColor,
        TextEditor_MergeConflictBase_ScrollBarColor,

        /* Debugger Plugin */
        Debugger_Breakpoint_TextMarkColor,

        /* ProjectExplorer Plugin */
        ProjectExplorer_TaskError_TextMarkColor,
        ProjectExplorer_TaskWarn_TextMarkColor,

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

        ChatUserMessageBackground,
        ChatToolCallBackground,
        ChatPlanBackground,
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
        DerivePaletteFromThemeIfNeeded,
        FlatToolBars,
        FlatSideBarIcons,
        FlatProjectsMode,
        FlatMenuBar,
        ToolBarIconShadow,
        WindowColorAsBase,
        DarkUserInterface,
    };

    enum TokenFlag {
        UsedInToolbar = 1,
    };

    Q_ENUM(Color)
    Q_ENUM(ImageFile)
    Q_ENUM(Flag)
    Q_DECLARE_FLAGS(TokenFlags, TokenFlag)

    Q_INVOKABLE bool flag(Utils::Theme::Flag f) const;
    Q_INVOKABLE QColor color(Utils::Theme::Color role) const;
    QString imageFile(ImageFile imageFile, const QString &fallBack) const;
    QPalette palette() const;
    QStringList preferredStyles() const;
    QString defaultTextEditorColorScheme() const;
    StyleHelper::ToolbarStyle defaultToolbarStyle() const;

    QString id() const;
    QString filePath() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void readSettings(QSettings &settings);

    Qt::ColorScheme colorScheme() const;
    static Qt::ColorScheme systemColorScheme();
    static QPalette initialPalette();

    static void setInitialPalette(Theme *initTheme);

    static void setHelpMenu(QMenu *menu);

    static Result<Color> colorToken(const QString &token, TokenFlags flags = {});
    static Color highlightFor(Color role);

protected:
    Theme(Theme *originTheme, QObject *parent = nullptr);
    ThemePrivate *d;

private:
    void readSettingsInternal(QSettings &settings);
    friend QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
    QColor readNamedColorNoWarning(const QString &color) const;
    QPair<QColor, QString> readNamedColor(const QString &color) const;
};

QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
QTCREATOR_UTILS_EXPORT QColor creatorColor(Theme::Color role);

QTCREATOR_UTILS_EXPORT void setCreatorTheme(Theme *theme);

class QTCREATOR_UTILS_EXPORT ThemeManager final : public QObject
{
    Q_OBJECT

public:
    static ThemeManager *instance();

    static int generation();

    // Connects handler to changed(), replacing an earlier connection made for the same
    // owner and key, and leaving the owner's other theme change handlers in place.
    // An empty handler only removes the earlier connection.
    static void onChanged(QObject *owner,
                          const QString &key,
                          const std::function<void()> &handler);

signals:
    void changed();

private:
    ThemeManager() = default;

    friend QTCREATOR_UTILS_EXPORT void setCreatorTheme(Theme *theme);
};

} // namespace Utils
