// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <texteditor/textdocument.h>

#include <languageserverprotocol/lspjsonrpc.h>

#include <utils/link.h>
#include <utils/searchresultitem.h>

#include <functional>

namespace Core { class SearchResult; }

namespace LanguageClient {

class Client;
enum class LinkTarget { SymbolDef, SymbolTypeDef, SymbolImplementation };

/// The document and position a request is made for.
struct DocumentPosition
{
    LanguageServerProtocol::TextDocumentIdentifier textDocument;
    LanguageServerProtocol::Position position;
};

class LANGUAGECLIENT_EXPORT SymbolSupport : public QObject
{
public:
    explicit SymbolSupport(Client *client);

    bool supportsFindLink(TextEditor::TextDocument *document, LinkTarget target) const;
    LanguageServerProtocol::MessageId findLinkAt(
        TextEditor::TextDocument *document,
        const QTextCursor &cursor,
        Utils::LinkHandler callback,
        const bool resolveTarget,
        const LinkTarget target);

    bool supportsFindUsages(TextEditor::TextDocument *document) const;
    /// The found locations, plus the result as it arrived, which carries whatever
    /// the server adds to a location beyond the protocol.
    using ResultHandler = std::function<
        void(const QList<LanguageServerProtocol::Location> &, const QJsonValue &rawResult)>;
    std::optional<LanguageServerProtocol::MessageId> findUsages(
        TextEditor::TextDocument *document,
        const QTextCursor &cursor,
        const ResultHandler &handler = {});

    bool supportsRename(TextEditor::TextDocument *document);
    void renameSymbol(TextEditor::TextDocument *document, const QTextCursor &cursor,
                      const QString &newSymbolName = {},
                      const std::function<void()> &callback = {},
                      bool preferLowerCaseFileNames = true);

    static Utils::Text::Range convertRange(const LanguageServerProtocol::Range &range);
    static QStringList getFileContents(const Utils::FilePath &filePath);

    using SymbolMapper = std::function<QString(const QString &)>;
    void setDefaultRenamingSymbolMapper(const SymbolMapper &mapper);

    void setLimitRenamingToProjects(bool limit) { m_limitRenamingToProjects = limit; }

    using RenameResultsEnhancer = std::function<Utils::SearchResultItems(const Utils::SearchResultItems &)>;
    void setRenameResultsEnhancer(const RenameResultsEnhancer &enhancer);

private:
    void handleFindReferencesResult(const QJsonObject &response,
                                    const QString &wordUnderCursor,
                                    const ResultHandler &handler);

    void requestPrepareRename(TextEditor::TextDocument *document,
                              const DocumentPosition &position,
                              const QString &placeholder,
                              const QString &oldSymbolName, const std::function<void()> &callback,
                              bool preferLowerCaseFileNames);
    void requestRename(const DocumentPosition &position, Core::SearchResult *search);
    Core::SearchResult *createSearch(const DocumentPosition &position,
                                     const QString &placeholder, const QString &oldSymbolName,
                                     const std::function<void()> &callback,
                                     bool preferLowerCaseFileNames);
    void startRenameSymbol(const DocumentPosition &position,
                           const QString &placeholder, const QString &oldSymbolName,
                           const std::function<void()> &callback, bool preferLowerCaseFileNames);
    void handleRenameResult(
        Core::SearchResult *search,
        const Utils::Result<LanguageServerProtocol::RenameRequestResult> &result);
    void applyRename(const Utils::SearchResultItems &checkedItems, Core::SearchResult *search);
    QString derivePlaceholder(const QString &oldSymbol, const QString &newSymbol);

    Client *m_client = nullptr;
    SymbolMapper m_defaultSymbolMapper;
    RenameResultsEnhancer m_renameResultsEnhancer;
    QHash<Core::SearchResult *, LanguageServerProtocol::MessageId> m_renameRequestIds;
    bool m_limitRenamingToProjects = false;
};

} // namespace LanguageClient
