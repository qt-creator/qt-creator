/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <texteditor/indenter.h>

namespace ClangFormat {

class ClangFormatIndenter final : public TextEditor::Indenter
{
public:
    void indent(QTextDocument *doc,
                const QTextCursor &cursor,
                const QChar &typedChar,
                const TextEditor::TabSettings &tabSettings,
                bool autoTriggered = true) override;
    void reindent(QTextDocument *doc,
                  const QTextCursor &cursor,
                  const TextEditor::TabSettings &tabSettings) override;
    void format(QTextDocument *doc,
                const QTextCursor &cursor,
                const TextEditor::TabSettings &tabSettings) override;
    void indentBlock(QTextDocument *doc,
                     const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings) override;
    int indentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings) override;

    bool isElectricCharacter(const QChar &ch) const override;

    bool hasTabSettings() const override { return true; }
    TextEditor::TabSettings tabSettings() const override;
};

} // namespace ClangFormat
