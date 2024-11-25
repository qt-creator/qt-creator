// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <KSyntaxHighlighting/Definition>

#include <QFuture>
#include <QTextDocument>

namespace Utils { class FilePath; }

namespace TextEditor {
class TextDocument;

namespace HighlighterHelper {

using Definition = KSyntaxHighlighting::Definition;
using Definitions = QList<Definition>;

Definition definitionForName(const QString &name);

Definitions definitionsForDocument(const TextDocument *document);
Definitions definitionsForMimeType(const QString &mimeType);
Definitions definitionsForFileName(const Utils::FilePath &fileName);

void rememberDefinitionForDocument(const Definition &definition, const TextDocument *document);
void clearDefinitionForDocumentCache();

void addCustomHighlighterPath(const Utils::FilePath &path);
void downloadDefinitions(std::function<void()> callback = nullptr);
void reload();

void handleShutdown();

QFuture<QTextDocument *> highlightCode(const QString &code, const QString &mimeType);

} // namespace HighlighterHelper

} // namespace TextEditor
