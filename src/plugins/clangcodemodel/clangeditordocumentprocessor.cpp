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

#include "clangeditordocumentprocessor.h"

#include "clangbackendcommunicator.h"
#include "clangprojectsettings.h"
#include "clangdiagnostictooltipwidget.h"
#include "clangfixitoperation.h"
#include "clangfixitoperationsextractor.h"
#include "clangmodelmanagersupport.h"
#include "clanghighlightingresultreporter.h"
#include "clangutils.h"

#include <diagnosticcontainer.h>
#include <sourcelocationcontainer.h>

#include <cppeditor/builtincursorinfo.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppworkingcopy.h>
#include <cppeditor/editordocumenthandle.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>

#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextBlock>
#include <QVBoxLayout>
#include <QWidget>

namespace ClangCodeModel {
namespace Internal {

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(
        BackendCommunicator &communicator,
        TextEditor::TextDocument *document)
    : BaseEditorDocumentProcessor(document->document(), document->filePath().toString())
    , m_document(*document)
    , m_diagnosticManager(document)
    , m_communicator(communicator)
    , m_parser(new ClangEditorDocumentParser(document->filePath().toString()))
    , m_parserRevision(0)
    , m_semanticHighlighter(document)
    , m_builtinProcessor(document, /*enableSemanticHighlighter=*/ false)
{
    m_updateBackendDocumentTimer.setSingleShot(true);
    m_updateBackendDocumentTimer.setInterval(350);
    connect(&m_updateBackendDocumentTimer, &QTimer::timeout,
            this, &ClangEditorDocumentProcessor::updateBackendDocumentIfProjectPartExists);

    connect(m_parser.data(), &ClangEditorDocumentParser::projectPartInfoUpdated,
            this, &BaseEditorDocumentProcessor::projectPartInfoUpdated);

    // Forwarding the semantic info from the builtin processor enables us to provide all
    // editor (widget) related features that are not yet implemented by the clang plugin.
    connect(&m_builtinProcessor, &CppEditor::BuiltinEditorDocumentProcessor::cppDocumentUpdated,
            this, &ClangEditorDocumentProcessor::cppDocumentUpdated);
    connect(&m_builtinProcessor, &CppEditor::BuiltinEditorDocumentProcessor::semanticInfoUpdated,
            this, &ClangEditorDocumentProcessor::semanticInfoUpdated);

    m_parserSynchronizer.setCancelOnWait(true);
}

ClangEditorDocumentProcessor::~ClangEditorDocumentProcessor()
{
    m_updateBackendDocumentTimer.stop();

    if (m_projectPart)
        closeBackendDocument();
}

void ClangEditorDocumentProcessor::runImpl(
        const CppEditor::BaseEditorDocumentParser::UpdateParams &updateParams)
{
    m_updateBackendDocumentTimer.start();

    // Run clang parser
    disconnect(&m_parserWatcher, &QFutureWatcher<void>::finished,
               this, &ClangEditorDocumentProcessor::onParserFinished);
    m_parserWatcher.cancel();
    m_parserWatcher.setFuture(QFuture<void>());

    m_parserRevision = revision();
    connect(&m_parserWatcher, &QFutureWatcher<void>::finished,
            this, &ClangEditorDocumentProcessor::onParserFinished);
    const QFuture<void> future = ::Utils::runAsync(&runParser, parser(), updateParams);
    m_parserWatcher.setFuture(future);
    m_parserSynchronizer.addFuture(future);

    // Run builtin processor
    m_builtinProcessor.runImpl(updateParams);
}

void ClangEditorDocumentProcessor::recalculateSemanticInfoDetached(bool force)
{
    m_builtinProcessor.recalculateSemanticInfoDetached(force);
}

void ClangEditorDocumentProcessor::semanticRehighlight()
{
    const auto matchesEditor = [this](const Core::IEditor *editor) {
        return editor->document()->filePath() == m_document.filePath();
    };
    if (!Utils::contains(Core::EditorManager::visibleEditors(), matchesEditor))
        return;
    if (ClangModelManagerSupport::instance()->clientForFile(m_document.filePath()))
        return;

    m_semanticHighlighter.updateFormatMapFromFontSettings();
    if (m_projectPart)
        requestAnnotationsFromBackend();
}

CppEditor::SemanticInfo ClangEditorDocumentProcessor::recalculateSemanticInfo()
{
    return m_builtinProcessor.recalculateSemanticInfo();
}

CppEditor::BaseEditorDocumentParser::Ptr ClangEditorDocumentProcessor::parser()
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
    return !m_projectPart.isNull();
}

CppEditor::ProjectPart::ConstPtr ClangEditorDocumentProcessor::projectPart() const
{
    return m_projectPart;
}

void ClangEditorDocumentProcessor::clearProjectPart()
{
    m_projectPart.clear();
}

::Utils::Id ClangEditorDocumentProcessor::diagnosticConfigId() const
{
    return m_diagnosticConfigId;
}

void ClangEditorDocumentProcessor::updateCodeWarnings(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
        const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic,
        uint documentRevision)
{
    if (ClangModelManagerSupport::instance()->clientForFile(m_document.filePath()))
        return;

    if (documentRevision == revision()) {
        if (m_invalidationState == InvalidationState::Scheduled)
            m_invalidationState = InvalidationState::Canceled;
        m_diagnosticManager.processNewDiagnostics(diagnostics, m_isProjectFile);
        const auto codeWarnings = m_diagnosticManager.takeExtraSelections();
        const auto fixitAvailableMarkers = m_diagnosticManager.takeFixItAvailableMarkers();
        const auto creator = creatorForHeaderErrorDiagnosticWidget(firstHeaderErrorDiagnostic);

        emit codeWarningsUpdated(revision(),
                                 codeWarnings,
                                 creator,
                                 fixitAvailableMarkers);
    }
}
namespace {

TextEditor::BlockRange
toTextEditorBlock(QTextDocument *textDocument,
                  const ClangBackEnd::SourceRangeContainer &sourceRangeContainer)
{
    return {::Utils::Text::positionInText(textDocument,
                                          sourceRangeContainer.start.line,
                                          sourceRangeContainer.start.column),
            ::Utils::Text::positionInText(textDocument,
                                          sourceRangeContainer.end.line,
                                          sourceRangeContainer.end.column)};
}

QList<TextEditor::BlockRange>
toTextEditorBlocks(QTextDocument *textDocument,
                   const QVector<ClangBackEnd::SourceRangeContainer> &ifdefedOutRanges)
{
    QList<TextEditor::BlockRange> blockRanges;
    blockRanges.reserve(ifdefedOutRanges.size());

    for (const auto &range : ifdefedOutRanges)
        blockRanges.append(toTextEditorBlock(textDocument, range));

    return blockRanges;
}
}

const QVector<ClangBackEnd::TokenInfoContainer>
&ClangEditorDocumentProcessor::tokenInfos() const
{
    return m_tokenInfos;
}

void ClangEditorDocumentProcessor::clearTaskHubIssues()
{
    ClangDiagnosticManager::clearTaskHubIssues();
}

void ClangEditorDocumentProcessor::generateTaskHubIssues()
{
    m_diagnosticManager.generateTaskHubIssues();
}

void ClangEditorDocumentProcessor::clearTextMarks(const Utils::FilePath &filePath)
{
    if (ClangEditorDocumentProcessor * const proc = get(filePath.toString())) {
        proc->m_diagnosticManager.cleanMarks();
        emit proc->codeWarningsUpdated(proc->revision(), {}, {}, {});
    }
}

void ClangEditorDocumentProcessor::updateHighlighting(
        const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
        const QVector<ClangBackEnd::SourceRangeContainer> &skippedPreprocessorRanges,
        uint documentRevision)
{
    if (ClangModelManagerSupport::instance()->clientForFile(m_document.filePath()))
        return;
    if (documentRevision == revision()) {
        const auto skippedPreprocessorBlocks = toTextEditorBlocks(textDocument(), skippedPreprocessorRanges);
        emit ifdefedOutBlocksUpdated(documentRevision, skippedPreprocessorBlocks);

        m_semanticHighlighter.setHighlightingRunner(
            [tokenInfos]() {
                auto *reporter = new HighlightingResultReporter(tokenInfos);
                return reporter->start();
            });
        m_semanticHighlighter.run();
    }
}

void ClangEditorDocumentProcessor::updateTokenInfos(
        const QVector<ClangBackEnd::TokenInfoContainer> &tokenInfos,
        uint documentRevision)
{
    if (documentRevision != revision())
        return;
    m_tokenInfos = tokenInfos;
    emit tokenInfosUpdated();
}

static int currentLine(const TextEditor::AssistInterface &assistInterface)
{
    int line, column;
    ::Utils::Text::convertPosition(assistInterface.textDocument(), assistInterface.position(),
                                   &line, &column);
    return line;
}

TextEditor::QuickFixOperations ClangEditorDocumentProcessor::extraRefactoringOperations(
        const TextEditor::AssistInterface &assistInterface)
{
    ClangFixItOperationsExtractor extractor(m_diagnosticManager.diagnosticsWithFixIts());

    return extractor.extract(assistInterface.filePath().toString(), currentLine(assistInterface));
}

void ClangEditorDocumentProcessor::editorDocumentTimerRestarted()
{
    m_updateBackendDocumentTimer.stop(); // Wait for the next call to run().
    m_invalidationState = InvalidationState::Scheduled;
}

void ClangEditorDocumentProcessor::invalidateDiagnostics()
{
    if (m_invalidationState != InvalidationState::Canceled)
        m_diagnosticManager.invalidateDiagnostics();
    m_invalidationState = InvalidationState::Off;
}

TextEditor::TextMarks ClangEditorDocumentProcessor::diagnosticTextMarksAt(uint line,
                                                                          uint column) const
{
    return m_diagnosticManager.diagnosticTextMarksAt(line, column);
}

void ClangEditorDocumentProcessor::setParserConfig(
        const CppEditor::BaseEditorDocumentParser::Configuration &config)
{
    m_parser->setConfiguration(config);
    m_builtinProcessor.parser()->setConfiguration(config);
}

static bool isCursorOnIdentifier(const QTextCursor &textCursor)
{
    QTextDocument *document = textCursor.document();
    return CppEditor::isValidIdentifierChar(document->characterAt(textCursor.position()));
}

static QFuture<CppEditor::CursorInfo> defaultCursorInfoFuture()
{
    QFutureInterface<CppEditor::CursorInfo> futureInterface;
    futureInterface.reportResult(CppEditor::CursorInfo());
    futureInterface.reportFinished();

    return futureInterface.future();
}

static bool convertPosition(const QTextCursor &textCursor, int *line, int *column)
{
    const bool converted = ::Utils::Text::convertPosition(textCursor.document(),
                                                          textCursor.position(),
                                                          line,
                                                          column);
    QTC_CHECK(converted);
    return converted;
}

QFuture<CppEditor::CursorInfo>
ClangEditorDocumentProcessor::cursorInfo(const CppEditor::CursorInfoParams &params)
{
    int line, column;
    convertPosition(params.textCursor, &line, &column);

    if (!isCursorOnIdentifier(params.textCursor))
        return defaultCursorInfoFuture();

    column = clangColumn(params.textCursor.document()->findBlockByNumber(line - 1), column);
    const CppEditor::SemanticInfo::LocalUseMap localUses
        = CppEditor::BuiltinCursorInfo::findLocalUses(params.semanticInfo.doc, line, column);

    return m_communicator.requestReferences(simpleFileContainer(),
                                            static_cast<quint32>(line),
                                            static_cast<quint32>(column),
                                            localUses);
}

QFuture<CppEditor::CursorInfo> ClangEditorDocumentProcessor::requestLocalReferences(
        const QTextCursor &cursor)
{
    int line, column;
    convertPosition(cursor, &line, &column);
    ++column; // for 1-based columns

    // TODO: check that by highlighting items
    if (!isCursorOnIdentifier(cursor))
        return defaultCursorInfoFuture();

    return m_communicator.requestLocalReferences(simpleFileContainer(),
                                                 static_cast<quint32>(line),
                                                 static_cast<quint32>(column));
}

QFuture<CppEditor::SymbolInfo>
ClangEditorDocumentProcessor::requestFollowSymbol(int line, int column)
{
    return m_communicator.requestFollowSymbol(simpleFileContainer(),
                                              static_cast<quint32>(line),
                                              static_cast<quint32>(column));
}

QFuture<CppEditor::ToolTipInfo> ClangEditorDocumentProcessor::toolTipInfo(const QByteArray &codecName,
                                                                         int line,
                                                                         int column)
{
    return m_communicator.requestToolTip(simpleFileContainer(codecName),
                                         static_cast<quint32>(line),
                                         static_cast<quint32>(column));
}

void ClangEditorDocumentProcessor::clearDiagnosticsWithFixIts()
{
    m_diagnosticManager.clearDiagnosticsWithFixIts();
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor*>(
                CppEditor::CppModelManager::cppEditorDocumentProcessor(filePath));
}

static bool isProjectPartLoadedOrIsFallback(CppEditor::ProjectPart::ConstPtr projectPart)
{
    return projectPart
        && (projectPart->id().isEmpty() || isProjectPartLoaded(projectPart));
}

void ClangEditorDocumentProcessor::updateBackendProjectPartAndDocument()
{
    const CppEditor::ProjectPart::ConstPtr projectPart = m_parser->projectPartInfo().projectPart;

    if (isProjectPartLoadedOrIsFallback(projectPart)) {
        updateBackendDocument(*projectPart.data());

        m_projectPart = projectPart;
        m_isProjectFile = m_parser->projectPartInfo().hints
                & CppEditor::ProjectPartInfo::IsFromProjectMatch;
    }
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    updateBackendProjectPartAndDocument();
}

void ClangEditorDocumentProcessor::updateBackendDocument(const CppEditor::ProjectPart &projectPart)
{
    // On registration we send the document content immediately as an unsaved
    // file, because
    //   (1) a refactoring action might have opened and already modified
    //       this document.
    //   (2) it prevents an extra preamble generation on first user
    //       modification of the document in case the line endings on disk
    //       differ from the ones returned by textDocument()->toPlainText(),
    //       like on Windows.

    if (m_projectPart) {
        if (projectPart.id() == m_projectPart->id())
            return;
    }

    ProjectExplorer::Project * const project = CppEditor::projectForProjectPart(projectPart);
    const CppEditor::ClangDiagnosticConfig config = warningsConfigForProject(project);
    const QStringList clangOptions = createClangOptions(projectPart, filePath(), config,
                                                        optionsForProject(project));
    m_diagnosticConfigId = config.id();

    m_communicator.documentsOpened(
        {fileContainerWithOptionsAndDocumentContent(clangOptions, projectPart.headerPaths)});
    setLastSentDocumentRevision(filePath(), revision());
}

void ClangEditorDocumentProcessor::closeBackendDocument()
{
    QTC_ASSERT(m_projectPart, return);
    m_communicator.documentsClosed({ClangBackEnd::FileContainer(filePath(), m_projectPart->id())});
}

void ClangEditorDocumentProcessor::updateBackendDocumentIfProjectPartExists()
{
    if (m_projectPart) {
        const ClangBackEnd::FileContainer fileContainer = fileContainerWithDocumentContent();
        m_communicator.documentsChangedWithRevisionCheck(fileContainer);
    }
}

void ClangEditorDocumentProcessor::requestAnnotationsFromBackend()
{
    const auto fileContainer = fileContainerWithDocumentContent();
    m_communicator.requestAnnotations(fileContainer);
}

CppEditor::BaseEditorDocumentProcessor::HeaderErrorDiagnosticWidgetCreator
ClangEditorDocumentProcessor::creatorForHeaderErrorDiagnosticWidget(
        const ClangBackEnd::DiagnosticContainer &firstHeaderErrorDiagnostic)
{
    if (firstHeaderErrorDiagnostic.text.isEmpty())
        return CppEditor::BaseEditorDocumentProcessor::HeaderErrorDiagnosticWidgetCreator();

    return [firstHeaderErrorDiagnostic]() {
        auto vbox = new QVBoxLayout;
        vbox->setContentsMargins(10, 0, 0, 2);
        vbox->setSpacing(2);

        vbox->addWidget(ClangDiagnosticWidget::createWidget({firstHeaderErrorDiagnostic},
                                                            ClangDiagnosticWidget::InfoBar, {},
                                                            "libclang"));

        auto widget = new QWidget;
        widget->setLayout(vbox);

        return widget;
    };
}

ClangBackEnd::FileContainer ClangEditorDocumentProcessor::simpleFileContainer(
    const QByteArray &codecName) const
{
    return ClangBackEnd::FileContainer(filePath(),
                                       Utf8String(),
                                       false,
                                       revision(),
                                       Utf8String::fromByteArray(codecName));
}

ClangBackEnd::FileContainer ClangEditorDocumentProcessor::fileContainerWithOptionsAndDocumentContent(
    const QStringList &compilationArguments, const ProjectExplorer::HeaderPaths headerPaths) const
{
    auto theHeaderPaths
        = ::Utils::transform<QVector>(headerPaths, [](const ProjectExplorer::HeaderPath path) {
              return Utf8String(QDir::toNativeSeparators(path.path));
    });
    theHeaderPaths << QDir::toNativeSeparators(
        ClangModelManagerSupport::instance()->dummyUiHeaderOnDiskDirPath());

    return ClangBackEnd::FileContainer(filePath(),
                                       Utf8StringVector(compilationArguments),
                                       theHeaderPaths,
                                       textDocument()->toPlainText(),
                                       true,
                                       revision());
}

ClangBackEnd::FileContainer
ClangEditorDocumentProcessor::fileContainerWithDocumentContent() const
{
    return ClangBackEnd::FileContainer(filePath(),
                                       textDocument()->toPlainText(),
                                       true,
                                       revision());
}

} // namespace Internal
} // namespace ClangCodeModel
