/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef COREICONS_H
#define COREICONS_H

#include <utils/icon.h>

namespace Core {
namespace Icons {

const Utils::Icon NEWFILE(
        QLatin1String(":/core/images/filenew.png"));
const Utils::Icon OPENFILE(
        QLatin1String(":/core/images/fileopen.png"));
const Utils::Icon SAVEFILE(
        QLatin1String(":/core/images/filesave.png"));
const Utils::Icon UNDO(
        QLatin1String(":/core/images/undo.png"));
const Utils::Icon REDO(
        QLatin1String(":/core/images/redo.png"));
const Utils::Icon COPY(
        QLatin1String(":/core/images/editcopy.png"));
const Utils::Icon PASTE(
        QLatin1String(":/core/images/editpaste.png"));
const Utils::Icon CUT(
        QLatin1String(":/core/images/editcut.png"));
const Utils::Icon DIR(
        QLatin1String(":/core/images/dir.png"));
const Utils::Icon RESET(
        QLatin1String(":/core/images/reset.png"));
const Utils::Icon DARK_CLOSE(
        QLatin1String(":/core/images/darkclose.png"));
const Utils::Icon LOCKED(
        QLatin1String(":/core/images/locked.png"));
const Utils::Icon UNLOCKED(
        QLatin1String(":/core/images/unlocked.png"));
const Utils::Icon FIND_CASE_INSENSITIVELY(
        QLatin1String(":/find/images/casesensitively.png"));
const Utils::Icon FIND_WHOLE_WORD(
        QLatin1String(":/find/images/wholewords.png"));
const Utils::Icon FIND_REGEXP(
        QLatin1String(":/find/images/regexp.png"));
const Utils::Icon FIND_PRESERVE_CASE(
        QLatin1String(":/find/images/preservecase.png"));
const Utils::Icon QTLOGO_32(
        QLatin1String(":/core/images/logo/32/QtProject-qtcreator.png"));
const Utils::Icon QTLOGO_64(
        QLatin1String(":/core/images/logo/64/QtProject-qtcreator.png"));
const Utils::Icon QTLOGO_128(
        QLatin1String(":/core/images/logo/128/QtProject-qtcreator.png"));

const Utils::Icon ARROW_UP({
        {QLatin1String(":/core/images/arrowup.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon ARROW_DOWN({
        {QLatin1String(":/core/images/arrowdown.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MINUS({
        {QLatin1String(":/core/images/minus.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon PLUS({
        {QLatin1String(":/core/images/plus.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon NEXT({
        {QLatin1String(":/core/images/next.png"), Utils::Theme::IconsNavigationArrowsColor}});
const Utils::Icon PREV({
        {QLatin1String(":/core/images/prev.png"), Utils::Theme::IconsNavigationArrowsColor}});
const Utils::Icon MAGNIFIER({
        {QLatin1String(":/core/images/magnifier.png"), Utils::Theme::BackgroundColorHover}}, Utils::Icon::Style::Tinted);
const Utils::Icon CLEAN_PANE({
        {QLatin1String(":/core/images/clean_pane_small.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon RELOAD({
        {QLatin1String(":/core/images/reload_gray.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon TOGGLE_SIDEBAR({
        {QLatin1String(":/core/images/sidebaricon.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_TOOLBAR({
        {QLatin1String(":/core/images/close.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_FOREGROUND({
        {QLatin1String(":/core/images/close.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Style::Tinted);
const Utils::Icon CLOSE_BACKGROUND({
        {QLatin1String(":/core/images/close.png"), Utils::Theme::PanelTextColorLight}}, Utils::Icon::Style::Tinted);
const Utils::Icon SPLIT_HORIZONTAL({
        {QLatin1String(":/core/images/splitbutton_horizontal.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon SPLIT_VERTICAL({
        {QLatin1String(":/core/images/splitbutton_vertical.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_SPLIT_TOP({
        {QLatin1String(":/core/images/splitbutton_closetop.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_SPLIT_BOTTOM({
        {QLatin1String(":/core/images/splitbutton_closebottom.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_SPLIT_LEFT({
        {QLatin1String(":/core/images/splitbutton_closeleft.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon CLOSE_SPLIT_RIGHT({
        {QLatin1String(":/core/images/splitbutton_closeright.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon FILTER({
        {QLatin1String(":/core/images/filtericon.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon LINK({
        {QLatin1String(":/core/images/linkicon.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon WARNING({
        {QLatin1String(":/core/images/warning.png"), Utils::Theme::IconsWarningColor}});
const Utils::Icon ERROR({
        {QLatin1String(":/core/images/error.png"), Utils::Theme::IconsErrorColor}});
const Utils::Icon INFO({
        {QLatin1String(":/core/images/info.png"), Utils::Theme::IconsInfoColor}});
const Utils::Icon EXPAND({
        {QLatin1String(":/find/images/expand.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon DEBUG_START_SMALL({
        {QLatin1String(":/core/images/debugger_overlay_small.png"), Utils::Theme::IconsDebugColor},
        {QLatin1String(":/core/images/run_overlay_small.png"), Utils::Theme::IconsRunColor}});
const Utils::Icon DEBUG_EXIT_SMALL({
        {QLatin1String(":/core/images/debugger_overlay_small.png"), Utils::Theme::IconsDebugColor},
        {QLatin1String(":/core/images/stop_overlay_small.png"), Utils::Theme::IconsStopColor}});
const Utils::Icon DEBUG_INTERRUPT_SMALL({
        {QLatin1String(":/core/images/debugger_overlay_small.png"), Utils::Theme::IconsDebugColor},
        {QLatin1String(":/core/images/interrupt_overlay_small.png"), Utils::Theme::IconsInterruptColor}});
const Utils::Icon DEBUG_CONTINUE_SMALL({
        {QLatin1String(":/core/images/debugger_overlay_small.png"), Utils::Theme::IconsDebugColor},
        {QLatin1String(":/core/images/continue_overlay_small.png"), Utils::Theme::IconsRunColor}});
const Utils::Icon ZOOM({
        {QLatin1String(":/core/images/zoom.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon TOOLBAR_EXTENSION({
        {QLatin1String(":/core/images/extension.png"), Utils::Theme::IconsBaseColor}});

const Utils::Icon MODE_EDIT_CLASSIC(
        QLatin1String(":/fancyactionbar/images/mode_Edit.png"));
const Utils::Icon MODE_EDIT_FLAT({
        {QLatin1String(":/fancyactionbar/images/mode_edit_mask.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_EDIT_FLAT_ACTIVE({
        {QLatin1String(":/fancyactionbar/images/mode_edit_mask.png"), Utils::Theme::IconsModeEditActiveColor}});
const Utils::Icon MODE_DESIGN_CLASSIC(
        QLatin1String(":/fancyactionbar/images/mode_Design.png"));
const Utils::Icon MODE_DESIGN_FLAT({
        {QLatin1String(":/fancyactionbar/images/mode_design_mask.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_DESIGN_FLAT_ACTIVE({
        {QLatin1String(":/fancyactionbar/images/mode_design_mask.png"), Utils::Theme::IconsModeDesignActiveColor}});

} // namespace Icons
} // namespace Core

#endif // COREICONS_H
