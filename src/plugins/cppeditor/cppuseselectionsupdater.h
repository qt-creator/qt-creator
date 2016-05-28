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

#include <cpptools/cppsemanticinfo.h>
#include <texteditor/texteditorconstants.h>

#include <cplusplus/CppDocument.h>

#include <QFutureWatcher>
#include <QTextEdit>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTextCharFormat;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }

namespace CppEditor {
namespace Internal {

typedef QList<QTextEdit::ExtraSelection> ExtraSelections;
typedef QList<CppTools::SemanticInfo::Use> SemanticUses;

struct UseSelectionsResult
{
    CppTools::SemanticInfo::LocalUseMap localUses;
    SemanticUses selectionsForLocalVariableUnderCursor;
    SemanticUses selectionsForLocalUnusedVariables;
    QList<int> references;
};

class CppUseSelectionsUpdater : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CppUseSelectionsUpdater)

public:
    explicit CppUseSelectionsUpdater(TextEditor::TextEditorWidget *editorWidget);

    enum CallType { Synchronous, Asynchronous };

    void scheduleUpdate();
    void abortSchedule();
    void update(CallType callType = Asynchronous);

signals:
    void finished(CppTools::SemanticInfo::LocalUseMap localUses);
    void selectionsForVariableUnderCursorUpdated(const QList<QTextEdit::ExtraSelection> &);

private:
    CppUseSelectionsUpdater();

    void onFindUsesFinished();
    bool handleMacroCase(const CPlusPlus::Document::Ptr document);
    void handleSymbolCaseAsynchronously(const CPlusPlus::Document::Ptr document,
                                        const CPlusPlus::Snapshot &snapshot);
    void handleSymbolCaseSynchronously(const CPlusPlus::Document::Ptr document,
                                       const CPlusPlus::Snapshot &snapshot);

    void processSymbolCaseResults(const UseSelectionsResult &result);

    ExtraSelections toExtraSelections(const SemanticUses &uses, TextEditor::TextStyle style) const;
    ExtraSelections toExtraSelections(const QList<int> &references,
                                      TextEditor::TextStyle style) const;

    // Convenience
    ExtraSelections currentUseSelections() const;
    void updateUseSelections(const ExtraSelections &selections);
    void updateUnusedSelections(const ExtraSelections &selections);
    QTextCharFormat textCharFormat(TextEditor::TextStyle category) const;
    QTextDocument *textDocument() const;

private:
    TextEditor::TextEditorWidget *m_editorWidget;

    QTimer m_timer;

    CPlusPlus::Document::Ptr m_document;

    QScopedPointer<QFutureWatcher<UseSelectionsResult>> m_findUsesWatcher;
    int m_findUsesRevision;
    int m_findUsesCursorPosition;
};

} // namespace Internal
} // namespace CppEditor
