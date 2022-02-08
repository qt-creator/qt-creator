/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <cppeditor/baseeditordocumentparser.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/refactoringengineinterface.h>
#include <languageclient/client.h>
#include <utils/link.h>
#include <utils/optional.h>

#include <QVersionNumber>

namespace Core { class SearchResultItem; }
namespace CppEditor { class CppEditorWidget; }
namespace LanguageServerProtocol { class Range; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class BaseTextEditor; }

namespace ClangCodeModel {
namespace Internal {

class ClangdClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    ClangdClient(ProjectExplorer::Project *project, const Utils::FilePath &jsonDbDir);
    ~ClangdClient() override;

    bool isFullyIndexed() const;
    QVersionNumber versionNumber() const;
    CppEditor::ClangdSettings::Data settingsData() const;

    void openExtraFile(const Utils::FilePath &filePath, const QString &content = {});
    void closeExtraFile(const Utils::FilePath &filePath);

    void findUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                    const Utils::optional<QString> &replacement);
    void followSymbol(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppEditor::CppEditorWidget *editorWidget,
            Utils::ProcessLinkCallback &&callback,
            bool resolveTarget,
            bool openInSplit);

    void switchDeclDef(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppEditor::CppEditorWidget *editorWidget,
            Utils::ProcessLinkCallback &&callback);

    void findLocalUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                         CppEditor::RefactoringEngineInterface::RenameCallback &&callback);

    void gatherHelpItemForTooltip(
            const LanguageServerProtocol::HoverRequest::Response &hoverResponse,
            const LanguageServerProtocol::DocumentUri &uri);

    void setVirtualRanges(const Utils::FilePath &filePath,
                          const QList<LanguageServerProtocol::Range> &ranges, int revision);

    void enableTesting();
    bool testingEnabled() const;

    static QString displayNameFromDocumentSymbol(LanguageServerProtocol::SymbolKind kind,
                                                 const QString &name, const QString &detail);

    static void handleUiHeaderChange(const QString &fileName);

    void updateParserConfig(const Utils::FilePath &filePath,
                            const CppEditor::BaseEditorDocumentParser::Configuration &config);

signals:
    void indexingFinished();
    void foundReferences(const QList<Core::SearchResultItem> &items);
    void findUsagesDone();
    void helpItemGathered(const Core::HelpItem &helpItem);
    void highlightingResultsReady(const TextEditor::HighlightingResults &results,
                                  const Utils::FilePath &file);
    void proposalReady(TextEditor::IAssistProposal *proposal);
    void textMarkCreated(const Utils::FilePath &file);

private:
    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params) override;
    void handleDocumentOpened(TextEditor::TextDocument *doc) override;
    void handleDocumentClosed(TextEditor::TextDocument *doc) override;
    QTextCursor adjustedCursorForHighlighting(const QTextCursor &cursor,
                                              TextEditor::TextDocument *doc) override;
    const CustomInspectorTabs createCustomInspectorTabs() override;

    class Private;
    class FollowSymbolData;
    class VirtualFunctionAssistProcessor;
    class VirtualFunctionAssistProvider;
    class ClangdFunctionHintProcessor;
    class ClangdCompletionAssistProcessor;
    class ClangdCompletionAssistProvider;
    Private * const d;
};

class ClangdDiagnostic : public LanguageServerProtocol::Diagnostic
{
public:
    using Diagnostic::Diagnostic;
    Utils::optional<QList<LanguageServerProtocol::CodeAction>> codeActions() const;
    QString category() const;
};

} // namespace Internal
} // namespace ClangCodeModel
