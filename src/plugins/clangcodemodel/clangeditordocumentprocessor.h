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

#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/semantichighlighter.h>

#include <QFutureWatcher>
#include <QTimer>

namespace ClangBackEnd {
class DiagnosticContainer;
class HighlightingMarkContainer;
class FileContainer;
}

namespace ClangCodeModel {
namespace Internal {

class IpcCommunicator;

class ClangEditorDocumentProcessor : public CppTools::BaseEditorDocumentProcessor
{
    Q_OBJECT

public:
    ClangEditorDocumentProcessor(IpcCommunicator &ipcCommunicator,
                                 TextEditor::TextDocument *document);
    ~ClangEditorDocumentProcessor();

    // BaseEditorDocumentProcessor interface
    void runImpl(const CppTools::BaseEditorDocumentParser::UpdateParams &updateParams) override;
    void semanticRehighlight() override;
    void recalculateSemanticInfoDetached(bool force) override;
    CppTools::SemanticInfo recalculateSemanticInfo() override;
    CppTools::BaseEditorDocumentParser::Ptr parser() override;
    CPlusPlus::Snapshot snapshot() override;
    bool isParserRunning() const override;

    bool hasProjectPart() const;
    CppTools::ProjectPart::Ptr projectPart() const;
    void clearProjectPart();

    void updateCodeWarnings(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                            const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic,
                            uint documentRevision);
    void updateHighlighting(const QVector<ClangBackEnd::HighlightingMarkContainer> &highlightingMarks,
                            const QVector<ClangBackEnd::SourceRangeContainer> &skippedPreprocessorRanges,
                            uint documentRevision);

    TextEditor::QuickFixOperations
    extraRefactoringOperations(const TextEditor::AssistInterface &assistInterface) override;

    bool hasDiagnosticsAt(uint line, uint column) const override;
    void addDiagnosticToolTipToLayout(uint line, uint column, QLayout *target) const override;

    void editorDocumentTimerRestarted() override;

    void setParserConfig(const CppTools::BaseEditorDocumentParser::Configuration config) override;

    ClangBackEnd::FileContainer fileContainerWithArguments() const;

    void clearDiagnosticsWithFixIts();

public:
    static ClangEditorDocumentProcessor *get(const QString &filePath);

private:
    void onParserFinished();
    void updateProjectPartAndTranslationUnitForEditor();
    void registerTranslationUnitForEditor(CppTools::ProjectPart *projectPart);
    void updateTranslationUnitIfProjectPartExists();
    void requestDocumentAnnotations(const QString &projectpartId);
    HeaderErrorDiagnosticWidgetCreator creatorForHeaderErrorDiagnosticWidget(
            const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic);
    ClangBackEnd::FileContainer fileContainerWithArguments(CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::FileContainer fileContainerWithArgumentsAndDocumentContent(
            CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::FileContainer fileContainerWithDocumentContent(const QString &projectpartId) const;

private:
    ClangDiagnosticManager m_diagnosticManager;
    IpcCommunicator &m_ipcCommunicator;
    QSharedPointer<ClangEditorDocumentParser> m_parser;
    CppTools::ProjectPart::Ptr m_projectPart;
    QFutureWatcher<void> m_parserWatcher;
    QTimer m_updateTranslationUnitTimer;
    unsigned m_parserRevision;

    CppTools::SemanticHighlighter m_semanticHighlighter;
    CppTools::BuiltinEditorDocumentProcessor m_builtinProcessor;
};

} // namespace Internal
} // namespace ClangCodeModel
