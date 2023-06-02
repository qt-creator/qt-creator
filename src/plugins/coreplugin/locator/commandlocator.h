// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core {

/* Command locators: Provides completion for a set of
 * Core::Command's by sub-string of their action's text. */
class Command;

class CORE_EXPORT CommandLocator : public ILocatorFilter
{
public:
    CommandLocator(Utils::Id id, const QString &displayName, const QString &shortCutString,
                   QObject *parent = nullptr);
    void appendCommand(Command *cmd) { m_commands.push_back(cmd); }

private:
    LocatorMatcherTasks matchers() final;

    QList<Command *> m_commands;
};

} // namespace Core
