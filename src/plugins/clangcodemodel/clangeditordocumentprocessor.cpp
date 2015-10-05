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
#include "cppcreatemarkers.h"
#include "diagnostic.h"
#include "pchinfo.h"

#include <diagnosticcontainer.h>
#include <sourcelocationcontainer.h>

#include <cpptools/cpptoolsplugin.h>
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

namespace {

typedef CPlusPlus::Document::DiagnosticMessage CppToolsDiagnostic;

QList<TextEditor::BlockRange> toTextEditorBlocks(
        const QList<ClangCodeModel::SemanticMarker::Range> &ranges)
{
    QList<TextEditor::BlockRange> result;
    result.reserve(ranges.size());
    foreach (const ClangCodeModel::SemanticMarker::Range &range, ranges)
        result.append(TextEditor::BlockRange(range.first, range.last));
    return result;
}

} // anonymous namespace

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

    m_semanticHighlighter.setHighlightingRunner(
        [this]() -> QFuture<TextEditor::HighlightingResult> {
            const int firstLine = 1;
            const int lastLine = baseTextDocument()->document()->blockCount();

            CreateMarkers *createMarkers = CreateMarkers::create(m_parser->semanticMarker(),
                                                                 baseTextDocument()->filePath().toString(),
                                                                 firstLine, lastLine);
            return createMarkers->start();
        });
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
    requestDiagnostics();

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
    m_semanticHighlighter.run();
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

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor *>(BaseEditorDocumentProcessor::get(filePath));
}

static bool isProjectPartLoadedOrIsFallback(CppTools::ProjectPart::Ptr projectPart)
{
    return projectPart
        && (projectPart->id().isEmpty() || ClangCodeModel::Utils::isProjectPartValid(projectPart));
}

void ClangEditorDocumentProcessor::updateProjectPartAndTranslationUnitForEditor()
{
    const CppTools::ProjectPart::Ptr projectPart = m_parser->projectPart();

    if (isProjectPartLoadedOrIsFallback(projectPart)) {
        updateTranslationUnitForEditor(projectPart.data());
        requestDiagnostics(projectPart.data());

        m_projectPart = projectPart;
    }
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(m_parser->ifdefedOutBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Run semantic highlighter
    m_semanticHighlighter.run();

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

void ClangEditorDocumentProcessor::requestDiagnostics(CppTools::ProjectPart *projectPart)
{
    if (!m_projectPart || projectPart->id() != m_projectPart->id()) {
        IpcCommunicator &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

        ipcCommunicator.requestDiagnostics({fileContainer(projectPart)});
    }
}

void ClangEditorDocumentProcessor::requestDiagnostics()
{
    // Get diagnostics
    if (m_projectPart) {
        auto  &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();
        ipcCommunicator.requestDiagnostics({filePath(),
                                            m_projectPart->id(),
                                            baseTextDocument()->plainText(),
                                            true,
                                            revision()});
    }
}

ClangBackEnd::FileContainer
ClangEditorDocumentProcessor::fileContainer(CppTools::ProjectPart *projectPart) const
{
    if (projectPart)
        return {filePath(), projectPart->id(), revision()};

    return {filePath(), Utf8String(), revision()};
}

} // namespace Internal
} // namespace ClangCodeModel
