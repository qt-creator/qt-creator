// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <texteditor/textdocument.h>

#include <languageserverprotocol/languagefeatures.h>

#include <utils/searchresultitem.h>

#include <functional>

namespace Core { class SearchResult; }
namespace LanguageServerProtocol { class MessageId; }

namespace LanguageClient {

class Client;
enum class LinkTarget { SymbolDef, SymbolTypeDef, SymbolImplementation };

class LANGUAGECLIENT_EXPORT SymbolSupport : public QObject
{
public:
    explicit SymbolSupport(Client *client);

    bool supportsFindLink(TextEditor::TextDocument *document, LinkTarget target) const;
    LanguageServerProtocol::MessageId findLinkAt(TextEditor::TextDocument *document,
                                                 const QTextCursor &cursor,
                                                 Utils::LinkHandler callback,
                                                 const bool resolveTarget,
                                                 const LinkTarget target);

    bool supportsFindUsages(TextEditor::TextDocument *document) const;
    using ResultHandler = std::function<void(const QList<LanguageServerProtocol::Location> &)>;
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
    void handleFindReferencesResponse(
        const LanguageServerProtocol::FindReferencesRequest::Response &response,
        const QString &wordUnderCursor,
        const ResultHandler &handler);

    void requestPrepareRename(TextEditor::TextDocument *document,
                              const LanguageServerProtocol::TextDocumentPositionParams &params,
                              const QString &placeholder,
                              const QString &oldSymbolName, const std::function<void()> &callback,
                              bool preferLowerCaseFileNames);
    void requestRename(const LanguageServerProtocol::TextDocumentPositionParams &positionParams,
                       Core::SearchResult *search);
    Core::SearchResult *createSearch(const LanguageServerProtocol::TextDocumentPositionParams &positionParams,
                                     const QString &placeholder, const QString &oldSymbolName,
                                     const std::function<void()> &callback,
                                     bool preferLowerCaseFileNames);
    void startRenameSymbol(const LanguageServerProtocol::TextDocumentPositionParams &params,
                           const QString &placeholder, const QString &oldSymbolName,
                           const std::function<void()> &callback, bool preferLowerCaseFileNames);
    void handleRenameResponse(Core::SearchResult *search,
                              const LanguageServerProtocol::RenameRequest::Response &response);
    void applyRename(const Utils::SearchResultItems &checkedItems, Core::SearchResult *search);
    QString derivePlaceholder(const QString &oldSymbol, const QString &newSymbol);

    Client *m_client = nullptr;
    SymbolMapper m_defaultSymbolMapper;
    RenameResultsEnhancer m_renameResultsEnhancer;
    QHash<Core::SearchResult *, LanguageServerProtocol::MessageId> m_renameRequestIds;
    bool m_limitRenamingToProjects = false;
};

} // namespace LanguageClient
