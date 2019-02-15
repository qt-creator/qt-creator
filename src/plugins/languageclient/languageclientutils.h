/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <languageserverprotocol/workspace.h>
#include <languageserverprotocol/languagefeatures.h>

#include <texteditor/refactoroverlay.h>

namespace TextEditor {
class TextDocument;
class TextDocumentManipulatorInterface;
} // namespace TextEditor

namespace LanguageClient {

class Client;

bool applyWorkspaceEdit(const LanguageServerProtocol::WorkspaceEdit &edit);
bool applyTextDocumentEdit(const LanguageServerProtocol::TextDocumentEdit &edit);
bool applyTextEdits(const LanguageServerProtocol::DocumentUri &uri,
                    const QList<LanguageServerProtocol::TextEdit> &edits);
void applyTextEdit(TextEditor::TextDocumentManipulatorInterface &manipulator,
                   const LanguageServerProtocol::TextEdit &edit);
void updateCodeActionRefactoringMarker(Client *client,
                                       const LanguageServerProtocol::CodeAction &action,
                                       const LanguageServerProtocol::DocumentUri &uri);

} // namespace LanguageClient
