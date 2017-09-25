/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangbackendreceiver.h"

#include "clangbackendlogging.h"

#include "clangcompletionassistprocessor.h"
#include "clangeditordocumentprocessor.h"

#include <cpptools/cpptoolsbridge.h>

#include <clangsupport/clangcodemodelclientmessages.h>

#include <QLoggingCategory>
#include <QTextBlock>

#include <utils/qtcassert.h>

static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.ipc")

using namespace ClangBackEnd;

namespace ClangCodeModel {
namespace Internal {

static bool printAliveMessageHelper()
{
    const bool print = qEnvironmentVariableIntValue("QTC_CLANG_FORCE_VERBOSE_ALIVE");
    if (!print) {
        qCDebug(log) << "Hint: AliveMessage will not be printed. "
                        "Force it by setting QTC_CLANG_FORCE_VERBOSE_ALIVE=1.";
    }

    return print;
}

static bool printAliveMessage()
{
    static bool print = log().isDebugEnabled() ? printAliveMessageHelper() : false;
    return print;
}

BackendReceiver::BackendReceiver()
{
}

BackendReceiver::~BackendReceiver()
{
    reset();
}

void BackendReceiver::setAliveHandler(const BackendReceiver::AliveHandler &handler)
{
    m_aliveHandler = handler;
}

void BackendReceiver::addExpectedCodeCompletedMessage(
        quint64 ticket,
        ClangCompletionAssistProcessor *processor)
{
    QTC_ASSERT(processor, return);
    QTC_CHECK(!m_assistProcessorsTable.contains(ticket));
    m_assistProcessorsTable.insert(ticket, processor);
}

void BackendReceiver::deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget)
{
    QMutableHashIterator<quint64, ClangCompletionAssistProcessor *> it(m_assistProcessorsTable);
    while (it.hasNext()) {
        it.next();
        ClangCompletionAssistProcessor *assistProcessor = it.value();
        if (assistProcessor->textEditorWidget() == textEditorWidget) {
            delete assistProcessor;
            it.remove();
        }
    }
}

QFuture<CppTools::CursorInfo> BackendReceiver::addExpectedReferencesMessage(
        quint64 ticket,
        QTextDocument *textDocument,
        const CppTools::SemanticInfo::LocalUseMap &localUses)
{
    QTC_CHECK(textDocument);
    QTC_CHECK(!m_referencesTable.contains(ticket));

    QFutureInterface<CppTools::CursorInfo> futureInterface;
    futureInterface.reportStarted();

    const ReferencesEntry entry{futureInterface, textDocument, localUses};
    m_referencesTable.insert(ticket, entry);

    return futureInterface.future();
}

QFuture<CppTools::SymbolInfo> BackendReceiver::addExpectedRequestFollowSymbolMessage(quint64 ticket)
{
    QTC_CHECK(!m_followTable.contains(ticket));

    QFutureInterface<CppTools::SymbolInfo> futureInterface;
    futureInterface.reportStarted();

    m_followTable.insert(ticket, futureInterface);

    return futureInterface.future();
}

bool BackendReceiver::isExpectingCodeCompletedMessage() const
{
    return !m_assistProcessorsTable.isEmpty();
}

void BackendReceiver::reset()
{
    // Clean up waiting assist processors
    qDeleteAll(m_assistProcessorsTable.begin(), m_assistProcessorsTable.end());
    m_assistProcessorsTable.clear();

    // Clean up futures for references
    for (ReferencesEntry &entry : m_referencesTable)
        entry.futureInterface.cancel();
    m_referencesTable.clear();
    for (QFutureInterface<CppTools::SymbolInfo> &futureInterface : m_followTable)
        futureInterface.cancel();
    m_followTable.clear();
}

void BackendReceiver::alive()
{
    if (printAliveMessage())
        qCDebug(log) << "<<< AliveMessage";
    QTC_ASSERT(m_aliveHandler, return);
    m_aliveHandler();
}

void BackendReceiver::echo(const EchoMessage &message)
{
    qCDebug(log) << "<<<" << message;
}

void BackendReceiver::codeCompleted(const CodeCompletedMessage &message)
{
    qCDebug(log) << "<<< CodeCompletedMessage with" << message.codeCompletions().size() << "items";

    const quint64 ticket = message.ticketNumber();
    QScopedPointer<ClangCompletionAssistProcessor> processor(m_assistProcessorsTable.take(ticket));
    if (processor) {
        processor->handleAvailableCompletions(message.codeCompletions(),
                                              message.neededCorrection());
    }
}

void BackendReceiver::documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message)
{
    qCDebug(log) << "<<< DocumentAnnotationsChangedMessage with"
                 << message.diagnostics().size() << "diagnostics"
                 << message.highlightingMarks().size() << "highlighting marks"
                 << message.skippedPreprocessorRanges().size() << "skipped preprocessor ranges";

    auto processor = ClangEditorDocumentProcessor::get(message.fileContainer().filePath());

    if (processor) {
        const QString projectPartId = message.fileContainer().projectPartId();
        const QString filePath = message.fileContainer().filePath();
        const QString documentProjectPartId = CppTools::CppToolsBridge::projectPartIdForFile(filePath);
        if (projectPartId == documentProjectPartId) {
            const quint32 documentRevision = message.fileContainer().documentRevision();
            processor->updateCodeWarnings(message.diagnostics(),
                                          message.firstHeaderErrorDiagnostic(),
                                          documentRevision);
            processor->updateHighlighting(message.highlightingMarks(),
                                          message.skippedPreprocessorRanges(),
                                          documentRevision);
        }
    }
}

static
CppTools::CursorInfo::Range toCursorInfoRange(const QTextDocument &textDocument,
                                              const SourceRangeContainer &sourceRange)
{
    const SourceLocationContainer start = sourceRange.start();
    const SourceLocationContainer end = sourceRange.end();
    const unsigned length = end.column() - start.column();

    const QTextBlock block = textDocument.findBlockByNumber(static_cast<int>(start.line()) - 1);
    const int shift = ClangCodeModel::Utils::extraUtf8CharsShift(block.text(),
                                                                 static_cast<int>(start.column()));
    const uint column = start.column() - static_cast<uint>(shift);

    return CppTools::CursorInfo::Range(start.line(), column, length);
}

static
CppTools::CursorInfo toCursorInfo(const QTextDocument &textDocument,
                                  const CppTools::SemanticInfo::LocalUseMap &localUses,
                                  const ReferencesMessage &message)
{
    CppTools::CursorInfo result;
    const QVector<SourceRangeContainer> references = message.references();

    result.areUseRangesForLocalVariable = message.isLocalVariable();
    for (const SourceRangeContainer &reference : references)
        result.useRanges.append(toCursorInfoRange(textDocument, reference));

    result.useRanges.reserve(references.size());
    result.localUses = localUses;

    return result;
}

static
CppTools::SymbolInfo toSymbolInfo(const FollowSymbolMessage &message)
{
    CppTools::SymbolInfo result;
    const SourceRangeContainer &range = message.sourceRange();

    const SourceLocationContainer start = range.start();
    const SourceLocationContainer end = range.end();
    result.startLine = static_cast<int>(start.line());
    result.startColumn = static_cast<int>(start.column());
    result.endLine = static_cast<int>(end.line());
    result.endColumn = static_cast<int>(end.column());
    result.fileName = start.filePath();

    return result;
}

void BackendReceiver::references(const ReferencesMessage &message)
{
    qCDebug(log) << "<<< ReferencesMessage with"
                 << message.references().size() << "references";

    const quint64 ticket = message.ticketNumber();
    const ReferencesEntry entry = m_referencesTable.take(ticket);
    QFutureInterface<CppTools::CursorInfo> futureInterface = entry.futureInterface;
    QTC_CHECK(futureInterface != QFutureInterface<CppTools::CursorInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    QTC_ASSERT(entry.textDocument, return);
    futureInterface.reportResult(toCursorInfo(*entry.textDocument, entry.localUses, message));
    futureInterface.reportFinished();
}

void BackendReceiver::followSymbol(const ClangBackEnd::FollowSymbolMessage &message)
{
    qCDebug(log) << "<<< FollowSymbolMessage with"
                 << message.sourceRange() << "range";

    const quint64 ticket = message.ticketNumber();
    QFutureInterface<CppTools::SymbolInfo> futureInterface = m_followTable.take(ticket);
    QTC_CHECK(futureInterface != QFutureInterface<CppTools::SymbolInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    futureInterface.reportResult(toSymbolInfo(message));
    futureInterface.reportFinished();
}

} // namespace Internal
} // namespace ClangCodeModel
