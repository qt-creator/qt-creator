/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangeditordocumentprocessor.h"

#include "clangfixitoperation.h"
#include "clangfixitoperationsextractor.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"
#include "highlightingmarksreporter.h"

#include <diagnosticcontainer.h>
#include <sourcelocationcontainer.h>

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppworkingcopy.h>

#include <texteditor/convenience.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>

#include <utils/qtcassert.h>
#include <utils/QtConcurrentTools>

#include <QTextBlock>

namespace ClangCodeModel {
namespace Internal {

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(
        ModelManagerSupportClang *modelManagerSupport,
        TextEditor::TextDocument *document)
    : BaseEditorDocumentProcessor(document)
    , m_diagnosticManager(document)
    , m_modelManagerSupport(modelManagerSupport)
    , m_parser(new ClangEditorDocumentParser(document->filePath().toString()))
    , m_parserRevision(0)
    , m_semanticHighlighter(document)
    , m_builtinProcessor(document, /*enableSemanticHighlighter=*/ false)
{
    // Forwarding the semantic info from the builtin processor enables us to provide all
    // editor (widget) related features that are not yet implemented by the clang plugin.
    connect(&m_builtinProcessor, &CppTools::BuiltinEditorDocumentProcessor::cppDocumentUpdated,
            this, &ClangEditorDocumentProcessor::cppDocumentUpdated);
    connect(&m_builtinProcessor, &CppTools::BuiltinEditorDocumentProcessor::semanticInfoUpdated,
            this, &ClangEditorDocumentProcessor::semanticInfoUpdated);
}

ClangEditorDocumentProcessor::~ClangEditorDocumentProcessor()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();

    if (m_projectPart) {
        QTC_ASSERT(m_modelManagerSupport, return);
        m_modelManagerSupport->ipcCommunicator().unregisterTranslationUnitsForEditor(
            {ClangBackEnd::FileContainer(filePath(), m_projectPart->id())});
    }
}

void ClangEditorDocumentProcessor::run()
{
    requestDiagnosticsAndHighlighting();

    // Run clang parser
    disconnect(&m_parserWatcher, &QFutureWatcher<void>::finished,
               this, &ClangEditorDocumentProcessor::onParserFinished);
    m_parserWatcher.cancel();
    m_parserWatcher.setFuture(QFuture<void>());

    m_parserRevision = revision();
    connect(&m_parserWatcher, &QFutureWatcher<void>::finished,
            this, &ClangEditorDocumentProcessor::onParserFinished);
    const QFuture<void> future = QtConcurrent::run(&runParser,
                                                   parser(),
                                                   ClangEditorDocumentParser::InMemoryInfo(true));
    m_parserWatcher.setFuture(future);

    // Run builtin processor
    m_builtinProcessor.run();
}

void ClangEditorDocumentProcessor::recalculateSemanticInfoDetached(bool force)
{
    m_builtinProcessor.recalculateSemanticInfoDetached(force);
}

void ClangEditorDocumentProcessor::semanticRehighlight()
{
    m_semanticHighlighter.updateFormatMapFromFontSettings();
    requestDiagnosticsAndHighlighting(DocumentChangedCheck::NoCheck);
}

CppTools::SemanticInfo ClangEditorDocumentProcessor::recalculateSemanticInfo()
{
    return m_builtinProcessor.recalculateSemanticInfo();
}

CppTools::BaseEditorDocumentParser::Ptr ClangEditorDocumentProcessor::parser()
{
    return m_parser;
}

CPlusPlus::Snapshot ClangEditorDocumentProcessor::snapshot()
{
   return m_builtinProcessor.snapshot();
}

bool ClangEditorDocumentProcessor::isParserRunning() const
{
    return m_parserWatcher.isRunning();
}

bool ClangEditorDocumentProcessor::hasProjectPart() const
{
    return m_projectPart;
}

CppTools::ProjectPart::Ptr ClangEditorDocumentProcessor::projectPart() const
{
    return m_projectPart;
}

void ClangEditorDocumentProcessor::clearProjectPart()
{
    m_projectPart.clear();
}

void ClangEditorDocumentProcessor::updateCodeWarnings(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                                                      uint documentRevision)
{
    if (documentRevision == revision()) {
        m_diagnosticManager.processNewDiagnostics(diagnostics);
        const auto codeWarnings = m_diagnosticManager.takeExtraSelections();
        emit codeWarningsUpdated(revision(), codeWarnings);
    }
}

static QList<TextEditor::BlockRange>
toTextEditorBlocks(const QVector<ClangBackEnd::SourceRangeContainer> &ifdefedOutRanges)
{
    QList<TextEditor::BlockRange> blockRanges;
    blockRanges.reserve(ifdefedOutRanges.size());

    for (const auto &range : ifdefedOutRanges)
        blockRanges.append(TextEditor::BlockRange(range.start().offset(),range.end().offset()));

    return blockRanges;
}

void ClangEditorDocumentProcessor::updateHighlighting(
        const QVector<ClangBackEnd::HighlightingMarkContainer> &highlightingMarks,
        const QVector<ClangBackEnd::SourceRangeContainer> &skippedPreprocessorRanges,
        uint documentRevision)
{
    if (documentRevision == revision()) {
        const auto skippedPreprocessorBlocks = toTextEditorBlocks(skippedPreprocessorRanges);
        emit ifdefedOutBlocksUpdated(documentRevision, skippedPreprocessorBlocks);

        m_semanticHighlighter.setHighlightingRunner(
            [highlightingMarks]() {
                auto *reporter = new HighlightingMarksReporter(highlightingMarks);
                return reporter->start();
            });
        m_semanticHighlighter.run();
    }
}

static int currentLine(const TextEditor::AssistInterface &assistInterface)
{
    int line, column;
    TextEditor::Convenience::convertPosition(assistInterface.textDocument(),
                                             assistInterface.position(),
                                             &line,
                                             &column);
    return line;
}

TextEditor::QuickFixOperations ClangEditorDocumentProcessor::extraRefactoringOperations(
        const TextEditor::AssistInterface &assistInterface)
{
    ClangFixItOperationsExtractor extractor(m_diagnosticManager.diagnosticsWithFixIts());

    return extractor.extract(assistInterface.fileName(), currentLine(assistInterface));
}

ClangBackEnd::FileContainer ClangEditorDocumentProcessor::fileContainer() const
{
    return fileContainer(m_projectPart.data());
}

void ClangEditorDocumentProcessor::clearDiagnosticsWithFixIts()
{
    m_diagnosticManager.clearDiagnosticsWithFixIts();
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor *>(BaseEditorDocumentProcessor::get(filePath));
}

static bool isProjectPartLoadedOrIsFallback(CppTools::ProjectPart::Ptr projectPart)
{
    return projectPart
        && (projectPart->id().isEmpty() || ClangCodeModel::Utils::isProjectPartLoaded(projectPart));
}

void ClangEditorDocumentProcessor::updateProjectPartAndTranslationUnitForEditor()
{
    const CppTools::ProjectPart::Ptr projectPart = m_parser->projectPart();

    if (isProjectPartLoadedOrIsFallback(projectPart)) {
        updateTranslationUnitForEditor(projectPart.data());
        requestDiagnosticsAndHighlighting(projectPart.data());

        m_projectPart = projectPart;
    }
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    updateProjectPartAndTranslationUnitForEditor();
}

void ClangEditorDocumentProcessor::updateTranslationUnitForEditor(CppTools::ProjectPart *projectPart)
{
    QTC_ASSERT(m_modelManagerSupport, return);
    IpcCommunicator &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

    if (m_projectPart) {
        if (projectPart->id() != m_projectPart->id()) {
            ipcCommunicator.unregisterTranslationUnitsForEditor({fileContainer()});
            ipcCommunicator.registerTranslationUnitsForEditor({fileContainer(projectPart)});
        }
    } else {
        ipcCommunicator.registerTranslationUnitsForEditor({{fileContainer(projectPart)}});
    }
}

void ClangEditorDocumentProcessor::requestDiagnosticsAndHighlighting(CppTools::ProjectPart *projectPart)
{
    if (!m_projectPart || projectPart->id() != m_projectPart->id()) {
        IpcCommunicator &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

        const ClangBackEnd::FileContainer fileContainer_ = fileContainer(projectPart);
        ipcCommunicator.requestDiagnosticsAndHighlighting(fileContainer_);
    }
}

IpcCommunicator::DocumentChangedCheck
toIpcCommunicatorDocumentChangedCheck(ClangEditorDocumentProcessor::DocumentChangedCheck condition)
{
    return condition == ClangEditorDocumentProcessor::DocumentChangedCheck::RevisionCheck
            ? IpcCommunicator::DocumentChangedCheck::RevisionCheck
            : IpcCommunicator::DocumentChangedCheck::NoCheck;
}

void ClangEditorDocumentProcessor::requestDiagnosticsAndHighlighting(DocumentChangedCheck documentChangedCheck)
{
    if (m_projectPart) {
        auto  &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

        const ClangBackEnd::FileContainer fileContainer(filePath(),
                                                        m_projectPart->id(),
                                                        baseTextDocument()->plainText(),
                                                        true,
                                                        revision());

        const auto documentCheck = toIpcCommunicatorDocumentChangedCheck(documentChangedCheck);

        ipcCommunicator.requestDiagnosticsAndHighlighting(fileContainer, documentCheck);
    }
}

static CppTools::ProjectPart projectPartForLanguageOption(CppTools::ProjectPart *projectPart)
{
    if (projectPart)
        return *projectPart;
    return *CppTools::CppModelManager::instance()->fallbackProjectPart().data();
}

static QStringList languageOptions(const QString &filePath, CppTools::ProjectPart *projectPart)
{
    const auto theProjectPart = projectPartForLanguageOption(projectPart);
    CppTools::CompilerOptionsBuilder builder(theProjectPart);
    builder.addLanguageOption(CppTools::ProjectFile::classify(filePath));

    return builder.options();
}

static QStringList fileArguments(const QString &filePath, CppTools::ProjectPart *projectPart)
{
    return QStringList(languageOptions(filePath, projectPart))
         + CppTools::codeModelSettings()->extraClangOptions();
}

ClangBackEnd::FileContainer
ClangEditorDocumentProcessor::fileContainer(CppTools::ProjectPart *projectPart) const
{
    const auto projectPartId = projectPart
            ? Utf8String::fromString(projectPart->id())
            : Utf8String();
    const QStringList theFileArguments = fileArguments(filePath(), projectPart);

    return {filePath(), projectPartId, Utf8StringVector(theFileArguments), revision()};
}

} // namespace Internal
} // namespace ClangCodeModel
