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

#include "basetextdocument.h"
#include "basetexteditor.h"
#include "storagesettings.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtGui/QMainWindow>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QApplication>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/reloadpromptutils.h>

using namespace TextEditor;

DocumentMarker::DocumentMarker(QTextDocument *doc)
  : ITextMarkable(doc), document(doc)
{
}

BaseTextDocument::BaseTextDocument()
  : m_document(new QTextDocument(this)),
    m_highlighter(0)
{
    m_documentMarker = new DocumentMarker(m_document);
    m_lineTerminatorMode = NativeLineTerminator;
    m_fileIsReadOnly = false;
    m_isBinaryData = false;
    m_codec = QTextCodec::codecForLocale();
    m_hasDecodingError = false;
}

BaseTextDocument::~BaseTextDocument()
{
    QTextBlock block = m_document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData()))
            data->documentClosing();
        block = block.next();
    }
    delete m_document;
    m_document = 0;
}

QString BaseTextDocument::mimeType() const
{
    return m_mimeType;
}

void BaseTextDocument::setMimeType(const QString &mt)
{
    m_mimeType = mt;
}

bool BaseTextDocument::save(const QString &fileName)
{
    QTextCursor cursor(m_document);

    cursor.beginEditBlock();
    if (m_storageSettings.m_cleanWhitespace)
        cleanWhitespace(cursor, m_storageSettings.m_cleanIndentation, m_storageSettings.m_inEntireDocument);
    if (m_storageSettings.m_addFinalNewLine)
        ensureFinalNewLine(cursor);
    cursor.endEditBlock();

    QString fName = m_fileName;
    if (!fileName.isEmpty())
        fName = fileName;

    QFile file(fName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QString plainText = m_document->toPlainText();

    if (m_lineTerminatorMode == CRLFLineTerminator)
        plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

    file.write(m_codec->fromUnicode(plainText));
    if (!file.flush())
        return false;
    file.close();

    const QFileInfo fi(fName);
    m_fileName = QDir::cleanPath(fi.absoluteFilePath());

    m_document->setModified(false);
    emit titleChanged(fi.fileName());
    emit changed();

    m_isBinaryData = false;
    m_hasDecodingError = false;
    m_decodingErrorSample.clear();

    return true;
}

bool BaseTextDocument::isReadOnly() const
{
    if (m_isBinaryData || m_hasDecodingError)
        return true;
    if (m_fileName.isEmpty()) //have no corresponding file, so editing is ok
        return false;
    return m_fileIsReadOnly;
}

bool BaseTextDocument::isModified() const
{
    return m_document->isModified();
}

void BaseTextDocument::checkPermissions()
{
    if (!m_fileName.isEmpty()) {
        const QFileInfo fi(m_fileName);
        m_fileIsReadOnly = !fi.isWritable();
    } else {
        m_fileIsReadOnly = false;
    }
}

bool BaseTextDocument::open(const QString &fileName)
{
    QString title = tr("untitled");
    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        m_fileIsReadOnly = !fi.isWritable();
        m_fileName = QDir::cleanPath(fi.absoluteFilePath());

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        title = fi.fileName();

        QByteArray buf = file.readAll();
        int bytesRead = buf.size();

        QTextCodec *codec = m_codec;

        // code taken from qtextstream
        if (bytesRead >= 4 && ((uchar(buf[0]) == 0xff && uchar(buf[1]) == 0xfe && uchar(buf[2]) == 0 && uchar(buf[3]) == 0)
                               || (uchar(buf[0]) == 0 && uchar(buf[1]) == 0 && uchar(buf[2]) == 0xfe && uchar(buf[3]) == 0xff))) {
            codec = QTextCodec::codecForName("UTF-32");
        } else if (bytesRead >= 2 && ((uchar(buf[0]) == 0xff && uchar(buf[1]) == 0xfe)
                                      || (uchar(buf[0]) == 0xfe && uchar(buf[1]) == 0xff))) {
            codec = QTextCodec::codecForName("UTF-16");
        } else if (!codec) {
            codec = QTextCodec::codecForLocale();
        }
        // end code taken from qtextstream

        m_codec = codec;

#if 0 // should work, but does not, Qt bug with "system" codec
        QTextDecoder *decoder = m_codec->makeDecoder();
        QString text = decoder->toUnicode(buf);
        m_hasDecodingError = (decoder->hasFailure());
        delete decoder;
#else
        QString text = m_codec->toUnicode(buf);
        QByteArray verifyBuf = m_codec->fromUnicode(text); // slow
        // the minSize trick lets us ignore unicode headers
        int minSize = qMin(verifyBuf.size(), buf.size());
        m_hasDecodingError = (minSize < buf.size()- 4
                              || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                                        buf.constData() + buf.size() - minSize, minSize));
#endif

        if (m_hasDecodingError) {
            int p = buf.indexOf('\n', 16384);
            if (p < 0)
                m_decodingErrorSample = buf;
            else
                m_decodingErrorSample = buf.left(p);
        } else {
            m_decodingErrorSample.clear();
        }

        int lf = text.indexOf('\n');
        if (lf > 0 && text.at(lf-1) == QLatin1Char('\r')) {
            m_lineTerminatorMode = CRLFLineTerminator;
        } else if (lf >= 0) {
            m_lineTerminatorMode = LFLineTerminator;
        } else {
            m_lineTerminatorMode = NativeLineTerminator;
        }

        m_document->setModified(false);
        if (m_isBinaryData)
            m_document->setHtml(tr("<em>Binary data</em>"));
        else
            m_document->setPlainText(text);
        TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(m_document->documentLayout());
        QTC_ASSERT(documentLayout, return true);
        documentLayout->lastSaveRevision = m_document->revision();
        m_document->setModified(false);
        emit titleChanged(title);
        emit changed();
    }
    return true;
}

void BaseTextDocument::reload(QTextCodec *codec)
{
    QTC_ASSERT(codec, return);
    m_codec = codec;
    reload();
}

void BaseTextDocument::reload()
{
    emit aboutToReload();
    if (open(m_fileName))
        emit reloaded();
}

Core::IFile::ReloadBehavior BaseTextDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents) {
        if (state == TriggerInternal && !isModified())
            return BehaviorSilent;
        return BehaviorAsk;
    }
    return BehaviorAsk;
}

void BaseTextDocument::reload(ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return;
    if (type == TypePermissions) {
        emit changed();
    } else {
        reload();
    }
}

void BaseTextDocument::setSyntaxHighlighter(QSyntaxHighlighter *highlighter)
{
    if (m_highlighter)
        delete m_highlighter;
    m_highlighter = highlighter;
    m_highlighter->setParent(this);
    m_highlighter->setDocument(m_document);
}



void BaseTextDocument::cleanWhitespace(const QTextCursor &cursor)
{
    bool hasSelection = cursor.hasSelection();
    QTextCursor copyCursor = cursor;
    copyCursor.beginEditBlock();
    cleanWhitespace(copyCursor, true, true);
    if (!hasSelection)
        ensureFinalNewLine(copyCursor);
    copyCursor.endEditBlock();
}

void BaseTextDocument::cleanWhitespace(QTextCursor& cursor, bool cleanIndentation, bool inEntireDocument)
{

    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(m_document->documentLayout());

    QTextBlock block = m_document->findBlock(cursor.selectionStart());
    QTextBlock end;
    if (cursor.hasSelection())
        end = m_document->findBlock(cursor.selectionEnd()-1).next();

    while (block.isValid() && block != end) {

        if (inEntireDocument || block.revision() != documentLayout->lastSaveRevision) {

            QString blockText = block.text();
            if (int trailing = m_tabSettings.trailingWhitespaces(blockText)) {
                cursor.setPosition(block.position() + block.length() - 1);
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, trailing);
                cursor.removeSelectedText();
            }
            if (cleanIndentation && !m_tabSettings.isIndentationClean(block)) {
                cursor.setPosition(block.position());
                int firstNonSpace = m_tabSettings.firstNonSpace(blockText);
                if (firstNonSpace == blockText.length()) {
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                } else {
                    int column = m_tabSettings.columnAt(blockText, firstNonSpace);
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace);
                    QString indentationString = m_tabSettings.indentationString(0, column, block);
                    cursor.insertText(indentationString);
                }
            }
        }

        block = block.next();
    }
}

void BaseTextDocument::ensureFinalNewLine(QTextCursor& cursor)
{
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    bool emptyFile = !cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);

    if (!emptyFile && cursor.selectedText().at(0) != QChar::ParagraphSeparator)
    {
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        cursor.insertText(QLatin1String("\n"));
    }
}
