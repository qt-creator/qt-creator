// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "syntaxhighlighter.h"

#include <KSyntaxHighlighting/AbstractHighlighter>

#include <QSet>

#include <optional>

namespace KSyntaxHighlighting { class Repository; }

namespace TextEditor {
class TextDocument;

class Highlighter : public SyntaxHighlighter, public KSyntaxHighlighting::AbstractHighlighter
{
    Q_OBJECT
    Q_INTERFACES(KSyntaxHighlighting::AbstractHighlighter)
public:
    Highlighter();
    ~Highlighter() override;

    void setDefinition(const KSyntaxHighlighting::Definition &definition) override;

protected:
    void highlightBlock(const QString &text) override;
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;
    void applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region) override;

private:
    bool definitionMarksProse(const KSyntaxHighlighting::Format &format);

    std::optional<QSet<int>> m_proseMarkingFormats;
};

} // namespace TextEditor
