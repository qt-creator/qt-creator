// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "syntaxhighlighter.h"

#include <utils/fileutils.h>

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>

namespace TextEditor {
class TextDocument;

class Highlighter : public SyntaxHighlighter, public KSyntaxHighlighting::AbstractHighlighter
{
    Q_OBJECT
    Q_INTERFACES(KSyntaxHighlighting::AbstractHighlighter)
public:
    using Definition = KSyntaxHighlighting::Definition;
    using Definitions = QList<Definition>;
    Highlighter();

    static Definition definitionForName(const QString &name);

    static Definitions definitionsForDocument(const TextDocument *document);
    static Definitions definitionsForMimeType(const QString &mimeType);
    static Definitions definitionsForFileName(const Utils::FilePath &fileName);

    static void rememberDefinitionForDocument(const Definition &definition,
                                              const TextDocument *document);
    static void clearDefinitionForDocumentCache();

    static void addCustomHighlighterPath(const Utils::FilePath &path);
    static void downloadDefinitions(std::function<void()> callback = nullptr);
    static void reload();

    static void handleShutdown();

protected:
    void highlightBlock(const QString &text) override;
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;
    void applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region) override;
};

} // namespace TextEditor
