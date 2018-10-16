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

#define qCDebugIpc() qCDebug(ipcLog) << "<===="

using namespace ClangBackEnd;

namespace ClangCodeModel {
namespace Internal {

static bool printAliveMessageHelper()
{
    const bool print = qEnvironmentVariableIntValue("QTC_CLANG_FORCE_VERBOSE_ALIVE");
    if (!print) {
        qCDebug(ipcLog) << "Hint: AliveMessage will not be printed. "
                           "Force it by setting QTC_CLANG_FORCE_VERBOSE_ALIVE=1.";
    }

    return print;
}

static bool printAliveMessage()
{
    static bool print = ipcLog().isDebugEnabled() ? printAliveMessageHelper() : false;
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

void BackendReceiver::addExpectedCompletionsMessage(
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
        const CppTools::SemanticInfo::LocalUseMap &localUses)
{
    QTC_CHECK(!m_referencesTable.contains(ticket));

    QFutureInterface<CppTools::CursorInfo> futureInterface;
    futureInterface.reportStarted();

    const ReferencesEntry entry{futureInterface, localUses};
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

QFuture<CppTools::ToolTipInfo> BackendReceiver::addExpectedToolTipMessage(quint64 ticket)
{
    QTC_CHECK(!m_toolTipsTable.contains(ticket));

    QFutureInterface<CppTools::ToolTipInfo> futureInterface;
    futureInterface.reportStarted();

    m_toolTipsTable.insert(ticket, futureInterface);

    return futureInterface.future();
}

bool BackendReceiver::isExpectingCompletionsMessage() const
{
    return !m_assistProcessorsTable.isEmpty();
}

void BackendReceiver::reset()
{
    // Clean up waiting assist processors
    qDeleteAll(m_assistProcessorsTable.begin(), m_assistProcessorsTable.end());
    m_assistProcessorsTable.clear();

    // Clean up futures for references; TODO: Remove duplication
    for (ReferencesEntry &entry : m_referencesTable) {
        entry.futureInterface.cancel();
        entry.futureInterface.reportFinished();
    }
    m_referencesTable.clear();
    for (QFutureInterface<CppTools::SymbolInfo> &futureInterface : m_followTable) {
        futureInterface.cancel();
        futureInterface.reportFinished();
    }
    m_followTable.clear();
    for (QFutureInterface<CppTools::ToolTipInfo> &futureInterface : m_toolTipsTable) {
        futureInterface.cancel();
        futureInterface.reportFinished();
    }
    m_toolTipsTable.clear();
}

void BackendReceiver::alive()
{
    if (printAliveMessage())
        qCDebugIpc() << "AliveMessage";
    QTC_ASSERT(m_aliveHandler, return);
    m_aliveHandler();
}

void BackendReceiver::echo(const EchoMessage &message)
{
    qCDebugIpc() << message;
}

void BackendReceiver::completions(const CompletionsMessage &message)
{
    qCDebugIpc() << "CompletionsMessage with" << message.codeCompletions.size()
                 << "items";

    const quint64 ticket = message.ticketNumber;
    QScopedPointer<ClangCompletionAssistProcessor> processor(m_assistProcessorsTable.take(ticket));
    if (processor)
        processor->handleAvailableCompletions(message.codeCompletions);
}

void BackendReceiver::annotations(const AnnotationsMessage &message)
{
    qCDebugIpc() << "AnnotationsMessage"
                 << "for" << QFileInfo(message.fileContainer.filePath).fileName() << "with"
                 << message.diagnostics.size() << "diagnostics" << message.tokenInfos.size()
                 << "token infos" << message.skippedPreprocessorRanges.size()
                 << "skipped preprocessor ranges";

    auto processor = ClangEditorDocumentProcessor::get(message.fileContainer.filePath);
    if (!processor)
        return;

    const quint32 documentRevision = message.fileContainer.documentRevision;
    if (message.onlyTokenInfos) {
        processor->updateTokenInfos(message.tokenInfos, documentRevision);
        return;
    }
    processor->updateCodeWarnings(message.diagnostics,
                                  message.firstHeaderErrorDiagnostic,
                                  documentRevision);
    processor->updateHighlighting(message.tokenInfos,
                                  message.skippedPreprocessorRanges,
                                  documentRevision);
}

static
CppTools::CursorInfo::Range toCursorInfoRange(const SourceRangeContainer &sourceRange)
{
    const SourceLocationContainer &start = sourceRange.start;
    const SourceLocationContainer &end = sourceRange.end;
    const unsigned length = end.column - start.column;

    return CppTools::CursorInfo::Range(start.line, start.column, length);
}

static
CppTools::CursorInfo toCursorInfo(const CppTools::SemanticInfo::LocalUseMap &localUses,
                                  const ReferencesMessage &message)
{
    CppTools::CursorInfo result;
    const QVector<SourceRangeContainer> &references = message.references;

    result.areUseRangesForLocalVariable = message.isLocalVariable;
    for (const SourceRangeContainer &reference : references)
        result.useRanges.append(toCursorInfoRange(reference));

    result.useRanges.reserve(references.size());
    result.localUses = localUses;

    return result;
}

static
CppTools::SymbolInfo toSymbolInfo(const FollowSymbolMessage &message)
{
    CppTools::SymbolInfo result;
    const SourceRangeContainer &range = message.result.range;

    const SourceLocationContainer &start = range.start;
    const SourceLocationContainer &end = range.end;
    result.startLine = static_cast<int>(start.line);
    result.startColumn = static_cast<int>(start.column);
    result.endLine = static_cast<int>(end.line);
    result.endColumn = static_cast<int>(end.column);
    result.fileName = start.filePath;

    result.isResultOnlyForFallBack = message.result.isResultOnlyForFallBack;

    return result;
}

void BackendReceiver::references(const ReferencesMessage &message)
{
    qCDebugIpc() << "ReferencesMessage with"
                 << message.references.size() << "references";

    const quint64 ticket = message.ticketNumber;
    const ReferencesEntry entry = m_referencesTable.take(ticket);
    QFutureInterface<CppTools::CursorInfo> futureInterface = entry.futureInterface;
    QTC_CHECK(futureInterface != QFutureInterface<CppTools::CursorInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    futureInterface.reportResult(toCursorInfo(entry.localUses, message));
    futureInterface.reportFinished();
}

static TextEditor::HelpItem::Category toHelpItemCategory(ToolTipInfo::QdocCategory category)
{
    switch (category) {
    case ToolTipInfo::Unknown:
        return TextEditor::HelpItem::Unknown;
    case ToolTipInfo::ClassOrNamespace:
        return TextEditor::HelpItem::ClassOrNamespace;
    case ToolTipInfo::Enum:
        return TextEditor::HelpItem::Enum;
    case ToolTipInfo::Typedef:
        return TextEditor::HelpItem::Typedef;
    case ToolTipInfo::Macro:
        return TextEditor::HelpItem::Macro;
    case ToolTipInfo::Brief:
        return TextEditor::HelpItem::Brief;
    case ToolTipInfo::Function:
        return TextEditor::HelpItem::Function;
    }

    return TextEditor::HelpItem::Unknown;
}

static QStringList toStringList(const Utf8StringVector &utf8StringVector)
{
    QStringList list;
    list.reserve(utf8StringVector.size());

    for (const Utf8String &utf8String : utf8StringVector)
        list << utf8String.toString();

    return list;
}

static CppTools::ToolTipInfo toToolTipInfo(const ToolTipMessage &message)
{
    CppTools::ToolTipInfo info;

    const ToolTipInfo &backendInfo = message.toolTipInfo;

    info.text = backendInfo.text;
    info.briefComment = backendInfo.briefComment;

    info.qDocIdCandidates = toStringList(backendInfo.qdocIdCandidates);
    info.qDocMark = backendInfo.qdocMark;
    info.qDocCategory = toHelpItemCategory(backendInfo.qdocCategory);

    info.sizeInBytes = backendInfo.sizeInBytes;

    return info;
}

void BackendReceiver::tooltip(const ToolTipMessage &message)
{
    qCDebugIpc() << "ToolTipMessage" << message.toolTipInfo.text;

    const quint64 ticket = message.ticketNumber;
    QFutureInterface<CppTools::ToolTipInfo> futureInterface = m_toolTipsTable.take(ticket);
    QTC_CHECK(futureInterface != QFutureInterface<CppTools::ToolTipInfo>());

    if (futureInterface.isCanceled())
        return; // A new request was issued making this one outdated.

    futureInterface.reportResult(toToolTipInfo(message));
    futureInterface.reportFinished();
}

void BackendReceiver::followSymbol(const ClangBackEnd::FollowSymbolMessage &message)
{
    qCDebugIpc() << "FollowSymbolMessage with"
                 << message.result;

    const quint64 ticket = message.ticketNumber;
    QFutureInterface<CppTools::SymbolInfo> futureInterface = m_followTable.take(ticket);
    QTC_CHECK(futureInterface != QFutureInterface<CppTools::SymbolInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    futureInterface.reportResult(toSymbolInfo(message));
    futureInterface.reportFinished();
}

} // namespace Internal
} // namespace ClangCodeModel
