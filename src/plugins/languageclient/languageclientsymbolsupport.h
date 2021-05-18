/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "languageclient_global.h"

#include <coreplugin/find/searchresultitem.h>
#include <texteditor/textdocument.h>

#include <languageserverprotocol/languagefeatures.h>

namespace Core {
class SearchResult;
class SearchResultItem;
}

namespace LanguageServerProtocol { class MessageId; }

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT SymbolSupport
{
    Q_DECLARE_TR_FUNCTIONS(SymbolSupport)
public:
    explicit SymbolSupport(Client *client);

    void findLinkAt(TextEditor::TextDocument *document,
                    const QTextCursor &cursor,
                    Utils::ProcessLinkCallback callback,
                    const bool resolveTarget);

    using ResultHandler = std::function<void(const QList<LanguageServerProtocol::Location> &)>;
    Utils::optional<LanguageServerProtocol::MessageId> findUsages(
            TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            const ResultHandler &handler = {});

    bool supportsRename(TextEditor::TextDocument *document);
    void renameSymbol(TextEditor::TextDocument *document, const QTextCursor &cursor);

    static Core::Search::TextRange convertRange(const LanguageServerProtocol::Range &range);
    static QStringList getFileContents(const Utils::FilePath &filePath);

private:
    void handleFindReferencesResponse(
        const LanguageServerProtocol::FindReferencesRequest::Response &response,
        const QString &wordUnderCursor,
        const ResultHandler &handler);

    void requestPrepareRename(const LanguageServerProtocol::TextDocumentPositionParams &params,
                              const QString &placeholder);
    void requestRename(const LanguageServerProtocol::TextDocumentPositionParams &positionParams,
                       const QString &newName, Core::SearchResult *search);
    void startRenameSymbol(const LanguageServerProtocol::TextDocumentPositionParams &params,
                           const QString &placeholder);
    void handleRenameResponse(Core::SearchResult *search,
                              const LanguageServerProtocol::RenameRequest::Response &response);
    void applyRename(const QList<Core::SearchResultItem> &checkedItems);

    Client *m_client = nullptr;
};

} // namespace LanguageClient
