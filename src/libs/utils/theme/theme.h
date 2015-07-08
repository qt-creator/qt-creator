/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef THEME_H
#define THEME_H

#include "../utils_global.h"

#include <QBrush> // QGradientStops
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QPalette)

namespace Utils {

class ThemePrivate;

class QTCREATOR_UTILS_EXPORT Theme : public QObject
{
    Q_OBJECT

    Q_ENUMS(Color)
    Q_ENUMS(ImageFile)
    Q_ENUMS(Gradient)
    Q_ENUMS(Flag)
    Q_ENUMS(WidgetStyle)

public:
    Theme(const QString &name, QObject *parent = 0);
    ~Theme();

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
        CanceledSearchTextColor,
        ComboBoxArrowColor,
        ComboBoxArrowColorDisabled,
        ComboBoxTextColor,
        DetailsButtonBackgroundColorHover,
        DetailsWidgetBackgroundColor,
        DockWidgetResizeHandleColor,
        DoubleTabWidget1stEmptyAreaBackgroundColor,
        DoubleTabWidget1stSeparatorColor,
        DoubleTabWidget1stTabActiveTextColor,
        DoubleTabWidget1stTabBackgroundColor,
        DoubleTabWidget1stTabInactiveTextColor,
        DoubleTabWidget2ndSeparatorColor,
        DoubleTabWidget2ndTabActiveTextColor,
        DoubleTabWidget2ndTabBackgroundColor,
        DoubleTabWidget2ndTabInactiveTextColor,
        EditorPlaceholderColor,
        FancyTabBarBackgroundColor,
        FancyTabWidgetDisabledSelectedTextColor,
        FancyTabWidgetDisabledUnselectedTextColor,
        FancyTabWidgetEnabledSelectedTextColor,
        FancyTabWidgetEnabledUnselectedTextColor,
        FancyToolButtonHoverColor,
        FancyToolButtonSelectedColor,
        FutureProgressBackgroundColor,
        InfoBarBackground,
        InfoBarText,
        MenuBarEmptyAreaBackgroundColor,
        MenuBarItemBackgroundColor,
        MenuBarItemTextColorDisabled,
        MenuBarItemTextColorNormal,
        MenuItemTextColorDisabled,
        MenuItemTextColorNormal,
        MiniProjectTargetSelectorBackgroundColor,
        MiniProjectTargetSelectorBorderColor,
        MiniProjectTargetSelectorSummaryBackgroundColor,
        MiniProjectTargetSelectorTextColor,
        OutputPaneButtonFlashColor,
        OutputPaneToggleButtonTextColorChecked,
        OutputPaneToggleButtonTextColorUnchecked,
        PanelButtonToolBackgroundColorHover,
        PanelStatusBarBackgroundColor,
        PanelsWidgetSeparatorLineColor,
        PanelTextColorDark,
        PanelTextColorLight,
        ProgressBarColorError,
        ProgressBarColorFinished,
        ProgressBarColorNormal,
        ProgressBarTitleColor,
        SplitterColor,
        TextColorDisabled,
        TextColorError,
        TextColorHighlight,
        TextColorLink,
        TextColorLinkVisited,
        TextColorNormal,
        TodoItemTextColor,
        ToggleButtonBackgroundColor,
        ToolBarBackgroundColor,
        TreeViewArrowColorNormal,
        TreeViewArrowColorSelected,

        /* Output panes */

        OutputPanes_DebugTextColor,
        OutputPanes_ErrorMessageTextColor,
        OutputPanes_MessageOutput,
        OutputPanes_NormalMessageTextColor,
        OutputPanes_StdErrTextColor,
        OutputPanes_StdOutTextColor,
        OutputPanes_WarningMessageTextColor,

        /* Debugger Log Window */

        Debugger_LogWindow_LogInput,
        Debugger_LogWindow_LogStatus,
        Debugger_LogWindow_LogTime,

        /* Debugger Watch Item */

        Debugger_WatchItem_ValueNormal,
        Debugger_WatchItem_ValueInvalid,
        Debugger_WatchItem_ValueChanged,

        /* Welcome Plugin */

        Welcome_TextColorNormal,
        Welcome_TextColorHeading,  // #535353 // Sessions, Recent Projects
        Welcome_BackgroundColorNormal,   // #ffffff
        Welcome_DividerColor,      // #737373
        Welcome_Button_BorderColorNormal,
        Welcome_Button_BorderColorPressed,
        Welcome_Button_TextColorNormal,
        Welcome_Button_TextColorPressed,
        Welcome_Link_TextColorNormal,
        Welcome_Link_TextColorActive,
        Welcome_Link_BackgroundColor,
        Welcome_Caption_TextColorNormal,
        Welcome_SideBar_BackgroundColor,

        Welcome_ProjectItem_TextColorFilepath,
        Welcome_ProjectItem_BackgroundColorHover,

        Welcome_SessionItem_BackgroundColorNormal,
        Welcome_SessionItem_BackgroundColorHover,
        Welcome_SessionItemExpanded_BackgroundColorNormal,
        Welcome_SessionItemExpanded_BackgroundColorHover,

        /* VcsBase Plugin */
        VcsBase_FileStatusUnknown_TextColor,
        VcsBase_FileAdded_TextColor,
        VcsBase_FileModified_TextColor,
        VcsBase_FileDeleted_TextColor,
        VcsBase_FileRenamed_TextColor,

        /* Bookmarks Plugin */
        Bookmarks_TextMarkColor,

        /* TextEditor Plugin */
        TextEditor_SearchResult_ScrollBarColor,
        TextEditor_CurrentLine_ScrollBarColor,

        /* Debugger Plugin */
        Debugger_Breakpoint_TextMarkColor,

        /* ProjectExplorer Plugin */
        ProjectExplorer_TaskError_TextMarkColor,
        ProjectExplorer_TaskWarn_TextMarkColor
    };

    enum Gradient {
        DetailsWidgetHeaderGradient,
        Welcome_Button_GradientNormal,
        Welcome_Button_GradientPressed
    };

    enum ImageFile {
        ProjectExplorerHeader,
        ProjectExplorerSource,
        ProjectExplorerForm,
        ProjectExplorerResource,
        ProjectExplorerQML,
        ProjectExplorerOtherFiles,
        ProjectFileIcon,
        IconOverlayCSource,
        IconOverlayCppHeader,
        IconOverlayCppSource,
        IconOverlayPri,
        IconOverlayPrf,
        IconOverlayPro,
        StandardPixmapFileIcon,
        StandardPixmapDirIcon,
        BuildStepDisable,
        BuildStepRemove,
        BuildStepMoveDown,
        BuildStepMoveUp
    };

    enum Flag {
        DrawTargetSelectorBottom,
        DrawSearchResultWidgetFrame,
        DrawProgressBarSunken,
        DrawIndicatorBranch,
        ComboBoxDrawTextShadow,
        DerivePaletteFromTheme,
        ApplyThemePaletteGlobally
    };

    enum WidgetStyle {
        StyleDefault,
        StyleFlat
    };

    WidgetStyle widgetStyle() const;
    bool flag(Flag f) const;
    QColor color(Color role) const;
    QString imageFile(ImageFile imageFile, const QString &fallBack) const;
    QGradientStops gradient(Gradient role) const;
    QPalette palette() const;
    QStringList preferredStyles() const;

    QString filePath() const;
    QString name() const;
    void setName(const QString &name);

    QVariantHash values() const;

    void writeSettings(const QString &filename) const;
    void readSettings(QSettings &settings);

    static QPalette initialPalette();

    ThemePrivate *d;

private:
    QPair<QColor, QString> readNamedColor(const QString &color) const;
};

QTCREATOR_UTILS_EXPORT Theme *creatorTheme();

} // namespace Utils

#endif // THEME_H
