/****************************************************************************
**
** Copyright (C) 2014 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef THEME_H
#define THEME_H

#include "../utils_global.h"

#include <QStyle>
#include <QFlags>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Utils {

class ThemePrivate;

class QTCREATOR_UTILS_EXPORT Theme : public QObject
{
    Q_OBJECT

    Q_ENUMS(ColorRole)
    Q_ENUMS(ImageFile)
    Q_ENUMS(GradientRole)
    Q_ENUMS(MimeType)
    Q_ENUMS(Flag)
    Q_ENUMS(WidgetStyle)

public:
    Theme(QObject *parent = 0);
    ~Theme();

    enum ColorRole {
        BackgroundColorAlternate,
        BackgroundColorDark,
        BackgroundColorHover,
        BackgroundColorNormal,
        BackgroundColorSelected,
        BadgeLabelBackgroundColorChecked,
        BadgeLabelBackgroundColorUnchecked,
        BadgeLabelTextColorChecked,
        BadgeLabelTextColorUnchecked,
        CanceledSearchTextColor,
        ComboBoxArrowColor,
        ComboBoxArrowColorDisabled,
        ComboBoxTextColor,
        DetailsWidgetBackgroundColor,
        DetailsButtonBackgroundColorHover,
        DockWidgetResizeHandleColor,
        DoubleTabWidget1stEmptyAreaBackgroundColor,
        DoubleTabWidget1stSeparatorColor,
        DoubleTabWidget2ndSeparatorColor,
        DoubleTabWidget1stTabBackgroundColor,
        DoubleTabWidget2ndTabBackgroundColor,
        DoubleTabWidget1stTabActiveTextColor,
        DoubleTabWidget1stTabInactiveTextColor,
        DoubleTabWidget2ndTabActiveTextColor,
        DoubleTabWidget2ndTabInactiveTextColor,
        EditorPlaceholderColor,
        FancyTabBarBackgroundColor,
        FancyTabWidgetEnabledSelectedTextColor,
        FancyTabWidgetEnabledUnselectedTextColor,
        FancyTabWidgetDisabledSelectedTextColor,
        FancyTabWidgetDisabledUnselectedTextColor,
        FancyToolButtonHoverColor,
        FancyToolButtonSelectedColor,
        FutureProgressBackgroundColor,
        MenuBarEmptyAreaBackgroundColor,
        MenuBarItemBackgroundColor,
        MenuBarItemTextColorDisabled,
        MenuBarItemTextColorNormal,
        MiniProjectTargetSelectorBackgroundColor,
        MiniProjectTargetSelectorBorderColor,
        MiniProjectTargetSelectorSummaryBackgroundColor,
        MiniProjectTargetSelectorTextColor,
        OutputPaneButtonFlashColor,
        OutputPaneToggleButtonTextColorChecked,
        OutputPaneToggleButtonTextColorUnchecked,
        PanelButtonToolBackgroundColorHover,
        PanelTextColor,
        PanelsWidgetSeparatorLineColor,
        PanelStatusBarBackgroundColor,
        ProgressBarTitleColor,
        ProgressBarColorError,
        ProgressBarColorFinished,
        ProgressBarColorNormal,
        SearchResultWidgetBackgroundColor,
        SearchResultWidgetTextColor,
        TextColorDisabled,
        TextColorHighlight,
        TextColorNormal,
        TodoItemTextColor,
        ToggleButtonBackgroundColor,
        ToolBarBackgroundColor,
        TreeViewArrowColorNormal,
        TreeViewArrowColorSelected,

        OutputFormatter_NormalMessageTextColor,
        OutputFormatter_ErrorMessageTextColor,
        OutputFormatter_StdOutTextColor,
        OutputFormatter_StdErrTextColor,
        OutputFormatter_DebugTextColor,

        QtOutputFormatter_LinkTextColor,

        /* Welcome Plugin */

        Welcome_TextColorNormal,
        Welcome_TextColorHeading,  // #535353 // Sessions, Recent Projects
        Welcome_BackgroundColorNormal,   // #ffffff
        Welcome_DividerColor,      // #737373
        Welcome_Button_BorderColor,
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
        Welcome_SessionItemExpanded_BackgroundColorHover
    };

    enum GradientRole {
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
        StandardPixmapDirIcon
    };

    enum Flag {
        DrawTargetSelectorBottom,
        DrawSearchResultWidgetFrame,
        DrawProgressBarSunken,
        DrawIndicatorBranch,
        ComboBoxDrawTextShadow,
        DerivePaletteFromTheme
    };

    enum WidgetStyle {
        StyleDefault,
        StyleFlat
    };

    WidgetStyle widgetStyle() const;
    bool flag(Flag f) const;
    QColor color(ColorRole role) const;
    QString imageFile(ImageFile imageFile, const QString &fallBack) const;
    QGradientStops gradient(GradientRole role) const;
    QPalette palette(const QPalette &base) const;

    QString fileName() const;
    void setName(const QString &name);

    void writeSettings(const QString &filename) const;
    void readSettings(QSettings &settings);
    ThemePrivate *d;

signals:
    void changed();

private:
    QPair<QColor, QString> readNamedColor(const QString &color) const;
};

QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
QTCREATOR_UTILS_EXPORT void setCreatorTheme(Theme *theme);

} // namespace Utils

#endif // THEME_H
