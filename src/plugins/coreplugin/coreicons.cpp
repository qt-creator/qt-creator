/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "coreicons.h"

using namespace Utils;

namespace Core {
namespace Icons {

const Icon NEWFILE(
        QLatin1String(":/core/images/filenew.png"));
const Icon OPENFILE(
        QLatin1String(":/core/images/fileopen.png"));
const Icon SAVEFILE(
        QLatin1String(":/core/images/filesave.png"));
const Icon UNDO(
        QLatin1String(":/core/images/undo.png"));
const Icon REDO(
        QLatin1String(":/core/images/redo.png"));
const Icon COPY(
        QLatin1String(":/core/images/editcopy.png"));
const Icon PASTE(
        QLatin1String(":/core/images/editpaste.png"));
const Icon CUT(
        QLatin1String(":/core/images/editcut.png"));
const Icon DIR(
        QLatin1String(":/core/images/dir.png"));
const Icon RESET(
        QLatin1String(":/core/images/reset.png"));
const Icon DARK_CLOSE(
        QLatin1String(":/core/images/darkclose.png"));
const Icon LOCKED(
        QLatin1String(":/core/images/locked.png"));
const Icon UNLOCKED(
        QLatin1String(":/core/images/unlocked.png"));
const Icon FIND_CASE_INSENSITIVELY(
        QLatin1String(":/find/images/casesensitively.png"));
const Icon FIND_WHOLE_WORD(
        QLatin1String(":/find/images/wholewords.png"));
const Icon FIND_REGEXP(
        QLatin1String(":/find/images/regexp.png"));
const Icon FIND_PRESERVE_CASE(
        QLatin1String(":/find/images/preservecase.png"));
const Icon QTLOGO_32(
        QLatin1String(":/core/images/logo/32/QtProject-qtcreator.png"));
const Icon QTLOGO_64(
        QLatin1String(":/core/images/logo/64/QtProject-qtcreator.png"));
const Icon QTLOGO_128(
        QLatin1String(":/core/images/logo/128/QtProject-qtcreator.png"));

const Icon ARROW_UP({
        {QLatin1String(":/core/images/arrowup.png"), Theme::IconsBaseColor}});
const Icon ARROW_DOWN({
        {QLatin1String(":/core/images/arrowdown.png"), Theme::IconsBaseColor}});
const Icon MINUS({
        {QLatin1String(":/core/images/minus.png"), Theme::IconsBaseColor}});
const Icon PLUS({
        {QLatin1String(":/core/images/plus.png"), Theme::IconsBaseColor}});
const Icon NEXT({
        {QLatin1String(":/core/images/next.png"), Theme::IconsNavigationArrowsColor}});
const Icon PREV({
        {QLatin1String(":/core/images/prev.png"), Theme::IconsNavigationArrowsColor}});
const Icon MAGNIFIER({
        {QLatin1String(":/core/images/magnifier.png"), Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLEAN_PANE({
        {QLatin1String(":/core/images/clean_pane_small.png"), Theme::IconsBaseColor}});
const Icon RELOAD({
        {QLatin1String(":/core/images/reload_gray.png"), Theme::IconsBaseColor}});
const Icon TOGGLE_SIDEBAR({
        {QLatin1String(":/core/images/sidebaricon.png"), Theme::IconsBaseColor}});
const Icon CLOSE_TOOLBAR({
        {QLatin1String(":/core/images/close.png"), Theme::IconsBaseColor}});
const Icon CLOSE_FOREGROUND({
        {QLatin1String(":/core/images/close.png"), Theme::PanelTextColorDark}}, Icon::Tint);
const Icon CLOSE_BACKGROUND({
        {QLatin1String(":/core/images/close.png"), Theme::PanelTextColorLight}}, Icon::Tint);
const Icon SPLIT_HORIZONTAL({
        {QLatin1String(":/core/images/splitbutton_horizontal.png"), Theme::IconsBaseColor}});
const Icon SPLIT_VERTICAL({
        {QLatin1String(":/core/images/splitbutton_vertical.png"), Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_TOP({
        {QLatin1String(":/core/images/splitbutton_closetop.png"), Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_BOTTOM({
        {QLatin1String(":/core/images/splitbutton_closebottom.png"), Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_LEFT({
        {QLatin1String(":/core/images/splitbutton_closeleft.png"), Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_RIGHT({
        {QLatin1String(":/core/images/splitbutton_closeright.png"), Theme::IconsBaseColor}});
const Icon FILTER({
        {QLatin1String(":/core/images/filtericon.png"), Theme::IconsBaseColor}});
const Icon LINK({
        {QLatin1String(":/core/images/linkicon.png"), Theme::IconsBaseColor}});
const Icon WARNING({
        {QLatin1String(":/core/images/warningfill.png"), Theme::BackgroundColorNormal},
        {QLatin1String(":/core/images/warning.png"), Theme::IconsWarningColor}}, Icon::Tint);
const Icon WARNING_TOOLBAR({
        {QLatin1String(":/core/images/warning.png"), Theme::IconsWarningToolBarColor}});
const Icon ERROR({
        {QLatin1String(":/core/images/warningfill.png"), Theme::BackgroundColorNormal},
        {QLatin1String(":/core/images/error.png"), Theme::IconsErrorColor}}, Icon::Tint);
const Icon ERROR_TOOLBAR({
        {QLatin1String(":/core/images/error.png"), Theme::IconsErrorToolBarColor}});
const Icon ERROR_TASKBAR({
        {QLatin1String(":/core/images/compile_error_taskbar.png"), Theme::IconsErrorColor}}, Icon::Tint);
const Icon INFO({
        {QLatin1String(":/core/images/warningfill.png"), Theme::BackgroundColorNormal},
        {QLatin1String(":/core/images/info.png"), Theme::IconsInfoColor}}, Icon::Tint);
const Icon INFO_TOOLBAR({
        {QLatin1String(":/core/images/info.png"), Theme::IconsInfoToolBarColor}});
const Icon EXPAND({
        {QLatin1String(":/find/images/expand.png"), Theme::IconsBaseColor}});
const Icon ZOOM({
        {QLatin1String(":/core/images/zoom.png"), Theme::IconsBaseColor}});
const Icon TOOLBAR_EXTENSION({
        {QLatin1String(":/core/images/extension.png"), Theme::IconsBaseColor}});

const Icon MODE_EDIT_CLASSIC(
        QLatin1String(":/fancyactionbar/images/mode_Edit.png"));
const Icon MODE_EDIT_FLAT({
        {QLatin1String(":/fancyactionbar/images/mode_edit_mask.png"), Theme::IconsBaseColor}});
const Icon MODE_EDIT_FLAT_ACTIVE({
        {QLatin1String(":/fancyactionbar/images/mode_edit_mask.png"), Theme::IconsModeEditActiveColor}});
const Icon MODE_DESIGN_CLASSIC(
        QLatin1String(":/fancyactionbar/images/mode_Design.png"));
const Icon MODE_DESIGN_FLAT({
        {QLatin1String(":/fancyactionbar/images/mode_design_mask.png"), Theme::IconsBaseColor}});
const Icon MODE_DESIGN_FLAT_ACTIVE({
        {QLatin1String(":/fancyactionbar/images/mode_design_mask.png"), Theme::IconsModeDesignActiveColor}});

} // namespace Icons
} // namespace Core
