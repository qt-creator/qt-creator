// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/baseeditordocumentparser.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cursorineditor.h>
#include <languageclient/client.h>
#include <utils/link.h>
#include <utils/searchresultitem.h>

#include <QVersionNumber>

#include <optional>

namespace Core { class SearchResult; }
namespace CppEditor { class CppEditorWidget; }
namespace LanguageServerProtocol { class Range; }
namespace ProjectExplorer {
class Project;
class Task;
}
namespace TextEditor {
class BaseTextEditor;
class IAssistProposal;
}

namespace ClangCodeModel {
namespace Internal {
class ClangdAstNode;

Q_DECLARE_LOGGING_CATEGORY(clangdLog);
Q_DECLARE_LOGGING_CATEGORY(clangdLogAst);

void setupClangdConfigFile();

enum class FollowTo { SymbolDef, SymbolType };

class ClangdFollowSymbol;

class ClangdClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    ClangdClient(ProjectExplorer::Project *project,
                 const Utils::FilePath &jsonDbDir,
                 const Utils::Id &id = {});
    ~ClangdClient() override;

    bool isFullyIndexed() const;
    QVersionNumber versionNumber() const;
    CppEditor::ClangdSettings::Data settingsData() const;

    void openExtraFile(const Utils::FilePath &filePath, const QString &content = {});
    void closeExtraFile(const Utils::FilePath &filePath);

    void findUsages(const CppEditor::CursorInEditor &cursor,
                    const std::optional<QString> &replacement,
                    const std::function<void()> &renameCallback);
    void checkUnused(const Utils::Link &link, Core::SearchResult *search,
                     const Utils::LinkHandler &callback);
    void followSymbol(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppEditor::CppEditorWidget *editorWidget,
            const Utils::LinkHandler &callback,
            bool resolveTarget,
            FollowTo followTo,
            bool openInSplit);

    void switchDeclDef(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppEditor::CppEditorWidget *editorWidget,
            const Utils::LinkHandler &callback);
    void switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit);

    void findLocalUsages(CppEditor::CppEditorWidget *editorWidget, const QTextCursor &cursor,
                         CppEditor::RenameCallback &&callback);

    void gatherHelpItemForTooltip(
            const LanguageServerProtocol::HoverRequest::Response &hoverResponse,
            const Utils::FilePath &filePath);
    bool gatherMemberFunctionOverrideHelpItemForTooltip(const LanguageServerProtocol::MessageId &token,
        const Utils::FilePath &uri,
        const QList<ClangdAstNode> &path);

    void setVirtualRanges(const Utils::FilePath &filePath,
                          const QList<LanguageServerProtocol::Range> &ranges, int revision);

    void enableTesting();
    bool testingEnabled() const;

    static QString displayNameFromDocumentSymbol(LanguageServerProtocol::SymbolKind kind,
                                                 const QString &name, const QString &detail);

    static void handleUiHeaderChange(const QString &fileName);

    void updateParserConfig(const Utils::FilePath &filePath,
                            const CppEditor::BaseEditorDocumentParser::Configuration &config);
    void switchIssuePaneEntries(const Utils::FilePath &filePath);
    void addTask(const ProjectExplorer::Task &task);
    void clearTasks(const Utils::FilePath &filePath);
    std::optional<bool> hasVirtualFunctionAt(TextEditor::TextDocument *doc, int revision,
                                               const LanguageServerProtocol::Range &range);

    using TextDocOrFile = std::variant<const TextEditor::TextDocument *, Utils::FilePath>;
    using AstHandler = std::function<void(const ClangdAstNode &ast,
                                                const LanguageServerProtocol::MessageId &)>;
    enum class AstCallbackMode { SyncIfPossible, AlwaysAsync };
    LanguageServerProtocol::MessageId getAndHandleAst(const TextDocOrFile &doc,
                                                     const AstHandler &astHandler,
                                                      AstCallbackMode callbackMode,
                                                      const LanguageServerProtocol::Range &range);

    using SymbolInfoHandler = std::function<void(const QString &name, const QString &prefix,
                                                 const LanguageServerProtocol::MessageId &)>;
    LanguageServerProtocol::MessageId requestSymbolInfo(
            const Utils::FilePath &filePath,
            const LanguageServerProtocol::Position &position,
            const SymbolInfoHandler &handler);

#ifdef WITH_TESTS
    ClangdFollowSymbol *currentFollowSymbolOperation();
#endif

signals:
    void indexingFinished();
    void foundReferences(const Utils::SearchResultItems &items);
    void findUsagesDone();
    void helpItemGathered(const Core::HelpItem &helpItem);
    void highlightingResultsReady(const TextEditor::HighlightingResults &results,
                                  const Utils::FilePath &file);
    void proposalReady(TextEditor::IAssistProposal *proposal);
    void textMarkCreated(const Utils::FilePath &file);
    void configChanged();

private:
    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params) override;
    void handleDocumentOpened(TextEditor::TextDocument *doc) override;
    void handleDocumentClosed(TextEditor::TextDocument *doc) override;
    QTextCursor adjustedCursorForHighlighting(const QTextCursor &cursor,
                                              TextEditor::TextDocument *doc) override;
    const CustomInspectorTabs createCustomInspectorTabs() override;
    TextEditor::RefactoringChangesData *createRefactoringChangesBackend() const override;
    LanguageClient::DiagnosticManager *createDiagnosticManager() override;
    LanguageClient::LanguageClientOutlineItem *createOutlineItem(
        const LanguageServerProtocol::DocumentSymbol &symbol) override;
    bool referencesShadowFile(const TextEditor::TextDocument *doc,
                              const Utils::FilePath &candidate) override;
    bool fileBelongsToProject(const Utils::FilePath &filePath) const override;
    QList<Utils::Text::Range> additionalDocumentHighlights(
        TextEditor::TextEditorWidget *editorWidget, const QTextCursor &cursor) override;


    class Private;
    class VirtualFunctionAssistProcessor;
    class VirtualFunctionAssistProvider;
    Private * const d;
};

class ClangdDiagnostic : public LanguageServerProtocol::Diagnostic
{
public:
    using Diagnostic::Diagnostic;
    std::optional<QList<LanguageServerProtocol::CodeAction>> codeActions() const;
    QString category() const;
};

} // namespace Internal
} // namespace ClangCodeModel
