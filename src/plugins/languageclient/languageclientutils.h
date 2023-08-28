// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/workspace.h>

#include <texteditor/refactoroverlay.h>
#include <utils/changeset.h>

namespace Core { class IEditor; }

namespace TextEditor {
class TextDocument;
class TextDocumentManipulatorInterface;
} // namespace TextEditor

namespace LanguageClient {

class Client;

enum class Schedule { Now, Delayed };

Utils::ChangeSet editsToChangeSet(const QList<LanguageServerProtocol::TextEdit> &edits,
                                  const QTextDocument *doc);
bool LANGUAGECLIENT_EXPORT applyWorkspaceEdit(const Client *client, const LanguageServerProtocol::WorkspaceEdit &edit);
bool LANGUAGECLIENT_EXPORT
applyTextDocumentEdit(const Client *client, const LanguageServerProtocol::TextDocumentEdit &edit);
bool LANGUAGECLIENT_EXPORT applyTextEdits(const Client *client,
                                          const LanguageServerProtocol::DocumentUri &uri,
                                          const QList<LanguageServerProtocol::TextEdit> &edits);
bool LANGUAGECLIENT_EXPORT applyTextEdits(const Client *client,
                                          const Utils::FilePath &filePath,
                                          const QList<LanguageServerProtocol::TextEdit> &edits);
bool LANGUAGECLIENT_EXPORT applyDocumentChange(const Client *client,
                                               const LanguageServerProtocol::DocumentChange &change);
void LANGUAGECLIENT_EXPORT applyTextEdit(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                         const LanguageServerProtocol::TextEdit &edit,
                                         bool newTextIsSnippet = false);
void LANGUAGECLIENT_EXPORT
updateCodeActionRefactoringMarker(Client *client,
                                  const QList<LanguageServerProtocol::CodeAction> &actions,
                                  const LanguageServerProtocol::DocumentUri &uri);
void updateEditorToolBar(Core::IEditor *editor);
const QIcon LANGUAGECLIENT_EXPORT symbolIcon(int type);

} // namespace LanguageClient
