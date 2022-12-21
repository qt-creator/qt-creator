// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QIcon>

namespace Todo {
namespace Internal {

enum class IconType {
    Info,
    Error,
    Warning,
    Bug,
    Todo
};

QIcon icon(IconType type);
QIcon toolBarIcon(IconType type);

} // namespace Internal
} // namespace Todo
