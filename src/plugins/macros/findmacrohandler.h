// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imacrohandler.h"

#include <coreplugin/find/textfindconstants.h>
#include <utils/filesearch.h>

namespace Core { class IEditor; }

namespace Macros {
namespace Internal {

class FindMacroHandler : public IMacroHandler
{
    Q_OBJECT

public:
    FindMacroHandler();

    void startRecording(Macro* macro) override;

    bool canExecuteEvent(const MacroEvent &macroEvent) override;
    bool executeEvent(const MacroEvent &macroEvent) override;

    void findIncremental(const QString &txt, Utils::FindFlags findFlags);
    void findStep(const QString &txt, Utils::FindFlags findFlags);
    void replace(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void replaceStep(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void replaceAll(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void resetIncrementalSearch();

private:
    void changeEditor(Core::IEditor *editor);
};

} // namespace Internal
} // namespace Macros
