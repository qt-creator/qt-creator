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

#include "basetextdocument.h"

#include "basetextdocumentlayout.h"
#include "basetexteditor.h"
#include "convenience.h"
#include "extraencodingsettings.h"
#include "fontsettings.h"
#include "indenter.h"
#include "storagesettings.h"
#include "syntaxhighlighter.h"
#include "tabsettings.h"
#include "texteditorconstants.h"
#include "typingsettings.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFutureInterface>
#include <QScrollBar>
#include <QStringList>
#include <QTextCodec>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>

using namespace Core;

/*!
    \class TextEditor::BaseTextDocument
    \brief The BaseTextDocument class is the base class for QTextDocument based documents.

    Subclasses of BaseTextEditor can either use BaseTextDocument as is (and this is the default),
    or created subclasses of BaseTextDocument if they have special requirements.
*/

namespace TextEditor {
class BaseTextDocumentPrivate : public QObject
{
    Q_OBJECT
public:
    explicit BaseTextDocumentPrivate(BaseTextDocument *q);

    QTextCursor indentOrUnindent(const QTextCursor &textCursor, bool doIndent);
    void resetRevisions();
    void updateRevisions();

public slots:
    void onModificationChanged(bool modified);

public:
    QString m_defaultPath;
    QString m_suggestedFileName;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    FontSettings m_fontSettings;
    bool m_fontSettingsNeedsApply; // for applying font settings delayed till an editor becomes visible
    QTextDocument *m_document;
    SyntaxHighlighter *m_highlighter;
    QScopedPointer<Indenter> m_indenter;

    bool m_fileIsReadOnly;
    int m_autoSaveRevision;

    TextMarks m_marksCache; // Marks not owned
};

BaseTextDocumentPrivate::BaseTextDocumentPrivate(BaseTextDocument *q) :
    m_fontSettingsNeedsApply(false),
    m_document(new QTextDocument(q)),
    m_highlighter(0),
    m_indenter(new Indenter),
    m_fileIsReadOnly(false),
    m_autoSaveRevision(-1)
{
}

QTextCursor BaseTextDocumentPrivate::indentOrUnindent(const QTextCursor &textCursor, bool doIndent)
{
    QTextCursor cursor = textCursor;
    cursor.beginEditBlock();

    if (cursor.hasSelection()) {
        // Indent or unindent the selected lines
        int pos = cursor.position();
        int anchor = cursor.anchor();
        int start = qMin(anchor, pos);
        int end = qMax(anchor, pos);

        QTextBlock startBlock = m_document->findBlock(start);
        QTextBlock endBlock = m_document->findBlock(end-1).next();

        if (startBlock.next() == endBlock
                && (start > startBlock.position() || end < endBlock.position() - 1)) {
            // Only one line partially selected.
            cursor.removeSelectedText();
        } else {
            for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
                QString text = block.text();
                int indentPosition = m_tabSettings.lineIndentPosition(text);
                if (!doIndent && !indentPosition)
                    indentPosition = m_tabSettings.firstNonSpace(text);
                int targetColumn = m_tabSettings.indentedColumn(m_tabSettings.columnAt(text, indentPosition), doIndent);
                cursor.setPosition(block.position() + indentPosition);
                cursor.insertText(m_tabSettings.indentationString(0, targetColumn, block));
                cursor.setPosition(block.position());
                cursor.setPosition(block.position() + indentPosition, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            }
            cursor.endEditBlock();
            return textCursor;
        }
    }

    // Indent or unindent at cursor position
    QTextBlock block = cursor.block();
    QString text = block.text();
    int indentPosition = cursor.positionInBlock();
    int spaces = m_tabSettings.spacesLeftFromPosition(text, indentPosition);
    int startColumn = m_tabSettings.columnAt(text, indentPosition - spaces);
    int targetColumn = m_tabSettings.indentedColumn(m_tabSettings.columnAt(text, indentPosition), doIndent);
    cursor.setPosition(block.position() + indentPosition);
    cursor.setPosition(block.position() + indentPosition - spaces, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(m_tabSettings.indentationString(startColumn, targetColumn, block));
    cursor.endEditBlock();
    return cursor;
}

void BaseTextDocumentPrivate::resetRevisions()
{
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(m_document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->lastSaveRevision = m_document->revision();

    for (QTextBlock block = m_document->begin(); block.isValid(); block = block.next())
        block.setRevision(documentLayout->lastSaveRevision);
}

void BaseTextDocumentPrivate::updateRevisions()
{
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(m_document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    int oldLastSaveRevision = documentLayout->lastSaveRevision;
    documentLayout->lastSaveRevision = m_document->revision();

    if (oldLastSaveRevision != documentLayout->lastSaveRevision) {
        for (QTextBlock block = m_document->begin(); block.isValid(); block = block.next()) {
            if (block.revision() < 0 || block.revision() != oldLastSaveRevision)
                block.setRevision(-documentLayout->lastSaveRevision - 1);
            else
                block.setRevision(documentLayout->lastSaveRevision);
        }
    }
}

void BaseTextDocumentPrivate::onModificationChanged(bool modified)
{
    // we only want to update the block revisions when going back to the saved version,
    // e.g. with undo
    if (!modified)
        updateRevisions();
}

BaseTextDocument::BaseTextDocument() : d(new BaseTextDocumentPrivate(this))
{
    connect(d->m_document, SIGNAL(modificationChanged(bool)), d, SLOT(onModificationChanged(bool)));
    connect(d->m_document, SIGNAL(modificationChanged(bool)), this, SIGNAL(changed()));
    connect(d->m_document, SIGNAL(contentsChanged()), this, SIGNAL(contentsChanged()));

    // set new document layout
    QTextOption opt = d->m_document->defaultTextOption();
    opt.setTextDirection(Qt::LeftToRight);
    opt.setFlags(opt.flags() | QTextOption::IncludeTrailingSpaces
            | QTextOption::AddSpaceForLineAndParagraphSeparators
            );
    d->m_document->setDefaultTextOption(opt);
    BaseTextDocumentLayout *documentLayout = new BaseTextDocumentLayout(d->m_document);
    d->m_document->setDocumentLayout(documentLayout);
}

BaseTextDocument::~BaseTextDocument()
{
    delete d->m_document;
    d->m_document = 0;
    delete d;
}

QString BaseTextDocument::plainText() const
{
    return document()->toPlainText();
}

QString BaseTextDocument::textAt(int pos, int length) const
{
    return Convenience::textAt(QTextCursor(document()), pos, length);
}

QChar BaseTextDocument::characterAt(int pos) const
{
    return document()->characterAt(pos);
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

void BaseTextDocument::setTabSettings(const TextEditor::TabSettings &tabSettings)
{
    if (tabSettings == d->m_tabSettings)
        return;
    d->m_tabSettings = tabSettings;
    emit tabSettingsChanged();
}

const TabSettings &BaseTextDocument::tabSettings() const
{
    return d->m_tabSettings;
}

void BaseTextDocument::setFontSettings(const FontSettings &fontSettings)
{
    if (fontSettings == d->m_fontSettings)
        return;
    d->m_fontSettings = fontSettings;
    d->m_fontSettingsNeedsApply = true;
    emit fontSettingsChanged();
}

void BaseTextDocument::triggerPendingUpdates()
{
    if (d->m_fontSettingsNeedsApply)
        applyFontSettings();
}

void BaseTextDocument::applyFontSettings()
{
    d->m_fontSettingsNeedsApply = false;
    if (d->m_highlighter) {
        d->m_highlighter->setFontSettings(d->m_fontSettings);
        d->m_highlighter->rehighlight();
    }
}

const FontSettings &BaseTextDocument::fontSettings() const
{
    return d->m_fontSettings;
}

void BaseTextDocument::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_extraEncodingSettings = extraEncodingSettings;
}

void BaseTextDocument::autoIndent(const QTextCursor &cursor, QChar typedChar)
{
    d->m_indenter->indent(d->m_document, cursor, typedChar, d->m_tabSettings);
}

void BaseTextDocument::autoReindent(const QTextCursor &cursor)
{
    d->m_indenter->reindent(d->m_document, cursor, d->m_tabSettings);
}

QTextCursor BaseTextDocument::indent(const QTextCursor &cursor)
{
    return d->indentOrUnindent(cursor, true);
}

QTextCursor BaseTextDocument::unindent(const QTextCursor &cursor)
{
    return d->indentOrUnindent(cursor, false);
}

const ExtraEncodingSettings &BaseTextDocument::extraEncodingSettings() const
{
    return d->m_extraEncodingSettings;
}

void BaseTextDocument::setIndenter(Indenter *indenter)
{
    // clear out existing code formatter data
    for (QTextBlock it = document()->begin(); it.isValid(); it = it.next()) {
        TextEditor::TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(it);
        if (userData)
            userData->setCodeFormatterData(0);
    }
    d->m_indenter.reset(indenter);
}

Indenter *BaseTextDocument::indenter() const
{
    return d->m_indenter.data();
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

/*!
 * Saves the document to the file specified by \a fileName. If errors occur,
 * \a errorString contains their cause.
 * \a autoSave returns whether this function was called by the automatic save routine.
 * If \a autoSave is true, the cursor will be restored and some signals suppressed
 * and we do not clean up the text file (cleanWhitespace(), ensureFinalNewLine()).
 */
bool BaseTextDocument::save(QString *errorString, const QString &saveFileName, bool autoSave)
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
    IEditor *currentEditor = EditorManager::currentEditor();
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

    if (!autoSave) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::Start);

        if (d->m_storageSettings.m_cleanWhitespace)
          cleanWhitespace(cursor, d->m_storageSettings.m_cleanIndentation, d->m_storageSettings.m_inEntireDocument);
        if (d->m_storageSettings.m_addFinalNewLine)
          ensureFinalNewLine(cursor);
        cursor.endEditBlock();
      }

    QString fName = filePath();
    if (!saveFileName.isEmpty())
        fName = saveFileName;

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
    d->m_document->setModified(false); // also triggers update of the block revisions
    setFilePath(QDir::cleanPath(fi.absoluteFilePath()));
    emit changed();
    return true;
}

bool BaseTextDocument::setContents(const QByteArray &contents)
{
    return setPlainText(QString::fromUtf8(contents));
}

bool BaseTextDocument::shouldAutoSave() const
{
    return d->m_autoSaveRevision != d->m_document->revision();
}

void BaseTextDocument::setFilePath(const QString &newName)
{
    if (newName == filePath())
        return;
    const QFileInfo fi(newName);
    IDocument::setFilePath(QDir::cleanPath(fi.absoluteFilePath()));
}

bool BaseTextDocument::isFileReadOnly() const
{
    if (filePath().isEmpty()) //have no corresponding file, so editing is ok
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
    if (!filePath().isEmpty()) {
        const QFileInfo fi(filePath());
        d->m_fileIsReadOnly = !fi.isWritable();
    } else {
        d->m_fileIsReadOnly = false;
    }
    if (previousReadOnly != d->m_fileIsReadOnly)
        emit changed();
}

bool BaseTextDocument::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    QStringList content;

    ReadResult readResult = Utils::TextFileFormat::ReadIOError;

    if (!fileName.isEmpty()) {
        const QFileInfo fi(fileName);
        d->m_fileIsReadOnly = !fi.isWritable();
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
            ProgressManager::addTask(interface.future(), tr("Opening File"), Constants::TASK_OPEN_FILE);
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
        d->updateRevisions();
        d->m_document->setModified(fileName != realFileName);
        setFilePath(QDir::cleanPath(fi.absoluteFilePath()));
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

    bool success = open(errorString, filePath(), filePath());

    if (documentLayout)
        documentLayout->documentReloaded(marks, this); // re-adds text marks
    emit reloadFinished(success);
    return success;
}

bool BaseTextDocument::setPlainText(const QString &text)
{
    if (text.size() > EditorManager::maxTextFileSize()) {
        document()->setPlainText(BaseTextEditorWidget::msgTextTooLarge(text.size()));
        d->resetRevisions();
        document()->setModified(false);
        return false;
    }
    document()->setPlainText(text);
    d->resetRevisions();
    document()->setModified(false);
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
            d->m_tabSettings.removeTrailingWhitespace(cursor, block);
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

TextMarks BaseTextDocument::marks() const
{
    return d->m_marksCache;
}

bool BaseTextDocument::addMark(ITextMark *mark)
{
    if (mark->baseTextDocument())
        return false;
    QTC_ASSERT(mark->lineNumber() >= 1, return false);
    int blockNumber = mark->lineNumber() - 1;
    auto documentLayout = qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
    QTC_ASSERT(documentLayout, return false);
    QTextBlock block = d->m_document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        TextBlockUserData *userData = BaseTextDocumentLayout::userData(block);
        userData->addMark(mark);
        d->m_marksCache.append(mark);
        mark->updateLineNumber(blockNumber + 1);
        QTC_CHECK(mark->lineNumber() == blockNumber + 1); // Checks that the base class is called
        mark->updateBlock(block);
        mark->setBaseTextDocument(this);
        if (!mark->isVisible())
            return true;
        // Update document layout
        double newMaxWidthFactor = qMax(mark->widthFactor(), documentLayout->maxMarkWidthFactor);
        bool fullUpdate =  newMaxWidthFactor > documentLayout->maxMarkWidthFactor || !documentLayout->hasMarks;
        documentLayout->hasMarks = true;
        documentLayout->maxMarkWidthFactor = newMaxWidthFactor;
        if (fullUpdate)
            documentLayout->requestUpdate();
        else
            documentLayout->requestExtraAreaUpdate();
        return true;
    }
    return false;
}

TextMarks BaseTextDocument::marksAt(int line) const
{
    QTC_ASSERT(line >= 1, return TextMarks());
    int blockNumber = line - 1;
    QTextBlock block = d->m_document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        if (TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(block))
            return userData->marks();
    }
    return TextMarks();
}

void BaseTextDocument::removeMarkFromMarksCache(ITextMark *mark)
{
    auto documentLayout = qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    d->m_marksCache.removeAll(mark);

    if (d->m_marksCache.isEmpty()) {
        documentLayout->hasMarks = false;
        documentLayout->maxMarkWidthFactor = 1.0;
        documentLayout->requestUpdate();
        return;
    }

    if (!mark->isVisible())
        return;

    if (documentLayout->maxMarkWidthFactor == 1.0
            || mark->widthFactor() == 1.0
            || mark->widthFactor() < documentLayout->maxMarkWidthFactor) {
        // No change in width possible
        documentLayout->requestExtraAreaUpdate();
    } else {
        double maxWidthFactor = 1.0;
        foreach (const ITextMark *mark, marks()) {
            if (!mark->isVisible())
                continue;
            maxWidthFactor = qMax(mark->widthFactor(), maxWidthFactor);
            if (maxWidthFactor == documentLayout->maxMarkWidthFactor)
                break; // Still a mark with the maxMarkWidthFactor
        }

        if (maxWidthFactor != documentLayout->maxMarkWidthFactor) {
            documentLayout->maxMarkWidthFactor = maxWidthFactor;
            documentLayout->requestUpdate();
        } else {
            documentLayout->requestExtraAreaUpdate();
        }
    }
}

void BaseTextDocument::removeMark(ITextMark *mark)
{
    QTextBlock block = d->m_document->findBlockByNumber(mark->lineNumber() - 1);
    if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
        if (!data->removeMark(mark))
            qDebug() << "Could not find mark" << mark << "on line" << mark->lineNumber();
    }

    removeMarkFromMarksCache(mark);
    mark->setBaseTextDocument(0);
}

void BaseTextDocument::updateMark(ITextMark *mark)
{
    Q_UNUSED(mark)
    auto documentLayout = qobject_cast<BaseTextDocumentLayout*>(d->m_document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->requestUpdate();
}

void BaseTextDocument::moveMark(ITextMark *mark, int previousLine)
{
    QTextBlock block = d->m_document->findBlockByNumber(previousLine - 1);
    if (TextBlockUserData *data = BaseTextDocumentLayout::testUserData(block)) {
        if (!data->removeMark(mark))
            qDebug() << "Could not find mark" << mark << "on line" << previousLine;
    }
    removeMarkFromMarksCache(mark);
    mark->setBaseTextDocument(0);
    addMark(mark);
}

} // namespace TextEditor

#include "basetextdocument.moc"
