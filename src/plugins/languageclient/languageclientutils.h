// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lsptypes.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/link.h>

#include <texteditor/refactoroverlay.h>
#include <utils/changeset.h>

namespace Core { class IEditor; }

namespace TextEditor {
class TextDocument;
class TextDocumentManipulator;
} // namespace TextEditor


namespace LanguageClient {

class Client;

enum class Schedule { Now, Delayed };

LANGUAGECLIENT_EXPORT Utils::ChangeSet editsToChangeSet(
    const QList<LanguageServerProtocol::TextEdit> &edits, const QTextDocument *doc);
bool LANGUAGECLIENT_EXPORT
applyWorkspaceEdit(const Client *client, const LanguageServerProtocol::WorkspaceEdit &edit);
bool LANGUAGECLIENT_EXPORT
applyTextDocumentEdit(const Client *client, const LanguageServerProtocol::TextDocumentEdit &edit);
bool LANGUAGECLIENT_EXPORT applyTextEdits(
    const Client *client, const QString &uri, const QList<LanguageServerProtocol::TextEdit> &edits);
bool LANGUAGECLIENT_EXPORT applyTextEdits(
    const Client *client,
    const Utils::FilePath &filePath,
    const QList<LanguageServerProtocol::TextEdit> &edits);
bool LANGUAGECLIENT_EXPORT applyDocumentChange(
    const Client *client, const LanguageServerProtocol::WorkspaceEditDocumentChangesItem &change);
void LANGUAGECLIENT_EXPORT applyTextEdit(
    TextEditor::TextEditorWidget *editorWidget,
    const LanguageServerProtocol::TextEdit &edit,
    bool newTextIsSnippet = false);
/// The place \a location denotes on the host.
Utils::Link LANGUAGECLIENT_EXPORT
linkFor(const Client *client, const LanguageServerProtocol::Location &location);
void LANGUAGECLIENT_EXPORT updateCodeActionRefactoringMarker(
    Client *client, const QList<LanguageServerProtocol::CodeAction> &actions, const QString &uri);
/// Whether a dynamic registration made with \a registrationOptions covers the document.
bool LANGUAGECLIENT_EXPORT registrationApplies(const QJsonValue &registrationOptions,
                                               const Utils::FilePath &filePath,
                                               const QString &mimeType = {});
void updateEditorToolBar(Core::IEditor *editor);
LANGUAGECLIENT_EXPORT const QIcon symbolIcon(int type, const QList<int> &tags);

void autoSetupLanguageServer(TextEditor::TextDocument *document);

} // namespace LanguageClient
