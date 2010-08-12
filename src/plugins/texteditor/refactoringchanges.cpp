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
#include <QtCore/QDebug>

using namespace TextEditor;

RefactoringChanges::RefactoringChanges()
{}

RefactoringChanges::~RefactoringChanges()
{}

QStringList RefactoringChanges::apply()
{
    QSet<QString> changed;

    { // open editors
        QHashIterator<QString, int> it(m_cursorByFile);
        while (it.hasNext()) {
            it.next();
            const QString &fileName = it.key();
            int pos = it.value();

            BaseTextEditor *editor = editorForFile(fileName, true);
            if (pos != -1) {
                QTextCursor cursor = editor->textCursor();
                cursor.setPosition(pos);
                editor->setTextCursor(cursor);
            }

            if (m_fileNameToShow == fileName) {
                Core::EditorManager *editorManager = Core::EditorManager::instance();
                editorManager->activateEditor(editor->editableInterface());
            }
        }
    }

    { // change and indent files
        QHashIterator<QString, Utils::ChangeSet> it(m_changesByFile);
        while (it.hasNext()) {
            it.next();
            const QString &fileName = it.key();
            Utils::ChangeSet changes = it.value();

            QTextDocument *document;
            QScopedPointer<QFile> file;
            QTextCursor cursor;
            {
                if (BaseTextEditor *editor = editorForFile(fileName, false)) {
                    document = editor->document();
                    cursor = editor->textCursor();
                } else {
                    file.reset(new QFile(fileName));
                    if (!file->open(QIODevice::ReadWrite)) {
                        qWarning() << "RefactoringChanges could not open" << fileName << "for read/write, skipping";
                        continue;
                    }
                    document = new QTextDocument(file->readAll());
                    cursor = QTextCursor(document);
                }
            }

            {
                cursor.beginEditBlock();

                // build indent selections now, applying the changeset will change locations
                const QList<QTextCursor> &indentSelections = rangesToSelections(
                        document, m_indentRangesByFile.values(fileName));

                // apply changes and reindent
                changes.apply(&cursor);

                // newly created files must be indented specially - there's no way to select everything in an empty file
                if (m_filesToCreate.contains(fileName) && m_indentRangesByFile.contains(fileName)) {
                    QTextCursor completeSelection(cursor);
                    completeSelection.select(QTextCursor::Document);
                    indentSelection(completeSelection);
                } else {
                    foreach (const QTextCursor &selection, indentSelections)
                        indentSelection(selection);
                }

                cursor.endEditBlock();
            }

            // if this document came from a file, write the result to it
            if (file) {
                const QByteArray &newContents = document->toPlainText().toUtf8();
                file->resize(newContents.size());
                file->seek(0);
                file->write(newContents);
                delete document;
            }

            changed.insert(fileName);
        }
    }

    { // Delete files
        // ###
    }

    return changed.toList();
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
                                                      Core::EditorManager::NoActivate | Core::EditorManager::IgnoreNavigationHistory | Core::EditorManager::NoModeSwitch);
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
    if (m_changesByFile.contains(fileName))
        return false;

    m_filesToCreate.insert(fileName);

    Utils::ChangeSet changes;
    changes.insert(0, contents);
    m_changesByFile.insert(fileName, changes);

    if (reindent)
        m_indentRangesByFile.insert(fileName, Range(0, 0));
    if (openEditor)
        m_cursorByFile.insert(fileName, -1);

    return true;
}

bool RefactoringChanges::removeFile(const QString &fileName)
{
    if (!QFile::exists(fileName))
        return false;

    // ### delete!
    return true;
}

RefactoringFile RefactoringChanges::file(const QString &fileName)
{
    if (QFile::exists(fileName))
        return RefactoringFile(fileName, this);
    else
        return RefactoringFile();
}

void RefactoringChanges::openEditor(const QString &fileName, int pos)
{
    m_cursorByFile.insert(fileName, pos);
}

void RefactoringChanges::setActiveEditor(const QString &fileName, int pos)
{
    m_cursorByFile.insert(fileName, pos);
    m_fileNameToShow = fileName;
}

void RefactoringChanges::changeFile(const QString &fileName, const Utils::ChangeSet &changes, bool openEditor)
{
    m_changesByFile.insert(fileName, changes);

    if (openEditor)
        m_cursorByFile.insert(fileName, -1);
}

void RefactoringChanges::reindent(const QString &fileName, const Range &range, bool openEditor)
{
    m_indentRangesByFile.insert(fileName, range);

    // simplify apply by never having files indented that are not also changed
    if (!m_changesByFile.contains(fileName))
        m_changesByFile.insert(fileName, Utils::ChangeSet());
    if (openEditor)
        m_cursorByFile.insert(fileName, -1);
}


RefactoringFile::RefactoringFile()
    : m_refactoringChanges(0)
    , m_document(0)
    , m_editor(0)
{ }

RefactoringFile::RefactoringFile(const QString &fileName, RefactoringChanges *refactoringChanges)
    : m_fileName(fileName)
    , m_refactoringChanges(refactoringChanges)
    , m_document(0)
    , m_editor(0)
{
    m_editor = RefactoringChanges::editorForFile(fileName, false);
}

RefactoringFile::RefactoringFile(const RefactoringFile &other)
    : m_fileName(other.m_fileName)
    , m_refactoringChanges(other.m_refactoringChanges)
    , m_document(0)
    , m_editor(other.m_editor)
{
    if (other.m_document)
        m_document = new QTextDocument(other.m_document);
}

RefactoringFile::~RefactoringFile()
{
    delete m_document;
}

bool RefactoringFile::isValid() const
{
    return !m_fileName.isEmpty();
}

const QTextDocument *RefactoringFile::document() const
{
    if (m_editor)
        return m_editor->document();

    return mutableDocument();
}

QTextDocument *RefactoringFile::mutableDocument() const
{
    if (!m_document && !m_fileName.isEmpty()) {
        QString fileContents;
        {
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

QString RefactoringFile::textAt(int start, int end) const
{
    QTextCursor c = cursor();
    c.setPosition(start);
    c.setPosition(end, QTextCursor::KeepAnchor);
    return c.selectedText();
}

QString RefactoringFile::textAt(const Range &range) const
{
    return textAt(range.start, range.end);
}

bool RefactoringFile::change(const Utils::ChangeSet &changeSet, bool openEditor)
{
    if (m_fileName.isEmpty())
        return false;
    if (!m_refactoringChanges->m_changesByFile.value(m_fileName).isEmpty())
        return false;

    m_refactoringChanges->m_changesByFile.insert(m_fileName, changeSet);

    if (openEditor)
        m_refactoringChanges->m_cursorByFile.insert(m_fileName, -1);

    return true;
}

bool RefactoringFile::indent(const Range &range, bool openEditor)
{
    if (m_fileName.isEmpty())
        return false;

    m_refactoringChanges->m_indentRangesByFile.insert(m_fileName, range);

    // simplify apply by never having files indented that are not also changed
    if (!m_refactoringChanges->m_changesByFile.contains(m_fileName))
        m_refactoringChanges->m_changesByFile.insert(m_fileName, Utils::ChangeSet());

    if (openEditor)
        m_refactoringChanges->m_cursorByFile.insert(m_fileName, -1);

    return true;
}
