/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "refactoringchanges.h"

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtGui/QTextBlock>
#include <QtCore/QDebug>

using namespace TextEditor;

RefactoringChanges::RefactoringChanges()
{}

RefactoringChanges::~RefactoringChanges()
{
    if (!m_fileToOpen.isEmpty()) {
        BaseTextEditor *editor = editorForFile(m_fileToOpen, true);
        editor->gotoLine(m_lineToOpen, m_columnToOpen);

        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->activateEditor(editor->editableInterface(),
                                      Core::EditorManager::ModeSwitch);
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

    QFile file(fileName);
    if (!file.exists()) {
        if (!file.open(QIODevice::Append))
            return 0;
        file.close();
    }

    Core::IEditor *editor = editorManager->openEditor(fileName, QString(),
                                                      Core::EditorManager::NoActivate | Core::EditorManager::IgnoreNavigationHistory);
    return qobject_cast<BaseTextEditor *>(editor->widget());
}

QList<QTextCursor> RefactoringChanges::rangesToSelections(QTextDocument *document, const QList<Range> &ranges)
{
    QList<QTextCursor> selections;

    foreach (const Range &range, ranges) {
        QTextCursor selection(document);
        // ### workaround for moving the textcursor when inserting text at the beginning of the range.
        selection.setPosition(qMax(0, range.start - 1));
        selection.setPosition(qMin(range.end, document->characterCount() - 1), QTextCursor::KeepAnchor);

        selections.append(selection);
    }

    return selections;
}

bool RefactoringChanges::createFile(const QString &fileName, const QString &contents, bool reindent, bool openEditor)
{
    if (QFile::exists(fileName))
        return false;

    BaseTextEditor *editor = editorForFile(fileName, openEditor);

    QTextDocument *document;
    if (editor)
        document = editor->document();
    else
        document = new QTextDocument;

    {
        QTextCursor cursor(document);
        cursor.beginEditBlock();

        cursor.insertText(contents);

        if (reindent) {
            cursor.select(QTextCursor::Document);
            indentSelection(cursor);
        }

        cursor.endEditBlock();
    }

    if (!editor) {
        QFile file(fileName);
        file.open(QFile::WriteOnly);
        file.write(document->toPlainText().toUtf8());
        delete document;
    }

    fileChanged(fileName);

    return true;
}

bool RefactoringChanges::removeFile(const QString &fileName)
{
    if (!QFile::exists(fileName))
        return false;

    // ### implement!
    qWarning() << "RefactoringChanges::removeFile is not implemented";
    return true;
}

RefactoringFile RefactoringChanges::file(const QString &fileName)
{
    if (QFile::exists(fileName))
        return RefactoringFile(fileName, this);
    else
        return RefactoringFile();
}

BaseTextEditor *RefactoringChanges::openEditor(const QString &fileName, int pos)
{
    BaseTextEditor *editor = editorForFile(fileName, true);
    if (pos != -1) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(pos);
        editor->setTextCursor(cursor);
    }
    return editor;
}

void RefactoringChanges::activateEditor(const QString &fileName, int line, int column)
{
    m_fileToOpen = fileName;
    m_lineToOpen = line;
    m_columnToOpen = column;
}


RefactoringFile::RefactoringFile()
    : m_refactoringChanges(0)
    , m_document(0)
    , m_editor(0)
    , m_openEditor(false)
{ }

RefactoringFile::RefactoringFile(const QString &fileName, RefactoringChanges *refactoringChanges)
    : m_fileName(fileName)
    , m_refactoringChanges(refactoringChanges)
    , m_document(0)
    , m_editor(0)
    , m_openEditor(false)
{
    m_editor = RefactoringChanges::editorForFile(fileName, false);
}

RefactoringFile::RefactoringFile(const RefactoringFile &other)
    : m_fileName(other.m_fileName)
    , m_refactoringChanges(other.m_refactoringChanges)
    , m_document(0)
    , m_editor(other.m_editor)
{
    Q_ASSERT_X(!other.m_document && other.m_changes.isEmpty() && other.m_indentRanges.isEmpty(),
               "RefactoringFile", "A refactoring file with changes is not copyable");
}

RefactoringFile::~RefactoringFile()
{
    if (m_refactoringChanges && m_openEditor && !m_fileName.isEmpty())
        m_editor = m_refactoringChanges->openEditor(m_fileName, -1);

    // apply changes, if any
    if (m_refactoringChanges && !(m_indentRanges.isEmpty() && m_changes.isEmpty())) {
        QTextDocument *doc = mutableDocument();
        {
            QTextCursor c = cursor();
            c.beginEditBlock();

            // build indent selections now, applying the changeset will change locations
            const QList<QTextCursor> &indentSelections =
                    RefactoringChanges::rangesToSelections(
                            doc, m_indentRanges);

            // apply changes and reindent
            m_changes.apply(&c);
            foreach (const QTextCursor &selection, indentSelections) {
                m_refactoringChanges->indentSelection(selection);
            }

            c.endEditBlock();
        }

        // if this document doesn't have an editor, write the result to a file
        if (!m_editor && !m_fileName.isEmpty()) {
            const QByteArray &newContents = doc->toPlainText().toUtf8();
            QFile file(m_fileName);
            file.open(QFile::WriteOnly);
            file.write(newContents);
        }

        if (!m_fileName.isEmpty())
            m_refactoringChanges->fileChanged(m_fileName);
    }

    delete m_document;
}

bool RefactoringFile::isValid() const
{
    return !m_fileName.isEmpty();
}

const QTextDocument *RefactoringFile::document() const
{
    return mutableDocument();
}

QTextDocument *RefactoringFile::mutableDocument() const
{
    if (m_editor)
        return m_editor->document();
    else if (!m_document) {
        QString fileContents;
        if (!m_fileName.isEmpty()) {
            QFile file(m_fileName);
            if (file.open(QIODevice::ReadOnly))
                fileContents = file.readAll();
        }
        m_document = new QTextDocument(fileContents);
    }
    return m_document;
}

const QTextCursor RefactoringFile::cursor() const
{
    if (m_editor)
        return m_editor->textCursor();
    else if (!m_fileName.isEmpty())
        return QTextCursor(mutableDocument());

    return QTextCursor();
}

QString RefactoringFile::fileName() const
{
    return m_fileName;
}

int RefactoringFile::position(unsigned line, unsigned column) const
{
    Q_ASSERT(line != 0);
    Q_ASSERT(column != 0);
    if (const QTextDocument *doc = document())
        return doc->findBlockByNumber(line - 1).position() + column - 1;
    return -1;
}

QChar RefactoringFile::charAt(int pos) const
{
    if (const QTextDocument *doc = document())
        return doc->characterAt(pos);
    return QChar();
}

QString RefactoringFile::textOf(int start, int end) const
{
    QTextCursor c = cursor();
    c.setPosition(start);
    c.setPosition(end, QTextCursor::KeepAnchor);
    return c.selectedText();
}

QString RefactoringFile::textOf(const Range &range) const
{
    return textOf(range.start, range.end);
}

bool RefactoringFile::change(const Utils::ChangeSet &changeSet, bool openEditor)
{
    if (m_fileName.isEmpty())
        return false;
    if (!m_changes.isEmpty())
        return false;

    m_changes = changeSet;
    m_openEditor = openEditor;

    return true;
}

bool RefactoringFile::indent(const Range &range, bool openEditor)
{
    if (m_fileName.isEmpty())
        return false;

    m_indentRanges.append(range);

    if (openEditor)
        m_openEditor = true;

    return true;
}
