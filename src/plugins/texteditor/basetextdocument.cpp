/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basetextdocument.h"

#include "basetextdocumentlayout.h"
#include "basetexteditor.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "syntaxhighlighter.h"
#include "texteditorconstants.h"

#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QFutureInterface>
#include <QtGui/QMainWindow>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QApplication>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>
#include <utils/reloadpromptutils.h>

namespace {
bool verifyDecodingError(const QString &text,
                         QTextCodec *codec,
                         const char *data,
                         const int dataSize,
                         const bool possibleHeader)
{
    QByteArray verifyBuf = codec->fromUnicode(text); // slow
    // the minSize trick lets us ignore unicode headers
    int minSize = qMin(verifyBuf.size(), dataSize);
    return (minSize < dataSize - (possibleHeader? 4 : 0)
            || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                      data + dataSize - minSize,
                      minSize));
}
}

namespace TextEditor {
namespace Internal {

class DocumentMarker : public ITextMarkable
{
    Q_OBJECT
public:
    DocumentMarker(QTextDocument *);

    // ITextMarkable
    bool addMark(ITextMark *mark, int line);
    TextMarks marksAt(int line) const;
    void removeMark(ITextMark *mark);
    bool hasMark(ITextMark *mark) const;
    void updateMark(ITextMark *mark);

private:
    QTextDocument *document;
};

DocumentMarker::DocumentMarker(QTextDocument *doc)
  : ITextMarkable(doc), document(doc)
{
}

bool DocumentMarker::addMark(TextEditor::ITextMark *mark, int line)
{
    QTC_ASSERT(line >= 1, return false);
    int blockNumber = line - 1;
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return false);
    QTextBlock block = document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        TextBlockUserData *userData = BaseTextDocumentLayout::userData(block);
        userData->addMark(mark);
        mark->updateLineNumber(blockNumber + 1);
        mark->updateBlock(block);
        documentLayout->hasMarks = true;
        documentLayout->requestUpdate();
        return true;
    }
    return false;
}

TextEditor::TextMarks DocumentMarker::marksAt(int line) const
{
    QTC_ASSERT(line >= 1, return TextMarks());
    int blockNumber = line - 1;
    QTextBlock block = document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        if (TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(block))
            return userData->marks();
    }
    return TextMarks();
}

void DocumentMarker::removeMark(TextEditor::ITextMark *mark)
{
    bool needUpdate = false;
    QTextBlock block = document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
            needUpdate |= data->removeMark(mark);
        }
        block = block.next();
    }
    if (needUpdate)
        updateMark(0);
}

bool DocumentMarker::hasMark(TextEditor::ITextMark *mark) const
{
    QTextBlock block = document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
            if (data->hasMark(mark))
                return true;
        }
        block = block.next();
    }
    return false;
}

void DocumentMarker::updateMark(ITextMark *mark)
{
    Q_UNUSED(mark)
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->requestUpdate();
}

} // namespace Internal

class BaseTextDocumentPrivate
{
public:
    explicit BaseTextDocumentPrivate(BaseTextDocument *q);

    QString m_fileName;
    QString m_defaultPath;
    QString m_suggestedFileName;
    QString m_mimeType;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    QTextDocument *m_document;
    Internal::DocumentMarker *m_documentMarker;
    SyntaxHighlighter *m_highlighter;

    enum LineTerminatorMode {
        LFLineTerminator,
        CRLFLineTerminator,
        NativeLineTerminator =
#if defined (Q_OS_WIN)
        CRLFLineTerminator
#else
        LFLineTerminator
#endif
    };
    LineTerminatorMode m_lineTerminatorMode;
    QTextCodec *m_codec;
    bool m_fileHasUtf8Bom;

    bool m_fileIsReadOnly;
    bool m_hasDecodingError;
    QByteArray m_decodingErrorSample;
    static const int kChunkSize;
};

const int BaseTextDocumentPrivate::kChunkSize = 65536;

BaseTextDocumentPrivate::BaseTextDocumentPrivate(BaseTextDocument *q) :
    m_document(new QTextDocument(q)),
    m_documentMarker(new Internal::DocumentMarker(m_document)),
    m_highlighter(0),
    m_lineTerminatorMode(NativeLineTerminator),
    m_codec(Core::EditorManager::instance()->defaultTextEncoding()),
    m_fileHasUtf8Bom(false),
    m_fileIsReadOnly(false),
    m_hasDecodingError(false)
{
}

BaseTextDocument::BaseTextDocument() : d(new BaseTextDocumentPrivate(this))
{
}

BaseTextDocument::~BaseTextDocument()
{
    documentClosing();
    delete d->m_document;
    d->m_document = 0;
    delete d;
}

QString BaseTextDocument::mimeType() const
{
    return d->m_mimeType;
}

void BaseTextDocument::setMimeType(const QString &mt)
{
    d->m_mimeType = mt;
}

void BaseTextDocument::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_storageSettings = storageSettings;
}

const StorageSettings &BaseTextDocument::storageSettings() const
{
    return d->m_storageSettings;
}

void BaseTextDocument::setTabSettings(const TabSettings &tabSettings)
{
    d->m_tabSettings = tabSettings;
}

const TabSettings &BaseTextDocument::tabSettings() const
{
    return d->m_tabSettings;
}

QString BaseTextDocument::fileName() const
{
    return d->m_fileName;
}

bool BaseTextDocument::isSaveAsAllowed() const
{
    return true;
}

QString BaseTextDocument::defaultPath() const
{
    return d->m_defaultPath;
}

QString BaseTextDocument::suggestedFileName() const
{
    return d->m_suggestedFileName;
}

void BaseTextDocument::setDefaultPath(const QString &defaultPath)
{
    d->m_defaultPath = defaultPath;
}

void BaseTextDocument::setSuggestedFileName(const QString &suggestedFileName)
{
    d->m_suggestedFileName = suggestedFileName;
}

QTextDocument *BaseTextDocument::document() const
{
    return d->m_document;
}

SyntaxHighlighter *BaseTextDocument::syntaxHighlighter() const
{
    return d->m_highlighter;
}

bool BaseTextDocument::hasDecodingError() const
{
    return d->m_hasDecodingError;
}

QTextCodec *BaseTextDocument::codec() const
{
    return d->m_codec;
}

void BaseTextDocument::setCodec(QTextCodec *c)
{
    d->m_codec = c;
}

QByteArray BaseTextDocument::decodingErrorSample() const
{
    return d->m_decodingErrorSample;
}

ITextMarkable *BaseTextDocument::documentMarker() const
{
    return d->m_documentMarker;
}

bool BaseTextDocument::save(const QString &fileName)
{
    QTextCursor cursor(d->m_document);

    // When saving the current editor, make sure to maintain the cursor position for undo
    Core::IEditor *currentEditor = Core::EditorManager::instance()->currentEditor();
    if (BaseTextEditorEditable *editable = qobject_cast<BaseTextEditorEditable*>(currentEditor)) {
        if (editable->file() == this)
            cursor.setPosition(editable->editor()->textCursor().position());
    }

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);

    if (d->m_storageSettings.m_cleanWhitespace)
        cleanWhitespace(cursor, d->m_storageSettings.m_cleanIndentation, d->m_storageSettings.m_inEntireDocument);
    if (d->m_storageSettings.m_addFinalNewLine)
        ensureFinalNewLine(cursor);
    cursor.endEditBlock();

    QString fName = d->m_fileName;
    if (!fileName.isEmpty())
        fName = fileName;

    QFile file(fName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QString plainText = d->m_document->toPlainText();

    if (d->m_lineTerminatorMode == BaseTextDocumentPrivate::CRLFLineTerminator)
        plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

    Core::IFile::Utf8BomSetting utf8bomSetting = Core::EditorManager::instance()->utf8BomSetting();
    if (d->m_codec->name() == "UTF-8" &&
        (utf8bomSetting == Core::IFile::AlwaysAdd || (utf8bomSetting == Core::IFile::OnlyKeep && d->m_fileHasUtf8Bom))) {
        file.write("\xef\xbb\xbf", 3);
    }

    file.write(d->m_codec->fromUnicode(plainText));
    if (!file.flush())
        return false;
    file.close();

    const QFileInfo fi(fName);
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());

    d->m_document->setModified(false);
    emit titleChanged(fi.fileName());
    emit changed();

    d->m_hasDecodingError = false;
    d->m_decodingErrorSample.clear();

    return true;
}

void BaseTextDocument::rename(const QString &newName)
{
    const QFileInfo fi(newName);
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());
    emit titleChanged(fi.fileName());
    emit changed();
}

bool BaseTextDocument::isReadOnly() const
{
    if (d->m_hasDecodingError)
        return true;
    if (d->m_fileName.isEmpty()) //have no corresponding file, so editing is ok
        return false;
    return d->m_fileIsReadOnly;
}

bool BaseTextDocument::isModified() const
{
    return d->m_document->isModified();
}

void BaseTextDocument::checkPermissions()
{
    bool previousReadOnly = d->m_fileIsReadOnly;
    if (!d->m_fileName.isEmpty()) {
        const QFileInfo fi(d->m_fileName);
        d->m_fileIsReadOnly = !fi.isWritable();
    } else {
        d->m_fileIsReadOnly = false;
    }
    if (previousReadOnly != d->m_fileIsReadOnly)
        emit changed();
}

bool BaseTextDocument::open(const QString &fileName)
{
    QString title = tr("untitled");
    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        d->m_fileIsReadOnly = !fi.isWritable();
        d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        title = fi.fileName();

        QByteArray buf = file.readAll();
        int bytesRead = buf.size();

        QTextCodec *codec = d->m_codec;
        d->m_fileHasUtf8Bom = false;

        // code taken from qtextstream
        if (bytesRead >= 4 && ((uchar(buf[0]) == 0xff && uchar(buf[1]) == 0xfe && uchar(buf[2]) == 0 && uchar(buf[3]) == 0)
                               || (uchar(buf[0]) == 0 && uchar(buf[1]) == 0 && uchar(buf[2]) == 0xfe && uchar(buf[3]) == 0xff))) {
            codec = QTextCodec::codecForName("UTF-32");
        } else if (bytesRead >= 2 && ((uchar(buf[0]) == 0xff && uchar(buf[1]) == 0xfe)
                                      || (uchar(buf[0]) == 0xfe && uchar(buf[1]) == 0xff))) {
            codec = QTextCodec::codecForName("UTF-16");
        } else if (bytesRead >= 3 && ((uchar(buf[0]) == 0xef && uchar(buf[1]) == 0xbb) && uchar(buf[2]) == 0xbf)) {
            codec = QTextCodec::codecForName("UTF-8");
            d->m_fileHasUtf8Bom = true;
        } else if (!codec) {
            codec = QTextCodec::codecForLocale();
        }
        // end code taken from qtextstream

        d->m_codec = codec;

        // An alternative to the code below would be creating a decoder from the codec,
        // but failure detection doesn't seem be working reliably.
        QStringList content;
        if (bytesRead <= BaseTextDocumentPrivate::kChunkSize) {
            QString text = d->m_codec->toUnicode(buf);
            d->m_hasDecodingError = verifyDecodingError(
                text, d->m_codec, buf.constData(), bytesRead, true);
            content.append(text);
        } else {
            // Avoid large allocation of contiguous memory.
            QTextCodec::ConverterState state;
            int offset = 0;
            while (offset < bytesRead) {
                int currentSize = qMin(BaseTextDocumentPrivate::kChunkSize, bytesRead - offset);
                QString text = d->m_codec->toUnicode(buf.constData() + offset, currentSize, &state);
                if (state.remainingChars) {
                    if (currentSize < BaseTextDocumentPrivate::kChunkSize && !d->m_hasDecodingError)
                        d->m_hasDecodingError = true;

                    // Process until the end of the current multi-byte character. Remaining might
                    // actually contain more than needed so try one-be-one.
                    while (state.remainingChars) {
                        text.append(d->m_codec->toUnicode(
                            buf.constData() + offset + currentSize, 1, &state));
                        ++currentSize;
                    }
                }

                if (!d->m_hasDecodingError) {
                    d->m_hasDecodingError = verifyDecodingError(
                        text, d->m_codec, buf.constData() + offset, currentSize, offset == 0);
                }

                offset += currentSize;
                content.append(text);
            }
        }

        if (d->m_hasDecodingError) {
            int p = buf.indexOf('\n', 16384);
            if (p < 0)
                d->m_decodingErrorSample = buf;
            else
                d->m_decodingErrorSample = buf.left(p);
        } else {
            d->m_decodingErrorSample.clear();
        }
        buf.clear();

        foreach (const QString &text, content) {
            int lf = text.indexOf('\n');
            if (lf >= 0) {
                if (lf > 0 && text.at(lf-1) == QLatin1Char('\r')) {
                    d->m_lineTerminatorMode = BaseTextDocumentPrivate::CRLFLineTerminator;
                } else {
                    d->m_lineTerminatorMode = BaseTextDocumentPrivate::LFLineTerminator;
                }
                break;
            }
        }

        d->m_document->setModified(false);
        const int chunks = content.size();
        if (chunks == 1) {
            d->m_document->setPlainText(content.at(0));
        } else {
            QFutureInterface<void> interface;
            interface.setProgressRange(0, chunks);
            Core::ICore::instance()->progressManager()->addTask(
                interface.future(), tr("Opening file"), Constants::TASK_OPEN_FILE);
            interface.reportStarted();
            d->m_document->setUndoRedoEnabled(false);
            QTextCursor c(d->m_document);
            c.beginEditBlock();
            d->m_document->clear();
            for (int i = 0; i < chunks; ++i) {
                c.insertText(content.at(i));
                interface.setProgressValue(i + 1);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
            c.endEditBlock();
            d->m_document->setUndoRedoEnabled(true);
            interface.reportFinished();
        }
        BaseTextDocumentLayout *documentLayout =
            qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
        QTC_ASSERT(documentLayout, return true);
        documentLayout->lastSaveRevision = d->m_document->revision();
        d->m_document->setModified(false);
        emit titleChanged(title);
        emit changed();
    }
    return true;
}

void BaseTextDocument::reload(QTextCodec *codec)
{
    QTC_ASSERT(codec, return);
    d->m_codec = codec;
    reload();
}

void BaseTextDocument::reload()
{
    emit aboutToReload();
    documentClosing(); // removes text marks non-permanently

    if (open(d->m_fileName))
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
        checkPermissions();
    } else {
        reload();
    }
}

void BaseTextDocument::setSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    if (d->m_highlighter)
        delete d->m_highlighter;
    d->m_highlighter = highlighter;
    d->m_highlighter->setParent(this);
    d->m_highlighter->setDocument(d->m_document);
}



void BaseTextDocument::cleanWhitespace(const QTextCursor &cursor)
{
    bool hasSelection = cursor.hasSelection();
    QTextCursor copyCursor = cursor;
    copyCursor.setVisualNavigation(false);
    copyCursor.beginEditBlock();
    cleanWhitespace(copyCursor, true, true);
    if (!hasSelection)
        ensureFinalNewLine(copyCursor);
    copyCursor.endEditBlock();
}

void BaseTextDocument::cleanWhitespace(QTextCursor &cursor, bool cleanIndentation, bool inEntireDocument)
{
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
    Q_ASSERT(cursor.visualNavigation() == false);

    QTextBlock block = d->m_document->findBlock(cursor.selectionStart());
    QTextBlock end;
    if (cursor.hasSelection())
        end = d->m_document->findBlock(cursor.selectionEnd()-1).next();

    while (block.isValid() && block != end) {

        if (inEntireDocument || block.revision() != documentLayout->lastSaveRevision) {

            QString blockText = block.text();
            if (int trailing = d->m_tabSettings.trailingWhitespaces(blockText)) {
                cursor.setPosition(block.position() + block.length() - 1);
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, trailing);
                cursor.removeSelectedText();
            }
            if (cleanIndentation && !d->m_tabSettings.isIndentationClean(block)) {
                cursor.setPosition(block.position());
                int firstNonSpace = d->m_tabSettings.firstNonSpace(blockText);
                if (firstNonSpace == blockText.length()) {
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                } else {
                    int column = d->m_tabSettings.columnAt(blockText, firstNonSpace);
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace);
                    QString indentationString = d->m_tabSettings.indentationString(0, column, block);
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

void BaseTextDocument::documentClosing()
{
    QTextBlock block = d->m_document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData()))
            data->documentClosing();
        block = block.next();
    }
}

} // namespace TextEditor

#include "basetextdocument.moc"
