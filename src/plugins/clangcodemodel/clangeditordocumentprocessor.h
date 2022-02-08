/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangdiagnosticmanager.h"
#include "clangeditordocumentparser.h"

#include <cppeditor/builtineditordocumentprocessor.h>
#include <cppeditor/semantichighlighter.h>

#include <utils/futuresynchronizer.h>
#include <utils/id.h>

#include <QFutureWatcher>
#include <QTimer>

namespace ClangBackEnd {
class DiagnosticContainer;
class TokenInfoContainer;
class FileContainer;
}

namespace ClangCodeModel {
namespace Internal {

class BackendCommunicator;

class ClangEditorDocumentProcessor : public CppEditor::BaseEditorDocumentProcessor
{
    Q_OBJECT

public:
    ClangEditorDocumentProcessor(BackendCommunicator &communicator,
                                 TextEditor::TextDocument *document);
    ~ClangEditorDocumentProcessor() override;

    // BaseEditorDocumentProcessor interface
    void runImpl(const CppEditor::BaseEditorDocumentParser::UpdateParams &updateParams) override;
    void semanticRehighlight() override;
    void recalculateSemanticInfoDetached(bool force) override;
    CppEditor::SemanticInfo recalculateSemanticInfo() override;
    CppEditor::BaseEditorDocumentParser::Ptr parser() override;
    CPlusPlus::Snapshot snapshot() override;
    bool isParserRunning() const override;

    bool hasProjectPart() const;
    CppEditor::ProjectPart::ConstPtr projectPart() const;
    void clearProjectPart();

    ::Utils::Id diagnosticConfigId() const;

    void updateCodeWarnings(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                            const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic,
                            uint documentRevision);
    void updateHighlighting(const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
                            const QVector<ClangBackEnd::SourceRangeContainer> &skippedPreprocessorRanges,
                            uint documentRevision);
    void updateTokenInfos(const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
                          uint documentRevision);

    TextEditor::QuickFixOperations
    extraRefactoringOperations(const TextEditor::AssistInterface &assistInterface) override;

    void invalidateDiagnostics() override;

    TextEditor::TextMarks diagnosticTextMarksAt(uint line, uint column) const;

    void editorDocumentTimerRestarted() override;

    void setParserConfig(const CppEditor::BaseEditorDocumentParser::Configuration &config) override;
    CppEditor::BaseEditorDocumentParser::Configuration parserConfig() const;

    QFuture<CppEditor::CursorInfo> cursorInfo(const CppEditor::CursorInfoParams &params) override;
    QFuture<CppEditor::CursorInfo> requestLocalReferences(const QTextCursor &cursor) override;
    QFuture<CppEditor::SymbolInfo> requestFollowSymbol(int line, int column) override;
    QFuture<CppEditor::ToolTipInfo> toolTipInfo(const QByteArray &codecName,
                                               int line,
                                               int column) override;

    void closeBackendDocument();

    void clearDiagnosticsWithFixIts();

    const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos() const;

    static void clearTaskHubIssues();
    void generateTaskHubIssues();

    static void clearTextMarks(const Utils::FilePath &filePath);

public:
    static ClangEditorDocumentProcessor *get(const QString &filePath);

signals:
    void tokenInfosUpdated();
    void parserConfigChanged(const Utils::FilePath &filePath,
                             const CppEditor::BaseEditorDocumentParser::Configuration &config);

private:
    void onParserFinished();

    void updateBackendProjectPartAndDocument();
    void updateBackendDocument(const CppEditor::ProjectPart &projectPart);
    void updateBackendDocumentIfProjectPartExists();
    void requestAnnotationsFromBackend();

    HeaderErrorDiagnosticWidgetCreator creatorForHeaderErrorDiagnosticWidget(
            const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic);
    ClangBackEnd::FileContainer simpleFileContainer(const QByteArray &codecName = QByteArray()) const;
    ClangBackEnd::FileContainer fileContainerWithOptionsAndDocumentContent(
        const QStringList &compilationArguments,
        const ProjectExplorer::HeaderPaths headerPaths) const;
    ClangBackEnd::FileContainer fileContainerWithDocumentContent() const;

private:
    TextEditor::TextDocument &m_document;
    ClangDiagnosticManager m_diagnosticManager;
    BackendCommunicator &m_communicator;
    QSharedPointer<ClangEditorDocumentParser> m_parser;
    CppEditor::ProjectPart::ConstPtr m_projectPart;
    ::Utils::Id m_diagnosticConfigId;
    bool m_isProjectFile = false;
    QFutureWatcher<void> m_parserWatcher;
    QTimer m_updateBackendDocumentTimer;
    unsigned m_parserRevision;
    enum class InvalidationState { Off, Scheduled, Canceled } m_invalidationState;

    QVector<ClangBackEnd::TokenInfoContainer> m_tokenInfos;
    CppEditor::SemanticHighlighter m_semanticHighlighter;
    CppEditor::BuiltinEditorDocumentProcessor m_builtinProcessor;
    Utils::FutureSynchronizer m_parserSynchronizer;
};

} // namespace Internal
} // namespace ClangCodeModel
