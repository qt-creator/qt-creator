// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppuseselectionsupdater.h"

#include "cppeditordocument.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"

#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QCoreApplication>
#include <QTextBlock>
#include <QTextCursor>

enum { updateUseSelectionsInternalInMs = 500 };

namespace CppEditor {
namespace Internal {

CppUseSelectionsUpdater::CppUseSelectionsUpdater(CppEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(updateUseSelectionsInternalInMs);
    connect(&m_timer, &QTimer::timeout, this, [this] { update(); });
}

CppUseSelectionsUpdater::~CppUseSelectionsUpdater()
{
    if (m_runnerWatcher)
        m_runnerWatcher->cancel();
}

void CppUseSelectionsUpdater::scheduleUpdate()
{
    m_timer.start();
}

void CppUseSelectionsUpdater::abortSchedule()
{
    m_timer.stop();
}

CppUseSelectionsUpdater::RunnerInfo CppUseSelectionsUpdater::update(CallType callType)
{
    auto *cppEditorWidget = qobject_cast<CppEditorWidget *>(m_editorWidget);
    QTC_ASSERT(cppEditorWidget, return RunnerInfo::FailedToStart);

    auto *cppEditorDocument = qobject_cast<CppEditorDocument *>(cppEditorWidget->textDocument());
    QTC_ASSERT(cppEditorDocument, return RunnerInfo::FailedToStart);

    m_updateSelections = !CppModelManager::usesClangd(cppEditorDocument)
                         && !m_editorWidget->isRenaming();

    CursorInfoParams params;
    params.semanticInfo = cppEditorWidget->semanticInfo();
    params.textCursor = Utils::Text::wordStartCursor(cppEditorWidget->textCursor());

    if (callType == CallType::Asynchronous) {
        if (isSameIdentifierAsBefore(params.textCursor))
            return RunnerInfo::AlreadyUpToDate;

        if (m_runnerWatcher)
            m_runnerWatcher->cancel();

        m_runnerWatcher.reset(new QFutureWatcher<CursorInfo>);
        connect(m_runnerWatcher.get(), &QFutureWatcherBase::finished,
                this, &CppUseSelectionsUpdater::onFindUsesFinished);

        m_runnerRevision = m_editorWidget->document()->revision();
        m_runnerWordStartPosition = params.textCursor.position();

        m_runnerWatcher->setFuture(cppEditorDocument->cursorInfo(params));
        Utils::futureSynchronizer()->addFuture(m_runnerWatcher->future());
        return RunnerInfo::Started;
    } else { // synchronous case
        abortSchedule();

        const int startRevision = cppEditorDocument->document()->revision();
        QFuture<CursorInfo> future = cppEditorDocument->cursorInfo(params);
        if (future.isCanceled())
            return RunnerInfo::Invalid;

        // QFuture::waitForFinished seems to block completely, not even
        // allowing to process events from QLocalSocket.
        while (!future.isFinished()) {
            if (future.isCanceled())
                return RunnerInfo::Invalid;

            QTC_ASSERT(startRevision == cppEditorDocument->document()->revision(),
                       return RunnerInfo::Invalid);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        processResults(future.result());
        return RunnerInfo::Invalid;
    }
}

bool CppUseSelectionsUpdater::isSameIdentifierAsBefore(const QTextCursor &cursorAtWordStart) const
{
    return m_runnerRevision != -1
        && m_runnerRevision == m_editorWidget->document()->revision()
        && m_runnerWordStartPosition == cursorAtWordStart.position();
}

void CppUseSelectionsUpdater::processResults(const CursorInfo &result)
{
    if (m_updateSelections) {
        ExtraSelections localVariableSelections;
        if (!result.useRanges.isEmpty() || !currentUseSelections().isEmpty()) {
            ExtraSelections selections = updateUseSelections(result.useRanges);
            if (result.areUseRangesForLocalVariable)
                localVariableSelections = selections;
        }
        updateUnusedSelections(result.unusedVariablesRanges);
        emit selectionsForVariableUnderCursorUpdated(localVariableSelections);
    }
    emit finished(result.localUses, true);
}

void CppUseSelectionsUpdater::onFindUsesFinished()
{
    QTC_ASSERT(m_runnerWatcher,
               emit finished(SemanticInfo::LocalUseMap(), false); return);

    if (m_runnerWatcher->isCanceled()) {
        emit finished(SemanticInfo::LocalUseMap(), false);
        return;
    }
    if (m_runnerRevision != m_editorWidget->document()->revision()) {
        emit finished(SemanticInfo::LocalUseMap(), false);
        return;
    }
    if (m_runnerWordStartPosition
            != Utils::Text::wordStartCursor(m_editorWidget->textCursor()).position()) {
        emit finished(SemanticInfo::LocalUseMap(), false);
        return;
    }

    processResults(m_runnerWatcher->result());

    m_runnerWatcher.release()->deleteLater();
}

CppUseSelectionsUpdater::ExtraSelections
CppUseSelectionsUpdater::toExtraSelections(const CursorInfo::Ranges &ranges,
                                           TextEditor::TextStyle style)
{
    CppUseSelectionsUpdater::ExtraSelections selections;
    selections.reserve(ranges.size());

    for (const CursorInfo::Range &range : ranges) {
        QTextDocument *document = m_editorWidget->document();
        const int position
                = document->findBlockByNumber(static_cast<int>(range.line) - 1).position()
                    + static_cast<int>(range.column) - 1;
        const int anchor = position + static_cast<int>(range.length);

        QTextEdit::ExtraSelection sel;
        sel.format = m_editorWidget->textDocument()->fontSettings().toTextCharFormat(style);
        sel.cursor = QTextCursor(document);
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        selections.append(sel);
    }

    return selections;
}

CppUseSelectionsUpdater::ExtraSelections
CppUseSelectionsUpdater::currentUseSelections() const
{
    return m_editorWidget->extraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection);
}

CppUseSelectionsUpdater::ExtraSelections
CppUseSelectionsUpdater::updateUseSelections(const CursorInfo::Ranges &ranges)
{
    const ExtraSelections selections = toExtraSelections(ranges, TextEditor::C_OCCURRENCES);
    m_editorWidget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection,
                                       selections);

    return selections;
}

void CppUseSelectionsUpdater::updateUnusedSelections(const CursorInfo::Ranges &ranges)
{
    const ExtraSelections selections = toExtraSelections(ranges, TextEditor::C_OCCURRENCES_UNUSED);
    m_editorWidget->setExtraSelections(TextEditor::TextEditorWidget::UnusedSymbolSelection,
                                       selections);
}

} // namespace Internal
} // namespace CppEditor
