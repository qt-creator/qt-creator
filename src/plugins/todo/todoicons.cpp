// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoicons.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

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
