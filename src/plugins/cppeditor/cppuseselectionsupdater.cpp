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

#include "cppuseselectionsupdater.h"

#include "cppeditorwidget.h"
#include "cppeditordocument.h"

#include <cpptools/cpptoolsreuse.h>
#include <utils/textutils.h>

#include <QTextBlock>
#include <QTextCursor>

#include <utils/qtcassert.h>

enum { updateUseSelectionsInternalInMs = 500 };

namespace CppEditor {
namespace Internal {

CppUseSelectionsUpdater::CppUseSelectionsUpdater(TextEditor::TextEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
    , m_runnerRevision(-1)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(updateUseSelectionsInternalInMs);
    connect(&m_timer, &QTimer::timeout, this, [this]() { update(); });
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

    CppTools::CursorInfoParams params;
    params.semanticInfo = cppEditorWidget->semanticInfo();
    params.textCursor = Utils::Text::wordStartCursor(cppEditorWidget->textCursor());

    if (callType == CallType::Asynchronous) {
        if (isSameIdentifierAsBefore(params.textCursor))
            return RunnerInfo::AlreadyUpToDate;

        if (m_runnerWatcher)
            m_runnerWatcher->cancel();

        m_runnerWatcher.reset(new QFutureWatcher<CursorInfo>);
        connect(m_runnerWatcher.data(), &QFutureWatcherBase::finished,
                this, &CppUseSelectionsUpdater::onFindUsesFinished);

        m_runnerRevision = m_editorWidget->document()->revision();
        m_runnerWordStartPosition = params.textCursor.position();

        m_runnerWatcher->setFuture(cppEditorDocument->cursorInfo(params));
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
    ExtraSelections localVariableSelections;
    if (!result.useRanges.isEmpty() || !currentUseSelections().isEmpty()) {
        ExtraSelections selections = updateUseSelections(result.useRanges);
        if (result.areUseRangesForLocalVariable)
            localVariableSelections = selections;
    }

    updateUnusedSelections(result.unusedVariablesRanges);

    emit selectionsForVariableUnderCursorUpdated(localVariableSelections);
    emit finished(result.localUses, true);
}

void CppUseSelectionsUpdater::onFindUsesFinished()
{
    QTC_ASSERT(m_runnerWatcher,
               emit finished(CppTools::SemanticInfo::LocalUseMap(), false); return);

    if (m_runnerWatcher->isCanceled()) {
        emit finished(CppTools::SemanticInfo::LocalUseMap(), false);
        return;
    }
    if (m_runnerRevision != m_editorWidget->document()->revision()) {
        emit finished(CppTools::SemanticInfo::LocalUseMap(), false);
        return;
    }
    if (m_runnerWordStartPosition
            != Utils::Text::wordStartCursor(m_editorWidget->textCursor()).position()) {
        emit finished(CppTools::SemanticInfo::LocalUseMap(), false);
        return;
    }

    processResults(m_runnerWatcher->result());

    m_runnerWatcher.reset();
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
