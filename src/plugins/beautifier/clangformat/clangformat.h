// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../beautifierabstracttool.h"

#include "clangformatsettings.h"

namespace Beautifier::Internal {

class ClangFormat : public BeautifierAbstractTool
{
public:
    ClangFormat();

    QString id() const override;
    void updateActions(Core::IEditor *editor) override;
    TextEditor::Command command() const override;
    bool isApplicable(const Core::IDocument *document) const override;

private:
    void formatFile();
    void formatAtPosition(const int pos, const int length);
    void formatAtCursor();
    void formatLines();
    void disableFormattingSelectedText();
    TextEditor::Command command(int offset, int length) const;

    QAction *m_formatFile = nullptr;
    QAction *m_formatLines = nullptr;
    QAction *m_formatRange = nullptr;
    QAction *m_disableFormattingSelectedText = nullptr;
    ClangFormatSettings m_settings;
    ClangFormatOptionsPage m_page{&m_settings};
};

} // Beautifier::Internal
