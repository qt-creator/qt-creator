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

#pragma once

#include "icon.h"

namespace Utils {
namespace Icons {

const Utils::Icon EDIT_CLEAR({
        {QLatin1String(":/core/images/editclear.png"), Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint);
const Utils::Icon LOCKED_TOOLBAR({
        {QLatin1String(":/utils/images/locked.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon LOCKED({
        {QLatin1String(":/utils/images/locked.png"), Utils::Theme::PanelTextColorDark}}, Icon::Tint);
const Utils::Icon UNLOCKED_TOOLBAR({
        {QLatin1String(":/utils/images/unlocked.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon NEXT({
        {QLatin1String(":/utils/images/next.png"), Utils::Theme::IconsWarningColor}}, Utils::Icon::MenuTintedStyle);
const Utils::Icon NEXT_TOOLBAR({
        {QLatin1String(":/utils/images/next.png"), Utils::Theme::IconsNavigationArrowsColor}});
const Utils::Icon PREV({
        {QLatin1String(":/utils/images/prev.png"), Utils::Theme::IconsWarningColor}}, Utils::Icon::MenuTintedStyle);
const Utils::Icon PREV_TOOLBAR({
        {QLatin1String(":/utils/images/prev.png"), Utils::Theme::IconsNavigationArrowsColor}});
const Utils::Icon ZOOM({
        {QLatin1String(":/utils/images/zoom.png"), Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint);
const Utils::Icon ZOOM_TOOLBAR({
        {QLatin1String(":/utils/images/zoom.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon OK({
        {QLatin1String(":/utils/images/ok.png"), Utils::Theme::IconsRunToolBarColor}}, Icon::Tint);
const Utils::Icon NOTLOADED({
        {QLatin1String(":/utils/images/notloaded.png"), Utils::Theme::IconsErrorColor}}, Icon::Tint);
const Utils::Icon ERROR({
        {QLatin1String(":/utils/images/error.png"), Utils::Theme::IconsErrorColor}}, Icon::Tint);

} // namespace Icons
} // namespace Utils
