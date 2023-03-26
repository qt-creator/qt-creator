// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imacrohandler.h"

#include <coreplugin/actionmanager/actionmanager.h>

#include <QSet>

namespace Macros {
namespace Internal {

class ActionMacroHandler : public IMacroHandler
{
    Q_OBJECT

public:
    ActionMacroHandler();

    bool canExecuteEvent(const MacroEvent &macroEvent) override;
    bool executeEvent(const MacroEvent &macroEvent) override;

private:
    void registerCommand(Utils::Id id);
    Core::Command *command(const QString &id);
    void addCommand(Utils::Id id);

private:
    QSet<Utils::Id> m_commandIds;
};

} // namespace Internal
} // namespace Macros
