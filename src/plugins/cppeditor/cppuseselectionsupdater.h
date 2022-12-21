// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcursorinfo.h"
#include "cppsemanticinfo.h"

#include <QFutureWatcher>
#include <QTextEdit>
#include <QTimer>

namespace CppEditor {
class CppEditorWidget;

namespace Internal {

class CppUseSelectionsUpdater : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CppUseSelectionsUpdater)

public:
    explicit CppUseSelectionsUpdater(CppEditorWidget *editorWidget);
    ~CppUseSelectionsUpdater() override;

    void scheduleUpdate();
    void abortSchedule();

    enum class CallType { Synchronous, Asynchronous };
    enum class RunnerInfo { AlreadyUpToDate, Started, FailedToStart, Invalid }; // For async case.
    RunnerInfo update(CallType callType = CallType::Asynchronous);

signals:
    void finished(SemanticInfo::LocalUseMap localUses, bool success);
    void selectionsForVariableUnderCursorUpdated(const QList<QTextEdit::ExtraSelection> &);

private:
    CppUseSelectionsUpdater();
    bool isSameIdentifierAsBefore(const QTextCursor &cursorAtWordStart) const;
    void processResults(const CursorInfo &result);
    void onFindUsesFinished();

    // Convenience
    using ExtraSelections = QList<QTextEdit::ExtraSelection>;
    ExtraSelections toExtraSelections(const CursorInfo::Ranges &ranges,
                                      TextEditor::TextStyle style);
    ExtraSelections currentUseSelections() const;
    ExtraSelections updateUseSelections(const CursorInfo::Ranges &selections);
    void updateUnusedSelections(const CursorInfo::Ranges &selections);

private:
    CppEditorWidget * const m_editorWidget;

    QTimer m_timer;

    QScopedPointer<QFutureWatcher<CursorInfo>> m_runnerWatcher;
    int m_runnerRevision = -1;
    int m_runnerWordStartPosition = -1;
    bool m_updateSelections = true;
};

} // namespace Internal
} // namespace CppEditor
