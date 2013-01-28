/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "refactoringchanges.h"
#include "basetexteditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QFile>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QDebug>

using namespace TextEditor;

RefactoringChanges::RefactoringChanges()
    : m_data(new RefactoringChangesData)
{}

RefactoringChanges::RefactoringChanges(RefactoringChangesData *data)
    : m_data(data)
{}

RefactoringChanges::~RefactoringChanges()
{}

BaseTextEditorWidget *RefactoringChanges::editorForFile(const QString &fileName)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();

    const QList<Core::IEditor *> editors = editorManager->editorsForFileName(fileName);
    foreach (Core::IEditor *editor, editors) {
        BaseTextEditorWidget *textEditor = qobject_cast<BaseTextEditorWidget *>(editor->widget());
        if (textEditor != 0)
            return textEditor;
    }
    return 0;
}

QList<QPair<QTextCursor, QTextCursor > > RefactoringChanges::rangesToSelections(QTextDocument *document,
                                                                                const QList<Range> &ranges)
{
    QList<QPair<QTextCursor, QTextCursor> > selections;

    foreach (const Range &range, ranges) {
        QTextCursor start(document);
        start.setPosition(range.start);
        start.setKeepPositionOnInsert(true);
        QTextCursor end(document);
        end.setPosition(qMin(range.end, document->characterCount() - 1));

        selections.append(qMakePair(start, end));
    }

    return selections;
}

bool RefactoringChanges::createFile(const QString &fileName, const QString &contents, bool reindent, bool openEditor) const
{
    if (QFile::exists(fileName))
        return false;

    // Create a text document for the new file:
    QTextDocument *document = new QTextDocument;
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    cursor.insertText(contents);

    // Reindent the contents:
    if (reindent) {
        cursor.select(QTextCursor::Document);
        m_data->indentSelection(cursor, fileName, 0);
    }
    cursor.endEditBlock();

    // Write the file to disk:
    Utils::TextFileFormat format;
    format.codec = Core::EditorManager::instance()->defaultTextCodec();
    QString error;
    bool saveOk = format.writeFile(fileName, document->toPlainText(), &error);
    delete document;
    if (!saveOk)
        return false;

    m_data->fileChanged(fileName);

    if (openEditor)
        this->openEditor(fileName, /*bool activate =*/ false, -1, -1);

    return true;
}

bool RefactoringChanges::removeFile(const QString &fileName) const
{
    if (!QFile::exists(fileName))
        return false;

    // ### implement!
    qWarning() << "RefactoringChanges::removeFile is not implemented";
    return true;
}

BaseTextEditorWidget *RefactoringChanges::openEditor(const QString &fileName, bool activate, int line, int column)
{
    Core::EditorManager::OpenEditorFlags flags = Core::EditorManager::IgnoreNavigationHistory;
    if (!activate)
        flags |= Core::EditorManager::NoActivate;
    if (line != -1) {
        // openEditorAt uses a 1-based line and a 0-based column!
        column -= 1;
    }
    Core::IEditor *editor = BaseTextEditorWidget::openEditorAt(
                fileName, line, column, Core::Id(), flags);

    if (editor)
        return qobject_cast<BaseTextEditorWidget *>(editor->widget());
    else
        return 0;
}

RefactoringFilePtr RefactoringChanges::file(BaseTextEditorWidget *editor)
{
    return RefactoringFilePtr(new RefactoringFile(editor));
}

RefactoringFilePtr RefactoringChanges::file(const QString &fileName) const
{
    return RefactoringFilePtr(new RefactoringFile(fileName, m_data));
}

RefactoringFile::RefactoringFile(QTextDocument *document, const QString &fileName)
    : m_fileName(fileName)
    , m_document(document)
    , m_editor(0)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
    , m_appliedOnce(false)
{ }

RefactoringFile::RefactoringFile(BaseTextEditorWidget *editor)
    : m_fileName(editor->editorDocument()->fileName())
    , m_document(0)
    , m_editor(editor)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
    , m_appliedOnce(false)
{ }

RefactoringFile::RefactoringFile(const QString &fileName, const QSharedPointer<RefactoringChangesData> &data)
    : m_fileName(fileName)
    , m_data(data)
    , m_document(0)
    , m_editor(0)
    , m_openEditor(false)
    , m_activateEditor(false)
    , m_editorCursorPosition(-1)
    , m_appliedOnce(false)
{
    m_editor = RefactoringChanges::editorForFile(fileName);
}

RefactoringFile::~RefactoringFile()
{
    delete m_document;
}

bool RefactoringFile::isValid() const
{
    if (m_fileName.isEmpty())
        return false;
    return document();
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
            QString error;
            QTextCodec *defaultCodec = Core::EditorManager::instance()->defaultTextCodec();
            Utils::TextFileFormat::ReadResult result = Utils::TextFileFormat::readFile(
                        m_fileName, defaultCodec,
                        &fileContents, &m_textFileFormat,
                        &error);
            if (result != Utils::TextFileFormat::ReadSuccess) {
                qWarning() << "Could not read " << m_fileName << ". Error: " << error;
                m_textFileFormat.codec = 0;
            }
        }
        // always make a QTextDocument to avoid excessive null checks
        m_document = new QTextDocument(fileContents);
    }
    return m_document;
}

const QTextCursor RefactoringFile::cursor() const
{
    if (m_editor)
        return m_editor->textCursor();
    else if (!m_fileName.isEmpty()) {
        if (QTextDocument *doc = mutableDocument())
            return QTextCursor(doc);
    }

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

void RefactoringFile::lineAndColumn(int offset, unsigned *line, unsigned *column) const
{
    Q_ASSERT(line);
    Q_ASSERT(column);
    Q_ASSERT(offset >= 0);
    QTextCursor c(cursor());
    c.setPosition(offset);
    *line = c.blockNumber() + 1;
    *column = c.positionInBlock() + 1;
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

void RefactoringFile::setChangeSet(const Utils::ChangeSet &changeSet)
{
    if (m_fileName.isEmpty())
        return;

    m_changes = changeSet;
}

void RefactoringFile::appendIndentRange(const Range &range)
{
    if (m_fileName.isEmpty())
        return;

    m_indentRanges.append(range);
}

void RefactoringFile::appendReindentRange(const Range &range)
{
    if (m_fileName.isEmpty())
        return;

    m_reindentRanges.append(range);
}

void RefactoringFile::setOpenEditor(bool activate, int pos)
{
    m_openEditor = true;
    m_activateEditor = activate;
    m_editorCursorPosition = pos;
}

void RefactoringFile::apply()
{
    // open / activate / goto position
    if (m_openEditor && !m_fileName.isEmpty()) {
        unsigned line = unsigned(-1), column = unsigned(-1);
        if (m_editorCursorPosition != -1)
            lineAndColumn(m_editorCursorPosition, &line, &column);
        m_editor = RefactoringChanges::openEditor(m_fileName, m_activateEditor, line, column);
        m_openEditor = false;
        m_activateEditor = false;
        m_editorCursorPosition = -1;
    }

    // apply changes, if any
    if (m_data && !(m_indentRanges.isEmpty() && m_changes.isEmpty())) {
        QTextDocument *doc = mutableDocument();
        if (doc) {
            QTextCursor c = cursor();
            if (m_appliedOnce)
                c.joinPreviousEditBlock();
            else
                c.beginEditBlock();

            // build indent selections now, applying the changeset will change locations
            const QList<QPair<QTextCursor, QTextCursor> > &indentSelections =
                    RefactoringChanges::rangesToSelections(doc, m_indentRanges);
            m_indentRanges.clear();
            const QList<QPair<QTextCursor, QTextCursor> > &reindentSelections =
                    RefactoringChanges::rangesToSelections(doc, m_reindentRanges);
            m_reindentRanges.clear();

            // apply changes and reindent
            m_changes.apply(&c);
            m_changes.clear();

            indentOrReindent(&RefactoringChangesData::indentSelection, indentSelections);
            indentOrReindent(&RefactoringChangesData::reindentSelection, reindentSelections);

            c.endEditBlock();

            // if this document doesn't have an editor, write the result to a file
            if (!m_editor && m_textFileFormat.codec) {
                QTC_ASSERT(!m_fileName.isEmpty(), return);
                QString error;
                if (!m_textFileFormat.writeFile(m_fileName, doc->toPlainText(), &error))
                    qWarning() << "Could not apply changes to" << m_fileName << ". Error: " << error;
            }

            fileChanged();
        }
    }

    m_appliedOnce = true;
}

void RefactoringFile::indentOrReindent(void (RefactoringChangesData::*mf)(const QTextCursor &,
                                                                          const QString &,
                                                                          const BaseTextEditorWidget *) const,
                                       const QList<QPair<QTextCursor, QTextCursor> > &ranges)
{
    QPair<QTextCursor, QTextCursor> p;
    foreach (p, ranges) {
        QTextCursor selection(p.first.document());
        selection.setPosition(p.first.position());
        selection.setPosition(p.second.position(), QTextCursor::KeepAnchor);
        ((*m_data).*(mf))(selection, m_fileName, m_editor);
    }
}

void RefactoringFile::fileChanged()
{
    if (!m_fileName.isEmpty())
        m_data->fileChanged(m_fileName);
}

RefactoringChangesData::~RefactoringChangesData()
{}

void RefactoringChangesData::indentSelection(const QTextCursor &, const QString &, const BaseTextEditorWidget *) const
{
    qWarning() << Q_FUNC_INFO << "not implemented";
}

void RefactoringChangesData::reindentSelection(const QTextCursor &, const QString &, const BaseTextEditorWidget *) const
{
    qWarning() << Q_FUNC_INFO << "not implemented";
}

void RefactoringChangesData::fileChanged(const QString &)
{
}
