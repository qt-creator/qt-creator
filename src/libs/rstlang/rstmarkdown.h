// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstdocument.h"

#include <QHash>
#include <QList>
#include <QString>

namespace RstLang {

class RSTLANG_EXPORT MarkdownOptions
{
public:
    // What a "|name|" stands for.  Document::substitutionTexts() has what a
    // document spells out itself.
    QHash<QString, QString> substitutions;

    // The heading the outermost section is written as.
    int headingLevel = 3;

    // The language a code block that names none is marked with.
    QString defaultLanguage;
};

// The document as Markdown, which is what the tooltips and the completion
// proposals of the editor show.
RSTLANG_EXPORT QString toMarkdown(const DocumentPtr &document,
                                  const MarkdownOptions &options = {});
RSTLANG_EXPORT QString toMarkdown(ListView<BlockAST *> blocks,
                                  const MarkdownOptions &options = {});
RSTLANG_EXPORT QString toMarkdown(const QList<BlockAST *> &blocks,
                                  const MarkdownOptions &options = {});

// What a tooltip has room for: the title of the document, the first
// paragraph of it and the first block of code.
RSTLANG_EXPORT QString briefMarkdown(const DocumentPtr &document,
                                     const MarkdownOptions &options = {});
RSTLANG_EXPORT QString briefMarkdown(AST *node, const MarkdownOptions &options = {});

// The text of a parsed literal block, whose layout stands the way it is
// written while its inline markup is markup: what a reference and a role
// point at is dropped, what they say is kept.
RSTLANG_EXPORT QString inlineToPlainText(QStringView text,
                                         const MarkdownOptions &options = {});

// One line of text, the inline markup of reStructuredText turned into the
// markup of Markdown.
RSTLANG_EXPORT QString inlineToMarkdown(QStringView text,
                                        const MarkdownOptions &options = {});

} // namespace RstLang
