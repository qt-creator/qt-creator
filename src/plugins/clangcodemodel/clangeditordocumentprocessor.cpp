/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangeditordocumentprocessor.h"

#include "cppcreatemarkers.h"
#include "diagnostic.h"
#include "pchinfo.h"

#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cppworkingcopy.h>

#include <cplusplus/CppDocument.h>

#include <utils/qtcassert.h>
#include <utils/QtConcurrentTools>

static const bool DebugTiming = qgetenv("QTC_CLANG_VERBOSE") == "1";

namespace {

typedef CPlusPlus::Document::DiagnosticMessage CppToolsDiagnostic;
QList<CppToolsDiagnostic> toCppToolsDiagnostics(
        const QString &filePath,
        const QList<ClangCodeModel::Diagnostic> &diagnostics)
{
    using namespace ClangCodeModel;

    QList<CppToolsDiagnostic> converted;
    foreach (const ClangCodeModel::Diagnostic &d, diagnostics) {
        if (DebugTiming)
            qDebug() << d.severityAsString() << d.location() << d.spelling();

        if (d.location().fileName() != filePath)
            continue;

        // TODO: retrieve fix-its for this diagnostic

        int level;
        switch (d.severity()) {
        case Diagnostic::Fatal: level = CppToolsDiagnostic::Fatal; break;
        case Diagnostic::Error: level = CppToolsDiagnostic::Error; break;
        case Diagnostic::Warning: level = CppToolsDiagnostic::Warning; break;
        default: continue;
        }
        converted.append(CppToolsDiagnostic(level, d.location().fileName(), d.location().line(),
                                           d.location().column(), d.spelling(), d.length()));
    }

    return converted;
}

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

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(TextEditor::BaseTextDocument *document)
    : BaseEditorDocumentProcessor(document)
    , m_parser(document->filePath())
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

            CreateMarkers *createMarkers = CreateMarkers::create(m_parser.semanticMarker(),
                                                                 baseTextDocument()->filePath(),
                                                                 firstLine, lastLine);
            return createMarkers->start();
        });
}

ClangEditorDocumentProcessor::~ClangEditorDocumentProcessor()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();
}

void ClangEditorDocumentProcessor::run()
{
    // Run clang parser
    const CppTools::WorkingCopy workingCopy
        = CppTools::CppModelManager::instance()->workingCopy();

    disconnect(&m_parserWatcher, &QFutureWatcher<void>::finished,
               this, &ClangEditorDocumentProcessor::onParserFinished);
    m_parserWatcher.cancel();
    m_parserWatcher.setFuture(QFuture<void>());

    m_parserRevision = revision();
    connect(&m_parserWatcher, &QFutureWatcher<void>::finished,
            this, &ClangEditorDocumentProcessor::onParserFinished);
    const QFuture<void> future = QtConcurrent::run(&runParser, parser(), workingCopy);
    m_parserWatcher.setFuture(future);

    // Run builtin processor
    m_builtinProcessor.run();
}

void ClangEditorDocumentProcessor::semanticRehighlight(bool force)
{
    m_builtinProcessor.semanticRehighlight(force);
}

CppTools::SemanticInfo ClangEditorDocumentProcessor::recalculateSemanticInfo()
{
    return m_builtinProcessor.recalculateSemanticInfo();
}

CppTools::BaseEditorDocumentParser *ClangEditorDocumentProcessor::parser()
{
    return &m_parser;
}

bool ClangEditorDocumentProcessor::isParserRunning() const
{
    return m_parserWatcher.isRunning();
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(m_parser.ifdefedOutBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Emit code warnings
    const auto diagnostics = toCppToolsDiagnostics(filePath(), m_parser.diagnostics());
    const auto codeWarnings = toTextEditorSelections(diagnostics, textDocument());
    emit codeWarningsUpdated(revision(), codeWarnings);

    // Run semantic highlighter
    m_semanticHighlighter.run();
}

} // namespace ClangCodeModel
