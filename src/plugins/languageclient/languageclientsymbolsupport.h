// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <coreplugin/find/searchresultitem.h>
#include <texteditor/textdocument.h>

#include <languageserverprotocol/languagefeatures.h>

#include <functional>

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
                    Utils::LinkHandler callback,
                    const bool resolveTarget);

    bool supportsFindUsages(TextEditor::TextDocument *document) const;
    using ResultHandler = std::function<void(const QList<LanguageServerProtocol::Location> &)>;
    std::optional<LanguageServerProtocol::MessageId> findUsages(
            TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            const ResultHandler &handler = {});

    bool supportsRename(TextEditor::TextDocument *document);
    void renameSymbol(TextEditor::TextDocument *document, const QTextCursor &cursor);

    static Core::Search::TextRange convertRange(const LanguageServerProtocol::Range &range);
    static QStringList getFileContents(const Utils::FilePath &filePath);

    using SymbolMapper = std::function<QString(const QString &)>;
    void setDefaultRenamingSymbolMapper(const SymbolMapper &mapper);

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
    SymbolMapper m_defaultSymbolMapper;
};

} // namespace LanguageClient
