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

#ifndef CPPUSESELECTIONSUPDATER_H
#define CPPUSESELECTIONSUPDATER_H

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

public slots:
    void scheduleUpdate();
    void abortSchedule();
    void update(CallType callType = Asynchronous);

signals:
    void finished(CppTools::SemanticInfo::LocalUseMap localUses);
    void selectionsForVariableUnderCursorUpdated(const QList<QTextEdit::ExtraSelection> &);

private slots:
    void onFindUsesFinished();

private:
    CppUseSelectionsUpdater();

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

#endif // CPPUSESELECTIONSUPDATER_H
