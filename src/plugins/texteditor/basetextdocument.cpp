/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basetextdocument.h"

#include "basetextdocumentlayout.h"
#include "basetexteditor.h"
#include "typingsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
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
#include <utils/fileutils.h>
#include <utils/reloadpromptutils.h>

namespace TextEditor {
namespace Internal {

class DocumentMarker : public ITextMarkable
{
    Q_OBJECT
public:
    DocumentMarker(QTextDocument *);

    TextMarks marks() const { return m_marksCache; }

    // ITextMarkable
    bool addMark(ITextMark *mark, int line);
    TextMarks marksAt(int line) const;
    void removeMark(ITextMark *mark);
    bool hasMark(ITextMark *mark) const;
    void updateMark(ITextMark *mark);

private:
    double recalculateMaxMarkWidthFactor() const;

    TextMarks m_marksCache; // not owned
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
    BaseTextDocumentLayout *documentLayout =
        qobject_cast<BaseTextDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return false);
    QTextBlock block = document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        TextBlockUserData *userData = BaseTextDocumentLayout::userData(block);
        userData->addMark(mark);
        m_marksCache.append(mark);
        mark->updateLineNumber(blockNumber + 1);
        mark->updateBlock(block);
        documentLayout->hasMarks = true;
        documentLayout->maxMarkWidthFactor = qMax(mark->widthFactor(),
            documentLayout->maxMarkWidthFactor);
        documentLayout->requestUpdate();
        return true;
    }
    return false;
}

double DocumentMarker::recalculateMaxMarkWidthFactor() const
{
    double maxWidthFactor = 1.0;
    foreach (const ITextMark *mark, marks())
        maxWidthFactor = qMax(mark->widthFactor(), maxWidthFactor);
    return maxWidthFactor;
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
    BaseTextDocumentLayout *documentLayout =
        qobject_cast<BaseTextDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return)

    bool needUpdate = false;
    QTextBlock block = document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
            needUpdate |= data->removeMark(mark);
        }
        block = block.next();
    }
    m_marksCache.removeAll(mark);

    if (needUpdate) {
        documentLayout->maxMarkWidthFactor = recalculateMaxMarkWidthFactor();
        updateMark(0);
    }
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
    BaseTextDocumentLayout *documentLayout =
        qobject_cast<BaseTextDocumentLayout*>(document->documentLayout());
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
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    QTextDocument *m_document;
    Internal::DocumentMarker *m_documentMarker;
    SyntaxHighlighter *m_highlighter;

    bool m_fileIsReadOnly;
    bool m_hasHighlightWarning;

    int m_autoSaveRevision;
};

BaseTextDocumentPrivate::BaseTextDocumentPrivate(BaseTextDocument *q) :
    m_document(new QTextDocument(q)),
    m_documentMarker(new Internal::DocumentMarker(m_document)),
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
    return d->m_documentMarker;
}

bool BaseTextDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    QTextCursor cursor(d->m_document);

    // When autosaving, we don't want to modify the document/location under the user's fingers.
    BaseTextEditorWidget *editorWidget = 0;
    int savedPosition = 0;
    int savedAnchor = 0;
    int undos = d->m_document->availableUndoSteps();

    // When saving the current editor, make sure to maintain the cursor position for undo
    Core::IEditor *currentEditor = Core::EditorManager::instance()->currentEditor();
    if (BaseTextEditor *editable = qobject_cast<BaseTextEditor*>(currentEditor)) {
        if (editable->file() == this) {
            editorWidget = editable->editorWidget();
            QTextCursor cur = editorWidget->textCursor();
            savedPosition = cur.position();
            savedAnchor = cur.anchor();
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

    Utils::TextFileFormat saveFormat = format();
    if (saveFormat.codec->name() == "UTF-8") {
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
    } // "UTF-8"

    const bool ok = write(fName, saveFormat, d->m_document->toPlainText(), errorString);

    if (autoSave && undos < d->m_document->availableUndoSteps()) {
        d->m_document->undo();
        if (editorWidget) {
            QTextCursor cur = editorWidget->textCursor();
            cur.setPosition(savedAnchor);
            cur.setPosition(savedPosition, QTextCursor::KeepAnchor);
            editorWidget->setTextCursor(cur);
        }
    }

    if (!ok)
        return false;
    d->m_autoSaveRevision = d->m_document->revision();
    if (autoSave)
        return true;

    const QFileInfo fi(fName);
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());

    d->m_document->setModified(false);
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
    d->m_fileName = QDir::cleanPath(fi.absoluteFilePath());
    emit titleChanged(fi.fileName());
    emit changed();
}

bool BaseTextDocument::isReadOnly() const
{
    if (hasDecodingError())
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
    documentClosing(); // removes text marks non-permanently

    if (!open(errorString, d->m_fileName, d->m_fileName))
        return false;
    emit reloaded();
    return true;
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

void BaseTextDocument::documentClosing()
{
    QTextBlock block = d->m_document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData()))
            data->documentClosing();
        block = block.next();
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

#include "basetextdocument.moc"
