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

#include <clangsupport/clangcodemodelclientmessages.h>

#include <QLoggingCategory>
#include <QTextBlock>

#include <utils/qtcassert.h>

#define qCDebugIpc() qCDebug(ipcLog) << "<===="

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

BackendReceiver::BackendReceiver() = default;

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

void BackendReceiver::cancelProcessor(TextEditor::IAssistProcessor *processor)
{
    for (auto it = m_assistProcessorsTable.cbegin(), end = m_assistProcessorsTable.cend();
            it != end; ++it)
    {
        if (it.value() == processor) {
            m_assistProcessorsTable.erase(it);
            return;
        }
    }
}

void BackendReceiver::deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget)
{
    QList<quint64> toRemove;
    for (auto it = m_assistProcessorsTable.cbegin(), end = m_assistProcessorsTable.cend();
            it != end; ++it)
    {
        ClangCompletionAssistProcessor *assistProcessor = it.value();
        if (assistProcessor->textEditorWidget() == textEditorWidget) {
            delete assistProcessor;
            toRemove.append(it.key());
        }
    }
    for (quint64 item : toRemove)
        m_assistProcessorsTable.remove(item);
}

QFuture<CppEditor::CursorInfo> BackendReceiver::addExpectedReferencesMessage(
        quint64 ticket,
        const CppEditor::SemanticInfo::LocalUseMap &localUses)
{
    QTC_CHECK(!m_referencesTable.contains(ticket));

    QFutureInterface<CppEditor::CursorInfo> futureInterface;
    futureInterface.reportStarted();

    const ReferencesEntry entry{futureInterface, localUses};
    m_referencesTable.insert(ticket, entry);

    return futureInterface.future();
}

QFuture<CppEditor::SymbolInfo> BackendReceiver::addExpectedRequestFollowSymbolMessage(quint64 ticket)
{
    QTC_CHECK(!m_followTable.contains(ticket));

    QFutureInterface<CppEditor::SymbolInfo> futureInterface;
    futureInterface.reportStarted();

    m_followTable.insert(ticket, futureInterface);

    return futureInterface.future();
}

QFuture<CppEditor::ToolTipInfo> BackendReceiver::addExpectedToolTipMessage(quint64 ticket)
{
    QTC_CHECK(!m_toolTipsTable.contains(ticket));

    QFutureInterface<CppEditor::ToolTipInfo> futureInterface;
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
    for (ClangCompletionAssistProcessor *processor : m_assistProcessorsTable)
        processor->setAsyncProposalAvailable(nullptr);
    m_assistProcessorsTable.clear();

    // Clean up futures for references; TODO: Remove duplication
    for (ReferencesEntry &entry : m_referencesTable) {
        entry.futureInterface.cancel();
        entry.futureInterface.reportFinished();
    }
    m_referencesTable.clear();
    for (QFutureInterface<CppEditor::SymbolInfo> &futureInterface : m_followTable) {
        futureInterface.cancel();
        futureInterface.reportFinished();
    }
    m_followTable.clear();
    for (QFutureInterface<CppEditor::ToolTipInfo> &futureInterface : m_toolTipsTable) {
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

void BackendReceiver::echo(const ClangBackEnd::EchoMessage &message)
{
    qCDebugIpc() << message;
}

void BackendReceiver::completions(const ClangBackEnd::CompletionsMessage &message)
{
    qCDebugIpc() << "CompletionsMessage with" << message.codeCompletions.size()
                 << "items";

    const quint64 ticket = message.ticketNumber;
    if (ClangCompletionAssistProcessor *processor = m_assistProcessorsTable.take(ticket))
        processor->handleAvailableCompletions(message.codeCompletions);
}

void BackendReceiver::annotations(const ClangBackEnd::AnnotationsMessage &message)
{
    qCDebugIpc() << "AnnotationsMessage"
                 << "for" << QFileInfo(message.fileContainer.filePath).fileName() << "with"
                 << message.diagnostics.size() << "diagnostics" << message.tokenInfos.size()
                 << "token infos" << message.skippedPreprocessorRanges.size()
                 << "skipped preprocessor ranges";
}

static
CppEditor::CursorInfo::Range toCursorInfoRange(const ClangBackEnd::SourceRangeContainer &sourceRange)
{
    const ClangBackEnd::SourceLocationContainer &start = sourceRange.start;
    const ClangBackEnd::SourceLocationContainer &end = sourceRange.end;
    const int length = end.column - start.column;

    return {start.line, start.column, length};
}

static
CppEditor::CursorInfo toCursorInfo(const CppEditor::SemanticInfo::LocalUseMap &localUses,
                                  const ClangBackEnd::ReferencesMessage &message)
{
    CppEditor::CursorInfo result;
    const QVector<ClangBackEnd::SourceRangeContainer> &references = message.references;

    result.areUseRangesForLocalVariable = message.isLocalVariable;
    for (const ClangBackEnd::SourceRangeContainer &reference : references)
        result.useRanges.append(toCursorInfoRange(reference));

    result.useRanges.reserve(references.size());
    result.localUses = localUses;

    return result;
}

static
CppEditor::SymbolInfo toSymbolInfo(const ClangBackEnd::FollowSymbolMessage &message)
{
    CppEditor::SymbolInfo result;
    const ClangBackEnd::SourceRangeContainer &range = message.result.range;

    const ClangBackEnd::SourceLocationContainer &start = range.start;
    const ClangBackEnd::SourceLocationContainer &end = range.end;
    result.startLine = start.line;
    result.startColumn = start.column;
    result.endLine = end.line;
    result.endColumn = end.column;
    result.fileName = start.filePath;

    result.isResultOnlyForFallBack = message.result.isResultOnlyForFallBack;

    return result;
}

void BackendReceiver::references(const ClangBackEnd::ReferencesMessage &message)
{
    qCDebugIpc() << "ReferencesMessage with"
                 << message.references.size() << "references";

    const quint64 ticket = message.ticketNumber;
    const ReferencesEntry entry = m_referencesTable.take(ticket);
    QFutureInterface<CppEditor::CursorInfo> futureInterface = entry.futureInterface;
    QTC_CHECK(futureInterface != QFutureInterface<CppEditor::CursorInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    futureInterface.reportResult(toCursorInfo(entry.localUses, message));
    futureInterface.reportFinished();
}

static Core::HelpItem::Category toHelpItemCategory(ClangBackEnd::ToolTipInfo::QdocCategory category)
{
    switch (category) {
    case ClangBackEnd::ToolTipInfo::Unknown:
        return Core::HelpItem::Unknown;
    case ClangBackEnd::ToolTipInfo::ClassOrNamespace:
        return Core::HelpItem::ClassOrNamespace;
    case ClangBackEnd::ToolTipInfo::Enum:
        return Core::HelpItem::Enum;
    case ClangBackEnd::ToolTipInfo::Typedef:
        return Core::HelpItem::Typedef;
    case ClangBackEnd::ToolTipInfo::Macro:
        return Core::HelpItem::Macro;
    case ClangBackEnd::ToolTipInfo::Brief:
        return Core::HelpItem::Brief;
    case ClangBackEnd::ToolTipInfo::Function:
        return Core::HelpItem::Function;
    }

    return Core::HelpItem::Unknown;
}

static QStringList toStringList(const Utf8StringVector &utf8StringVector)
{
    QStringList list;
    list.reserve(utf8StringVector.size());

    for (const Utf8String &utf8String : utf8StringVector)
        list << utf8String.toString();

    return list;
}

static CppEditor::ToolTipInfo toToolTipInfo(const ClangBackEnd::ToolTipMessage &message)
{
    CppEditor::ToolTipInfo info;

    const ClangBackEnd::ToolTipInfo &backendInfo = message.toolTipInfo;

    info.text = backendInfo.text;
    info.briefComment = backendInfo.briefComment;

    info.qDocIdCandidates = toStringList(backendInfo.qdocIdCandidates);
    info.qDocMark = backendInfo.qdocMark;
    info.qDocCategory = toHelpItemCategory(backendInfo.qdocCategory);
    info.value = backendInfo.value;
    info.sizeInBytes = backendInfo.sizeInBytes;

    return info;
}

void BackendReceiver::tooltip(const ClangBackEnd::ToolTipMessage &message)
{
    qCDebugIpc() << "ToolTipMessage" << message.toolTipInfo.text;

    const quint64 ticket = message.ticketNumber;
    QFutureInterface<CppEditor::ToolTipInfo> futureInterface = m_toolTipsTable.take(ticket);
    QTC_CHECK(futureInterface != QFutureInterface<CppEditor::ToolTipInfo>());

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
    QFutureInterface<CppEditor::SymbolInfo> futureInterface = m_followTable.take(ticket);
    QTC_CHECK(futureInterface != QFutureInterface<CppEditor::SymbolInfo>());

    if (futureInterface.isCanceled())
        return; // Editor document closed or a new request was issued making this result outdated.

    futureInterface.reportResult(toSymbolInfo(message));
    futureInterface.reportFinished();
}

} // namespace Internal
} // namespace ClangCodeModel
