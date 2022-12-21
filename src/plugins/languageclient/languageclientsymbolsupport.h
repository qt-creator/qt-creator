// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class LANGUAGECLIENT_EXPORT SymbolSupport : public QObject
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
    void renameSymbol(TextEditor::TextDocument *document, const QTextCursor &cursor,
                      const QString &newSymbolName = {}, bool preferLowerCaseFileNames = true);

    static Core::Search::TextRange convertRange(const LanguageServerProtocol::Range &range);
    static QStringList getFileContents(const Utils::FilePath &filePath);

    using SymbolMapper = std::function<QString(const QString &)>;
    void setDefaultRenamingSymbolMapper(const SymbolMapper &mapper);

    void setLimitRenamingToProjects(bool limit) { m_limitRenamingToProjects = limit; }

private:
    void handleFindReferencesResponse(
        const LanguageServerProtocol::FindReferencesRequest::Response &response,
        const QString &wordUnderCursor,
        const ResultHandler &handler);

    void requestPrepareRename(TextEditor::TextDocument *document,
                              const LanguageServerProtocol::TextDocumentPositionParams &params,
                              const QString &placeholder,
                              const QString &oldSymbolName,
                              bool preferLowerCaseFileNames);
    void requestRename(const LanguageServerProtocol::TextDocumentPositionParams &positionParams,
                       Core::SearchResult *search);
    Core::SearchResult *createSearch(const LanguageServerProtocol::TextDocumentPositionParams &positionParams,
                                     const QString &placeholder, const QString &oldSymbolName,
                                     bool preferLowerCaseFileNames);
    void startRenameSymbol(const LanguageServerProtocol::TextDocumentPositionParams &params,
                           const QString &placeholder, const QString &oldSymbolName,
                           bool preferLowerCaseFileNames);
    void handleRenameResponse(Core::SearchResult *search,
                              const LanguageServerProtocol::RenameRequest::Response &response);
    void applyRename(const QList<Core::SearchResultItem> &checkedItems, Core::SearchResult *search);

    Client *m_client = nullptr;
    SymbolMapper m_defaultSymbolMapper;
    bool m_limitRenamingToProjects = false;
};

} // namespace LanguageClient
