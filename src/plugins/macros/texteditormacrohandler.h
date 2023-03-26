// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imacrohandler.h"

namespace Core { class IEditor; }

namespace TextEditor { class BaseTextEditor; }

namespace Macros {
namespace Internal {

class TextEditorMacroHandler : public IMacroHandler
{
    Q_OBJECT

public:
    TextEditorMacroHandler();

    void startRecording(Macro *macro) override;
    void endRecordingMacro(Macro *macro) override;

    bool canExecuteEvent(const MacroEvent &macroEvent) override;
    bool executeEvent(const MacroEvent &macroEvent) override;

    bool eventFilter(QObject *watched, QEvent *event) override;

    void changeEditor(Core::IEditor *editor);
    void closeEditor(Core::IEditor *editor);

private:
    TextEditor::BaseTextEditor *m_currentEditor = nullptr;
};

} // namespace Internal
} // namespace Macros
