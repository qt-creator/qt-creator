/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "syntaxhighlighter.h"

#include <utils/fileutils.h>

#include <AbstractHighlighter>
#include <Definition>

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
