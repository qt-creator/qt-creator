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

#pragma once

#include <cpptools/cppcursorinfo.h>
#include <cpptools/cppsemanticinfo.h>

#include <QFutureWatcher>
#include <QTextEdit>
#include <QTimer>

namespace TextEditor { class TextEditorWidget; }

namespace CppEditor {
namespace Internal {

class CppUseSelectionsUpdater : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CppUseSelectionsUpdater)

public:
    explicit CppUseSelectionsUpdater(TextEditor::TextEditorWidget *editorWidget);
    ~CppUseSelectionsUpdater();

    void scheduleUpdate();
    void abortSchedule();

    enum class CallType { Synchronous, Asynchronous };
    enum class RunnerInfo { AlreadyUpToDate, Started, FailedToStart, Invalid }; // For async case.
    RunnerInfo update(CallType callType = CallType::Asynchronous);

signals:
    void finished(CppTools::SemanticInfo::LocalUseMap localUses, bool success);
    void selectionsForVariableUnderCursorUpdated(const QList<QTextEdit::ExtraSelection> &);

private:
    CppUseSelectionsUpdater();
    bool isSameIdentifierAsBefore(const QTextCursor &cursorAtWordStart) const;
    void processResults(const CppTools::CursorInfo &result);
    void onFindUsesFinished();

    // Convenience
    using ExtraSelections = QList<QTextEdit::ExtraSelection>;
    using CursorInfo = CppTools::CursorInfo;
    ExtraSelections toExtraSelections(const CursorInfo::Ranges &ranges,
                                      TextEditor::TextStyle style);
    ExtraSelections currentUseSelections() const;
    ExtraSelections updateUseSelections(const CursorInfo::Ranges &selections);
    void updateUnusedSelections(const CursorInfo::Ranges &selections);

private:
    TextEditor::TextEditorWidget *m_editorWidget;

    QTimer m_timer;

    QScopedPointer<QFutureWatcher<CppTools::CursorInfo>> m_runnerWatcher;
    int m_runnerRevision = -1;
    int m_runnerWordStartPosition = -1;
};

} // namespace Internal
} // namespace CppEditor
