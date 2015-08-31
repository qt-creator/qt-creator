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

#include "clangmodelmanagersupport.h"
#include "clangutils.h"
#include "cppcreatemarkers.h"
#include "diagnostic.h"
#include "pchinfo.h"

#include <diagnosticcontainer.h>
#include <sourcelocationcontainer.h>

#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cppworkingcopy.h>

#include <texteditor/texteditor.h>

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
    , m_modelManagerSupport(modelManagerSupport)
    , m_parser(document->filePath().toString())
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

    connect(CppTools::CppModelManager::instance(), &CppTools::CppModelManager::projectPartsRemoved,
            this, &ClangEditorDocumentProcessor::onProjectPartsRemoved);

    m_semanticHighlighter.setHighlightingRunner(
        [this]() -> QFuture<TextEditor::HighlightingResult> {
            const int firstLine = 1;
            const int lastLine = baseTextDocument()->document()->blockCount();

            CreateMarkers *createMarkers = CreateMarkers::create(m_parser.semanticMarker(),
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
        m_modelManagerSupport->ipcCommunicator().unregisterFilesForCodeCompletion(
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

CppTools::BaseEditorDocumentParser *ClangEditorDocumentProcessor::parser()
{
    return &m_parser;
}

CPlusPlus::Snapshot ClangEditorDocumentProcessor::snapshot()
{
   return m_builtinProcessor.snapshot();
}

bool ClangEditorDocumentProcessor::isParserRunning() const
{
    return m_parserWatcher.isRunning();
}

CppTools::ProjectPart::Ptr ClangEditorDocumentProcessor::projectPart() const
{
    return m_projectPart;
}

void ClangEditorDocumentProcessor::updateCodeWarnings(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                                                      uint documentRevision)
{
    if (documentRevision == revision()) {
        const auto codeWarnings = generateDiagnosticHints(diagnostics);
        emit codeWarningsUpdated(revision(), codeWarnings);
    }
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor *>(BaseEditorDocumentProcessor::get(filePath));
}

void ClangEditorDocumentProcessor::updateProjectPartAndTranslationUnitForCompletion()
{
    const CppTools::ProjectPart::Ptr projectPart = m_parser.projectPart();
    QTC_ASSERT(projectPart, return);

    updateTranslationUnitForCompletion(*projectPart.data());
    requestDiagnostics(*projectPart.data());

    m_projectPart = projectPart;
}

void ClangEditorDocumentProcessor::onParserFinished()
{
    if (revision() != m_parserRevision)
        return;

    // Emit ifdefed out blocks
    const auto ifdefoutBlocks = toTextEditorBlocks(m_parser.ifdefedOutBlocks());
    emit ifdefedOutBlocksUpdated(revision(), ifdefoutBlocks);

    // Run semantic highlighter
    m_semanticHighlighter.run();

    updateProjectPartAndTranslationUnitForCompletion();
}

void ClangEditorDocumentProcessor::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (m_projectPart && projectPartIds.contains(m_projectPart->id()))
        m_projectPart.clear();
}

void ClangEditorDocumentProcessor::updateTranslationUnitForCompletion(CppTools::ProjectPart &projectPart)
{
    QTC_ASSERT(m_modelManagerSupport, return);
    IpcCommunicator &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

    if (m_projectPart) {
        if (projectPart.id() != m_projectPart->id()) {
            auto container1 = ClangBackEnd::FileContainer(filePath(),
                                                          m_projectPart->id(),
                                                          revision());
            ipcCommunicator.unregisterFilesForCodeCompletion({container1});

            auto container2 = ClangBackEnd::FileContainer(filePath(),
                                                          projectPart.id(),
                                                          revision());
            ipcCommunicator.registerFilesForCodeCompletion({container2});
        }
    } else {
        auto container = ClangBackEnd::FileContainer(filePath(),
                                                     projectPart.id(),
                                                     revision());
        ipcCommunicator.registerFilesForCodeCompletion({container});
    }
}

namespace {
bool isWarningOrNote(ClangBackEnd::DiagnosticSeverity severity)
{
    using ClangBackEnd::DiagnosticSeverity;
    switch (severity) {
        case DiagnosticSeverity::Ignored:
        case DiagnosticSeverity::Note:
        case DiagnosticSeverity::Warning: return true;
        case DiagnosticSeverity::Error:
        case DiagnosticSeverity::Fatal: return false;
    }

    Q_UNREACHABLE();
}

bool isHelpfulChildDiagnostic(const ClangBackEnd::DiagnosticContainer &parentDiagnostic,
                              const ClangBackEnd::DiagnosticContainer &childDiagnostic)
{
    auto parentLocation = parentDiagnostic.location();
    auto childLocation = childDiagnostic.location();

    return parentLocation == childLocation;
}

QString diagnosticText(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    QString text = diagnostic.category().toString()
            + QStringLiteral(" ")
            + diagnostic.text().toString();
    if (!diagnostic.enableOption().isEmpty()) {
        text += QStringLiteral(" (clang option: ")
                + diagnostic.enableOption().toString()
                + QStringLiteral(" disable with: ")
                + diagnostic.disableOption().toString()
                + QStringLiteral(")");
    }

    for (auto &&childDiagnostic : diagnostic.children()) {
        if (isHelpfulChildDiagnostic(diagnostic, childDiagnostic))
            text += QStringLiteral("\n  ") + childDiagnostic.text().toString();
    }

    return text;
}

template <class Condition>
std::vector<ClangBackEnd::DiagnosticContainer>
filterDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                  const Condition &condition)
{
    std::vector<ClangBackEnd::DiagnosticContainer> filteredDiagnostics;

    std::copy_if(diagnostics.cbegin(),
                 diagnostics.cend(),
                 std::back_inserter(filteredDiagnostics),
                 condition);

    return filteredDiagnostics;
}

std::vector<ClangBackEnd::DiagnosticContainer>
filterInterestingWarningDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                                    QString &&documentFilePath)
{
    auto isLocalWarning = [documentFilePath] (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return isWarningOrNote(diagnostic.severity())
            && diagnostic.location().filePath() == documentFilePath;
    };

    return filterDiagnostics(diagnostics, isLocalWarning);
}

std::vector<ClangBackEnd::DiagnosticContainer>
filterInterestingErrorsDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                                   QString &&documentFilePath)
{
    auto isLocalWarning = [documentFilePath] (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return !isWarningOrNote(diagnostic.severity())
            && diagnostic.location().filePath() == documentFilePath;
    };

    return filterDiagnostics(diagnostics, isLocalWarning);
}

QTextEdit::ExtraSelection createExtraSelections(const QTextCharFormat &mainformat,
                                               const QTextCursor &cursor,
                                               const QString &diagnosticText)
{
    QTextEdit::ExtraSelection extraSelection;

    extraSelection.format = mainformat;
    extraSelection.cursor = cursor;
    extraSelection.format.setToolTip(diagnosticText);

    return extraSelection;
}

void addRangeSelections(const ClangBackEnd::DiagnosticContainer &diagnostic,
                        QTextDocument *textDocument,
                        const QTextCharFormat &rangeFormat,
                        const QString &diagnosticText,
                        QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&range : diagnostic.ranges()) {
        QTextCursor cursor(textDocument);
        cursor.setPosition(int(range.start().offset()));
        cursor.setPosition(int(range.end().offset()), QTextCursor::KeepAnchor);

        auto extraSelection = createExtraSelections(rangeFormat, cursor, diagnosticText);

        extraSelections.push_back(std::move(extraSelection));
    }
}

QTextCursor createSelectionCursor(QTextDocument *textDocument, uint position)
{
    QTextCursor cursor(textDocument);
    cursor.setPosition(int(position));
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    if (!cursor.hasSelection()) {
        cursor.setPosition(int(position) - 1);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    }

    return cursor;
}

void addSelections(const std::vector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                   QTextDocument *textDocument,
                   const QTextCharFormat &mainFormat,
                   const QTextCharFormat &rangeFormat,
                   QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&diagnostic : diagnostics) {
        auto cursor = createSelectionCursor(textDocument, diagnostic.location().offset());

        auto text = diagnosticText(diagnostic);
        auto extraSelection = createExtraSelections(mainFormat, cursor, text);

        addRangeSelections(diagnostic, textDocument, rangeFormat, text, extraSelections);

        extraSelections.push_back(std::move(extraSelection));
    }
}

void addWarningSelections(const std::vector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                          QTextDocument *textDocument,
                          QList<QTextEdit::ExtraSelection> &extraSelections)
{
    QTextCharFormat warningFormat;
    warningFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    warningFormat.setUnderlineColor(QColor(180, 180, 0, 255));

    QTextCharFormat warningRangeFormat;
    warningRangeFormat.setUnderlineStyle(QTextCharFormat::DotLine);
    warningRangeFormat.setUnderlineColor(QColor(180, 180, 0, 255));

    addSelections(diagnostics, textDocument, warningFormat, warningRangeFormat, extraSelections);
}

void addErrorSelections(const std::vector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                         QTextDocument *textDocument,
                         QList<QTextEdit::ExtraSelection> &extraSelections)
{
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    errorFormat.setUnderlineColor(QColor(255, 0, 0, 255));

    QTextCharFormat errorRangeFormat;
    errorRangeFormat.setUnderlineStyle(QTextCharFormat::DotLine);
    errorRangeFormat.setUnderlineColor(QColor(255, 0, 0, 255));

    addSelections(diagnostics, textDocument, errorFormat, errorRangeFormat, extraSelections);
}

}  // anonymous namespace

QList<QTextEdit::ExtraSelection>
ClangEditorDocumentProcessor::generateDiagnosticHints(const QVector<ClangBackEnd::DiagnosticContainer> &allDiagnostics)
{
    const auto warningDiagnostic = filterInterestingWarningDiagnostics(allDiagnostics, filePath());
    const auto errorDiagnostic = filterInterestingErrorsDiagnostics(allDiagnostics, filePath());

    m_clangTextMarks.clear();
    m_clangTextMarks.reserve(warningDiagnostic.size() + errorDiagnostic.size());

    addClangTextMarks(warningDiagnostic);
    addClangTextMarks(errorDiagnostic);

    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.reserve(int(warningDiagnostic.size() + errorDiagnostic.size()));

    addWarningSelections(warningDiagnostic, textDocument(), extraSelections);
    addErrorSelections(errorDiagnostic, textDocument(), extraSelections);

    return extraSelections;
}

void ClangEditorDocumentProcessor::addClangTextMarks(const std::vector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    QTC_ASSERT(m_clangTextMarks.size() + diagnostics.size() <= m_clangTextMarks.capacity(), return);

    for (auto &&diagnostic : diagnostics) {
        m_clangTextMarks.emplace_back(filePath(),
                                      diagnostic.location().line(),
                                      diagnostic.severity());

        ClangTextMark &textMark = m_clangTextMarks.back();

        textMark.setBaseTextDocument(baseTextDocument());

        baseTextDocument()->addMark(&textMark);
    }
}

void ClangEditorDocumentProcessor::requestDiagnostics(CppTools::ProjectPart &projectPart)
{
    if (!m_projectPart || projectPart.id() != m_projectPart->id()) {
        IpcCommunicator &ipcCommunicator = m_modelManagerSupport->ipcCommunicator();

        ipcCommunicator.requestDiagnostics({filePath(),
                                            projectPart.id(),
                                            Utf8String(),
                                            false,
                                            revision()});
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

} // namespace Internal
} // namespace ClangCodeModel
