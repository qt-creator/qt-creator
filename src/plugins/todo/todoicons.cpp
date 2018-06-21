/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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
#include <utils/icon.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include "todoicons.h"

using namespace Utils;

namespace Todo {
namespace Internal {

QIcon icon(IconType type)
{
    switch (type) {
    case IconType::Info: {
        const static QIcon icon = Utils::Icons::INFO.icon();
        return icon;
    }
    case IconType::Warning: {
        const static QIcon icon = Utils::Icons::WARNING.icon();
        return icon;
    }
    case IconType::Bug: {
        const static QIcon icon =
                Icon({
                         {":/todoplugin/images/bugfill.png", Theme::BackgroundColorNormal},
                         {":/todoplugin/images/bug.png", Theme::IconsInterruptColor}
                     }, Icon::Tint).icon();

        return icon;
    }
    case IconType::Todo: {
        const static QIcon icon =
                Icon({
                         {":/todoplugin/images/tasklist.png", Theme::IconsRunColor}
                     }, Icon::Tint).icon();
        return icon;
    }

    default:
    case IconType::Error: {
        const static QIcon icon = Utils::Icons::CRITICAL.icon();
        return icon;
    }
    }
}

QIcon toolBarIcon(IconType type)
{
    switch (type) {
    case IconType::Info:
        return Icons::INFO_TOOLBAR.icon();
    case IconType::Warning:
        return Icons::WARNING_TOOLBAR.icon();
    case IconType::Bug:
        return Icon({{":/todoplugin/images/bug.png", Theme::IconsInterruptToolBarColor}}).icon();
    case IconType::Todo:
        return Icon({{":/todoplugin/images/tasklist.png", Theme::IconsRunToolBarColor}}).icon();
    default:
    case IconType::Error:
        return Icons::CRITICAL_TOOLBAR.icon();
    }
}

} // namespace Internal
} // namespace Todo
