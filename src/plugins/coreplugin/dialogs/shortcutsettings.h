// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../actionmanager/commandmappings.h"
#include "ioptionspage.h"

namespace Core {

class Command;

namespace Internal {

struct ShortcutItem
{
    Command *m_cmd;
    QList<QKeySequence> m_keys;
    QTreeWidgetItem *m_item;
};

class ShortcutSettings final : public IOptionsPage
{
public:
    ShortcutSettings();
};

} // namespace Internal
} // namespace Core
