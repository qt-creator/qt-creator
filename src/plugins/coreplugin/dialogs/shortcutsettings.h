// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../actionmanager/commandmappings.h"

namespace Core {

class Command;

namespace Internal {

struct ShortcutItem
{
    Command *m_cmd;
    QList<QKeySequence> m_keys;
    QTreeWidgetItem *m_item;
};

void setupShortcutSettings();

} // namespace Internal
} // namespace Core
