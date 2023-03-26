// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "keyword.h"
#include <utils/theme/theme.h>

namespace Todo {
namespace Internal {

Keyword::Keyword() : color(Utils::creatorTheme()->color(Utils::Theme::TextColorNormal))
{
}

bool Keyword::equals(const Keyword &other) const
{
    return (this->name == other.name)
        && (this->iconType == other.iconType)
        && (this->color == other.color);
}

bool operator ==(const Keyword &k1, const Keyword &k2)
{
    return k1.equals(k2);
}

bool operator !=(const Keyword &k1, const Keyword &k2)
{
    return !k1.equals(k2);
}

} // namespace Internal
} // namespace Todo
