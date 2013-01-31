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

#include "basetextdocument.h"

#include "basetextdocumentlayout.h"
#include "basetexteditor.h"
#include "typingsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "syntaxhighlighter.h"
#include "texteditorconstants.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureInterface>
#include <QScrollBar>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QTextCodec>
#include <QTextStream>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <utils/reloadpromptutils.h>

namespace TextEditor {
class BaseTextDocumentPrivate
{
public:
    explicit BaseTextDocumentPrivate(BaseTextDocument *q);

    QString m_fileName;
    QString m_defaultPath;
    QString m_suggestedFileName;
    QString m_mimeType;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    QTextDocument *m_document;
    SyntaxHighlighter *m_highlighter;

    bool m_fileIsReadOnly;
    bool m_hasHighlightWarning;

    int m_autoSaveRevision;
};

BaseTextDocumentPrivate::BaseTextDocumentPrivate(BaseTextDocument *q) :
    m_document(new QTextDocument(q)),
    m_highlighter(0),
    m_fileIsReadOnly(false),
    m_hasHighlightWarning(false),
    m_autoSaveRevision(-1)
{
}

BaseTextDocument::BaseTextDocument() : d(new BaseTextDocumentPrivate(this))
{
}

BaseTextDocument::~BaseTextDocument()
{
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

void BaseTextDocument::setTypingSettings(const TypingSettings &typingSettings)
{
    d->m_typingSettings = typingSettings;
}

void BaseTextDocument::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_storageSettings = storageSettings;
}

const TypingSettings &BaseTextDocument::typingSettings() const
{
    return d->m_typingSettings;
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

void BaseTextDocument::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_extraEncodingSettings = extraEncodingSettings;
}

const ExtraEncodingSettings &BaseTextDocument::extraEncodingSettings() const
{
    return d->m_extraEncodingSettings;
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

ITextMarkable *BaseTextDocument::documentMarker() const
{
    BaseTextDocumentLayout *documentLayout =
        qobject_cast<BaseTextDocumentLayout *>(d->m_document->documentLayout());
    QTC_ASSERT(documentLayout, return 0);
    return documentLayout->markableInterface();
}

/*!
 * \brief Saves the document to the specified file.
 * \param errorString output parameter, contains error reason.
 * \param autoSave signalise that this function was called by the automatic save routine.
 * If autosave is true, the cursor will be restored and some signals suppressed.
 */
bool BaseTextDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    QTextCursor cursor(d->m_document);

    // When autosaving, we don't want to modify the document/location under the user's fingers.
    BaseTextEditorWidget *editorWidget = 0;
    int savedPosition = 0;
    int savedAnchor = 0;
    int savedVScrollBarValue = 0;
    int savedHScrollBarValue = 0;
    int undos = d->m_document->availableUndoSteps();

    // When saving the current editor, make sure to maintain the cursor and scroll bar
    // positions for undo
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
    if (BaseTextEditor *editable = qobject_cast<BaseTextEditor*>(currentEditor)) {
        if (editable->document() == this) {
            editorWidget = editable->editorWidget();
            QTextCursor cur = editorWidget->textCursor();
            savedPosition = cur.position();
            savedAnchor = cur.anchor();
            savedVScrollBarValue = editorWidget->verticalScrollBar()->value();
            savedHScrollBarValue = editorWidget->horizontalScrollBar()->value();
            cursor.setPosition(cur.position());
        }
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

    // check if UTF8-BOM has to be added or removed
    Utils::TextFileFormat saveFormat = format();
    if (saveFormat.codec->name() == "UTF-8" && supportsUtf8Bom()) {
        switch (d->m_extraEncodingSettings.m_utf8BomSetting) {
        case TextEditor::ExtraEncodingSettings::AlwaysAdd:
            saveFormat.hasUtf8Bom = true;
            break;
        case TextEditor::ExtraEncodingSettings::OnlyKeep:
            break;
        case TextEditor::ExtraEncodingSettings::AlwaysDelete:
            saveFormat.hasUtf8Bom = false;
            break;
        }
    }

    const bool ok = write(fName, saveFormat, d->m_document->toPlainText(), errorString);

    // restore text cursor and scroll bar positions
    if (autoSave && undos < d->m_document->availableUndoSteps()) {
        d->m_document->undo();
        if (editorWidget) {
            QTextCursor cur = editorWidget->textCursor();
            cur.setPosition(savedAnchor);
            cur.setPosition(savedPosition, QTextCursor::KeepAnchor);
            editorWidget->verticalScrollBar()->setValue(savedVScrollBarValue);
            editorWidget->horizontalScrollBar()->setValue(savedHScrollBarValue);
            editorWidget->setTextCursor(cur);
        }
    }

    if (!ok)
        return false;
    d->m_autoSaveRevision = d->m_document->revision();
    if (autoSave)
        return true;

    // inform about the new filename
    const QFileInfo fi(fName);
    const QString oldFileName = d->m_fileName;
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());
    d->m_document->setModified(false);
    emit fileNameChanged(oldFileName, d->m_fileName);
    emit titleChanged(fi.fileName());
    emit changed();
    return true;
}

bool BaseTextDocument::shouldAutoSave() const
{
    return d->m_autoSaveRevision != d->m_document->revision();
}

void BaseTextDocument::rename(const QString &newName)
{
    const QFileInfo fi(newName);
    const QString oldFileName = d->m_fileName;
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());
    emit fileNameChanged(oldFileName, d->m_fileName);
    emit titleChanged(fi.fileName());
    emit changed();
}

bool BaseTextDocument::isFileReadOnly() const
{
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

bool BaseTextDocument::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    QString title = tr("untitled");
    QStringList content;

    ReadResult readResult = Utils::TextFileFormat::ReadIOError;

    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        d->m_fileIsReadOnly = !fi.isWritable();
        d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());

        title = fi.fileName();
        readResult = read(realFileName, &content, errorString);

        d->m_document->setModified(false);
        const int chunks = content.size();
        if (chunks == 0) {
            d->m_document->setPlainText(QString());
        } else if (chunks == 1) {
            d->m_document->setPlainText(content.at(0));
        } else {
            QFutureInterface<void> interface;
            interface.setProgressRange(0, chunks);
            Core::ICore::progressManager()->addTask(
                interface.future(), tr("Opening file"), QLatin1String(Constants::TASK_OPEN_FILE));
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
        documentLayout->lastSaveRevision = d->m_autoSaveRevision = d->m_document->revision();
        d->m_document->setModified(fileName != realFileName);
        emit titleChanged(title);
        emit changed();
    }
    return readResult == Utils::TextFileFormat::ReadSuccess
           || readResult == Utils::TextFileFormat::ReadEncodingError;
}

bool BaseTextDocument::reload(QString *errorString, QTextCodec *codec)
{
    QTC_ASSERT(codec, return false);
    setCodec(codec);
    return reload(errorString);
}

bool BaseTextDocument::reload(QString *errorString)
{
    emit aboutToReload();
    BaseTextDocumentLayout *documentLayout =
        qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
    TextMarks marks;
    if (documentLayout)
        marks = documentLayout->documentClosing(); // removes text marks non-permanently

    bool success = open(errorString, d->m_fileName, d->m_fileName);

    if (documentLayout)
        documentLayout->documentReloaded(marks); // readds text marks
    if (success)
        emit reloaded();
    return success;
}

bool BaseTextDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        checkPermissions();
        return true;
    } else {
        return reload(errorString);
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

bool BaseTextDocument::hasHighlightWarning() const
{
    return d->m_hasHighlightWarning;
}

void BaseTextDocument::setHighlightWarning(bool has)
{
    d->m_hasHighlightWarning = has;
}

} // namespace TextEditor
