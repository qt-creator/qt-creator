/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

namespace TextEditor { class BaseTextEditorWidget; }

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
    explicit CppUseSelectionsUpdater(TextEditor::BaseTextEditorWidget *editorWidget);

public slots:
    void scheduleUpdate();
    void abortSchedule();
    void update();

signals:
    void finished(CppTools::SemanticInfo::LocalUseMap localUses);
    void selectionsForVariableUnderCursorUpdated(const QList<QTextEdit::ExtraSelection> &);

private slots:
    void onFindUsesFinished();

private:
    CppUseSelectionsUpdater();

    bool handleMacroCase(const QTextCursor &textCursor,
                         const CPlusPlus::Document::Ptr document);
    void handleSymbolCase(const QTextCursor &textCursor,
                          const CPlusPlus::Document::Ptr document,
                          const CPlusPlus::Snapshot &snapshot);

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
    TextEditor::BaseTextEditorWidget *m_editorWidget;

    QTimer m_timer;

    CPlusPlus::Document::Ptr m_document;
    CPlusPlus::Snapshot m_snapshot;

    QScopedPointer<QFutureWatcher<UseSelectionsResult>> m_findUsesWatcher;
    int m_findUsesRevision;
    int m_findUsesCursorPosition;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPUSESELECTIONSUPDATER_H
