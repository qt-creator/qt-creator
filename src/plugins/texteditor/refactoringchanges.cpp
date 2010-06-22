/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "refactoringchanges.h"

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtGui/QTextBlock>

using namespace TextEditor;

RefactoringChanges::~RefactoringChanges()
{}

void RefactoringChanges::createFile(const QString &fileName, const QString &contents)
{
    m_contentsByCreatedFile.insert(fileName, contents);
}

void RefactoringChanges::changeFile(const QString &fileName, const Utils::ChangeSet &changeSet)
{
    m_changesByFile.insert(fileName, changeSet);
}

void RefactoringChanges::reindent(const QString &fileName, const Range &range)
{
    m_indentRangesByFile[fileName].append(range);
}

QStringList RefactoringChanges::apply()
{
    QSet<QString> changed;

    { // create files
        foreach (const QString &fileName, m_contentsByCreatedFile.keys()) {
            BaseTextEditor *editor = editorForNewFile(fileName);
            if (editor == 0)
                continue;

            QTextCursor tc = editor->textCursor();
            tc.beginEditBlock();
            tc.setPosition(0);
            tc.insertText(m_contentsByCreatedFile.value(fileName));

            foreach (const Range &range, m_indentRangesByFile.value(fileName, QList<Range>())) {
                QTextCursor indentCursor = editor->textCursor();
                indentCursor.setPosition(range.start);
                indentCursor.setPosition(range.end, QTextCursor::KeepAnchor);
                editor->indentInsertedText(indentCursor);
            }

            tc.endEditBlock();
            changed.insert(fileName);
        }
    }

    { // change and indent files
        foreach (const QString &fileName, m_changesByFile.keys()) {
            BaseTextEditor *editor = editorForFile(fileName, true);
            if (editor == 0)
                continue;

            QTextCursor tc = editor->textCursor();
            tc.beginEditBlock();

            typedef QPair<QTextCursor, QTextCursor> CursorPair;
            QList<CursorPair> cursorPairs;
            foreach (const Range &range, m_indentRangesByFile.value(fileName, QList<Range>())) {
                QTextCursor start = editor->textCursor();
                QTextCursor end = editor->textCursor();
                start.setPosition(range.start);
                end.setPosition(range.end);
                cursorPairs.append(qMakePair(start, end));
            }

            QTextCursor changeSetCursor = editor->textCursor();
            Utils::ChangeSet changeSet = m_changesByFile.value(fileName);
            changeSet.apply(&changeSetCursor);

            foreach (const CursorPair &cursorPair, cursorPairs) {
                QTextCursor indentCursor = cursorPair.first;
                indentCursor.setPosition(cursorPair.second.position(),
                                         QTextCursor::KeepAnchor);
                editor->indentInsertedText(indentCursor);
            }

            tc.endEditBlock();
            changed.insert(fileName);
        }
    }

    { // Indent files which are not changed
        foreach (const QString &fileName, m_indentRangesByFile.keys()) {
            BaseTextEditor *editor = editorForFile(fileName);

            if (editor == 0)
                continue;

            if (changed.contains(fileName))
                continue;

            QTextCursor tc = editor->textCursor();
            tc.beginEditBlock();

            foreach (const Range &range, m_indentRangesByFile.value(fileName, QList<Range>())) {
                QTextCursor indentCursor = editor->textCursor();
                indentCursor.setPosition(range.start);
                indentCursor.setPosition(range.end, QTextCursor::KeepAnchor);
                editor->indentInsertedText(indentCursor);
            }

            tc.endEditBlock();
            changed.insert(fileName);
        }
    }

    { // Delete files
        // ###
    }

    return changed.toList();
}

int RefactoringChanges::positionInFile(const QString &fileName, int line, int column) const
{
    if (BaseTextEditor *editor = editorForFile(fileName)) {
        return editor->document()->findBlockByNumber(line).position() + column;
    } else {
        return -1;
    }
}

BaseTextEditor *RefactoringChanges::editorForFile(const QString &fileName,
                                                  bool openIfClosed)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();

    const QList<Core::IEditor *> editors = editorManager->editorsForFileName(fileName);
    foreach (Core::IEditor *editor, editors) {
        BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(editor->widget());
        if (textEditor != 0)
            return textEditor;
    }

    if (!openIfClosed)
        return 0;

    Core::IEditor *editor = editorManager->openEditor(fileName, QString(),
                                                      Core::EditorManager::NoActivate | Core::EditorManager::IgnoreNavigationHistory | Core::EditorManager::NoModeSwitch);
    return qobject_cast<BaseTextEditor *>(editor->widget());
}

BaseTextEditor *RefactoringChanges::editorForNewFile(const QString &fileName)
{
    QFile f(fileName);
    if (f.exists())
        return 0;
    if (!f.open(QIODevice::Append))
        return 0;
    f.close();
    return editorForFile(fileName, true);
}
