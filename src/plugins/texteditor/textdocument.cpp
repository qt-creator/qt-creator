// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocument.h"

#include "extraencodingsettings.h"
#include "fontsettings.h"
#include "storagesettings.h"
#include "syntaxhighlighter.h"
#include "tabsettings.h"
#include "textdocumentlayout.h"
#include "texteditor.h"
#include "texteditortr.h"
#include "textindenter.h"
#include "typingsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/guard.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QAction>
#include <QApplication>
#include <QFutureInterface>
#include <QScrollBar>
#include <QStringList>
#include <QTextCodec>

using namespace Core;
using namespace Utils;

/*!
    \class TextEditor::BaseTextDocument
    \brief The BaseTextDocument class is the base class for QTextDocument based documents.

    It is the base class for documents used by implementations of the BaseTextEditor class,
    and contains basic functions for retrieving text content and markers (like bookmarks).

    Subclasses of BaseTextEditor can either use BaseTextDocument as is (and this is the default),
    or created subclasses of BaseTextDocument if they have special requirements.
*/

namespace TextEditor {

class TextDocumentPrivate
{
public:
    TextDocumentPrivate()
        : m_indenter(new TextIndenter(&m_document))
    {
    }

    MultiTextCursor indentOrUnindent(const MultiTextCursor &cursor, bool doIndent, const TabSettings &tabSettings);
    void resetRevisions();
    void updateRevisions();

public:
    FilePath m_defaultPath;
    QString m_suggestedFileName;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    FontSettings m_fontSettings;
    bool m_fontSettingsNeedsApply = false; // for applying font settings delayed till an editor becomes visible
    QTextDocument m_document;
    SyntaxHighlighter *m_highlighter = nullptr;
    CompletionAssistProvider *m_completionAssistProvider = nullptr;
    CompletionAssistProvider *m_functionHintAssistProvider = nullptr;
    IAssistProvider *m_quickFixProvider = nullptr;
    QScopedPointer<Indenter> m_indenter;
    QScopedPointer<Formatter> m_formatter;

    int m_autoSaveRevision = -1;
    bool m_silentReload = false;

    TextMarks m_marksCache; // Marks not owned
    Utils::Guard m_modificationChangedGuard;
};

MultiTextCursor TextDocumentPrivate::indentOrUnindent(const MultiTextCursor &cursors,
                                                      bool doIndent,
                                                      const TabSettings &tabSettings)
{
    MultiTextCursor result;
    bool first = true;
    for (const QTextCursor &textCursor : cursors) {
        QTextCursor cursor = textCursor;
        if (first) {
            cursor.beginEditBlock();
            first = false;
        } else {
            cursor.joinPreviousEditBlock();
        }

        // Indent or unindent the selected lines
        int pos = cursor.position();
        int column = tabSettings.columnAt(cursor.block().text(), cursor.positionInBlock());
        int anchor = cursor.anchor();
        int start = qMin(anchor, pos);
        int end = qMax(anchor, pos);

        QTextBlock startBlock = m_document.findBlock(start);
        QTextBlock endBlock = m_document.findBlock(qMax(end - 1, 0)).next();
        const bool cursorAtBlockStart = (cursor.position() == startBlock.position());
        const bool anchorAtBlockStart = (cursor.anchor() == startBlock.position());
        const bool oneLinePartial = (startBlock.next() == endBlock)
                                    && (start > startBlock.position()
                                        || end < endBlock.position() - 1)
                                    && !cursors.hasMultipleCursors();

        // Make sure one line selection will get processed in "for" loop
        if (startBlock == endBlock)
            endBlock = endBlock.next();

        if (cursor.hasSelection()) {
            if (oneLinePartial) {
                cursor.removeSelectedText();
            } else {
                for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
                    const QString text = block.text();
                    int indentPosition = tabSettings.lineIndentPosition(text);
                    if (!doIndent && !indentPosition)
                        indentPosition = TabSettings::firstNonSpace(text);
                    int targetColumn
                        = tabSettings.indentedColumn(tabSettings.columnAt(text, indentPosition),
                                                     doIndent);
                    cursor.setPosition(block.position() + indentPosition);
                    cursor.insertText(tabSettings.indentationString(0, targetColumn, 0, block));
                    cursor.setPosition(block.position());
                    cursor.setPosition(block.position() + indentPosition, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                }
                // make sure that selection that begins in first column stays at first column
                // even if we insert text at first column
                cursor = textCursor;
                if (cursorAtBlockStart) {
                    cursor.setPosition(startBlock.position(), QTextCursor::KeepAnchor);
                } else if (anchorAtBlockStart) {
                    cursor.setPosition(startBlock.position(), QTextCursor::MoveAnchor);
                    cursor.setPosition(textCursor.position(), QTextCursor::KeepAnchor);
                }
            }
        } else {
            QString text = startBlock.text();
            int indentPosition = tabSettings.positionAtColumn(text, column, nullptr, true);
            int spaces = tabSettings.spacesLeftFromPosition(text, indentPosition);
            int startColumn = tabSettings.columnAt(text, indentPosition - spaces);
            int targetColumn = tabSettings.indentedColumn(tabSettings.columnAt(text, indentPosition),
                                                          doIndent);
            cursor.setPosition(startBlock.position() + indentPosition);
            cursor.setPosition(startBlock.position() + indentPosition - spaces,
                               QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText(
                tabSettings.indentationString(startColumn, targetColumn, 0, startBlock));
        }

        cursor.endEditBlock();
        result.addCursor(cursor);
    }

    return result;
}

void TextDocumentPrivate::resetRevisions()
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(m_document.documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->lastSaveRevision = m_document.revision();

    for (QTextBlock block = m_document.begin(); block.isValid(); block = block.next())
        block.setRevision(documentLayout->lastSaveRevision);
}

void TextDocumentPrivate::updateRevisions()
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(m_document.documentLayout());
    QTC_ASSERT(documentLayout, return);
    int oldLastSaveRevision = documentLayout->lastSaveRevision;
    documentLayout->lastSaveRevision = m_document.revision();

    if (oldLastSaveRevision != documentLayout->lastSaveRevision) {
        for (QTextBlock block = m_document.begin(); block.isValid(); block = block.next()) {
            if (block.revision() < 0 || block.revision() != oldLastSaveRevision)
                block.setRevision(-documentLayout->lastSaveRevision - 1);
            else
                block.setRevision(documentLayout->lastSaveRevision);
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// BaseTextDocument
//
///////////////////////////////////////////////////////////////////////

TextDocument::TextDocument(Id id)
    : d(new TextDocumentPrivate)
{
    d->m_document.setParent(this);
    connect(&d->m_document, &QTextDocument::modificationChanged,
            this, &TextDocument::modificationChanged);
    connect(&d->m_document, &QTextDocument::contentsChanged,
            this, &Core::IDocument::contentsChanged);
    connect(&d->m_document, &QTextDocument::contentsChange,
            this, &TextDocument::contentsChangedWithPosition);

    // set new document layout
    QTextOption opt = d->m_document.defaultTextOption();
    opt.setTextDirection(Qt::LeftToRight);
    opt.setFlags(opt.flags() | QTextOption::IncludeTrailingSpaces
            | QTextOption::AddSpaceForLineAndParagraphSeparators
            );
    d->m_document.setDefaultTextOption(opt);
    d->m_document.setDocumentLayout(new TextDocumentLayout(&d->m_document));

    if (id.isValid())
        setId(id);

    setSuspendAllowed(true);
}

TextDocument::~TextDocument()
{
    delete d;
}

QMap<FilePath, QString> TextDocument::openedTextDocumentContents()
{
    QMap<FilePath, QString> workingCopy;
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        auto textEditorDocument = qobject_cast<TextDocument *>(document);
        if (!textEditorDocument)
            continue;
        const FilePath fileName = textEditorDocument->filePath();
        workingCopy[fileName] = textEditorDocument->plainText();
    }
    return workingCopy;
}

QMap<FilePath, QTextCodec *> TextDocument::openedTextDocumentEncodings()
{
    QMap<FilePath, QTextCodec *> workingCopy;
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        auto textEditorDocument = qobject_cast<TextDocument *>(document);
        if (!textEditorDocument)
            continue;
        const FilePath fileName = textEditorDocument->filePath();
        workingCopy[fileName] = const_cast<QTextCodec *>(textEditorDocument->codec());
    }
    return workingCopy;
}

TextDocument *TextDocument::currentTextDocument()
{
    return qobject_cast<TextDocument *>(EditorManager::currentDocument());
}

TextDocument *TextDocument::textDocumentForFilePath(const Utils::FilePath &filePath)
{
    return qobject_cast<TextDocument *>(DocumentModel::documentForFilePath(filePath));
}

QString TextDocument::convertToPlainText(const QString &rawText)
{
    // This is basically a copy of QTextDocument::toPlainText but since toRawText returns a
    // text containing formating characters and toPlainText replaces non breaking spaces, we
    // provide our own plain text conversion to be able to save and copy document content
    // containing non breaking spaces.

    QString txt = rawText;
    QChar *uc = txt.data();
    QChar *e = uc + txt.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        default:;
        }
    }
    return txt;
}

QString TextDocument::plainText() const
{
    return convertToPlainText(d->m_document.toRawText());
}

QString TextDocument::textAt(int pos, int length) const
{
    return Utils::Text::textAt(QTextCursor(document()), pos, length);
}

QChar TextDocument::characterAt(int pos) const
{
    return document()->characterAt(pos);
}

void TextDocument::setTypingSettings(const TypingSettings &typingSettings)
{
    d->m_typingSettings = typingSettings;
}

void TextDocument::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_storageSettings = storageSettings;
}

const TypingSettings &TextDocument::typingSettings() const
{
    return d->m_typingSettings;
}

const StorageSettings &TextDocument::storageSettings() const
{
    return d->m_storageSettings;
}

void TextDocument::setTabSettings(const TabSettings &newTabSettings)
{
    if (newTabSettings == d->m_tabSettings)
        return;
    d->m_tabSettings = newTabSettings;

    emit tabSettingsChanged();
}

TabSettings TextDocument::tabSettings() const
{
    return d->m_tabSettings;
}

void TextDocument::setFontSettings(const FontSettings &fontSettings)
{
    if (fontSettings == d->m_fontSettings)
        return;
    d->m_fontSettings = fontSettings;
    d->m_fontSettingsNeedsApply = true;
    emit fontSettingsChanged();
}

QAction *TextDocument::createDiffAgainstCurrentFileAction(
    QObject *parent, const std::function<Utils::FilePath()> &filePath)
{
    const auto diffAgainstCurrentFile = [filePath]() {
        auto diffService = DiffService::instance();
        auto textDocument = TextEditor::TextDocument::currentTextDocument();
        const QString leftFilePath = textDocument ? textDocument->filePath().toString() : QString();
        const QString rightFilePath = filePath().toString();
        if (diffService && !leftFilePath.isEmpty() && !rightFilePath.isEmpty())
            diffService->diffFiles(leftFilePath, rightFilePath);
    };
    auto diffAction = new QAction(Tr::tr("Diff Against Current File"), parent);
    QObject::connect(diffAction, &QAction::triggered, parent, diffAgainstCurrentFile);
    return diffAction;
}

void TextDocument::insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion)
{
    QTextCursor cursor(&d->m_document);
    cursor.setPosition(suggestion->position());
    const QTextBlock block = cursor.block();
    TextDocumentLayout::userData(block)->insertSuggestion(std::move(suggestion));
    TextDocumentLayout::updateSuggestionFormats(block, fontSettings());
    updateLayout();
}

#ifdef WITH_TESTS
void TextDocument::setSilentReload()
{
    d->m_silentReload = true;
}
#endif

void TextDocument::triggerPendingUpdates()
{
    if (d->m_fontSettingsNeedsApply)
        applyFontSettings();
}

void TextDocument::setCompletionAssistProvider(CompletionAssistProvider *provider)
{
    d->m_completionAssistProvider = provider;
}

CompletionAssistProvider *TextDocument::completionAssistProvider() const
{
    return d->m_completionAssistProvider;
}

void TextDocument::setFunctionHintAssistProvider(CompletionAssistProvider *provider)
{
    d->m_functionHintAssistProvider = provider;
}

CompletionAssistProvider *TextDocument::functionHintAssistProvider() const
{
    return d->m_functionHintAssistProvider;
}

void TextDocument::setQuickFixAssistProvider(IAssistProvider *provider) const
{
    d->m_quickFixProvider = provider;
}

IAssistProvider *TextDocument::quickFixAssistProvider() const
{
    return d->m_quickFixProvider;
}

void TextDocument::applyFontSettings()
{
    d->m_fontSettingsNeedsApply = false;
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        TextDocumentLayout::updateSuggestionFormats(block, fontSettings());
        block = block.next();
    }
    updateLayout();
    if (d->m_highlighter) {
        d->m_highlighter->setFontSettings(d->m_fontSettings);
        d->m_highlighter->rehighlight();
    }
}

const FontSettings &TextDocument::fontSettings() const
{
    return d->m_fontSettings;
}

void TextDocument::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_extraEncodingSettings = extraEncodingSettings;
}

void TextDocument::autoIndent(const QTextCursor &cursor, QChar typedChar, int currentCursorPosition)
{
    d->m_indenter->indent(cursor, typedChar, tabSettings(), currentCursorPosition);
}

void TextDocument::autoReindent(const QTextCursor &cursor, int currentCursorPosition)
{
    d->m_indenter->reindent(cursor, tabSettings(), currentCursorPosition);
}

void TextDocument::autoFormatOrIndent(const QTextCursor &cursor)
{
    d->m_indenter->autoIndent(cursor, tabSettings());
}

Utils::MultiTextCursor TextDocument::indent(const Utils::MultiTextCursor &cursor)
{
    return d->indentOrUnindent(cursor, true, tabSettings());
}

Utils::MultiTextCursor TextDocument::unindent(const Utils::MultiTextCursor &cursor)
{
    return d->indentOrUnindent(cursor, false, tabSettings());
}

void TextDocument::setFormatter(Formatter *formatter)
{
    d->m_formatter.reset(formatter);
}

void TextDocument::autoFormat(const QTextCursor &cursor)
{
    using namespace Utils::Text;
    if (!d->m_formatter)
        return;
    if (QFutureWatcher<ChangeSet> *watcher = d->m_formatter->format(cursor, tabSettings())) {
        connect(watcher, &QFutureWatcher<ChangeSet>::finished, this, [this, watcher]() {
            if (!watcher->isCanceled())
                applyChangeSet(watcher->result());
            delete watcher;
        });
    }
}

bool TextDocument::applyChangeSet(const ChangeSet &changeSet)
{
    if (changeSet.isEmpty())
        return true;
    RefactoringChanges changes;
    const RefactoringFilePtr file = changes.file(filePath());
    file->setChangeSet(changeSet);
    return file->apply();
}

// the blocks list must be sorted
void TextDocument::setIfdefedOutBlocks(const QList<BlockRange> &blocks)
{
    QTextDocument *doc = document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    bool needUpdate = false;

    QTextBlock block = doc->firstBlock();

    int rangeNumber = 0;
    int braceDepthDelta = 0;
    while (block.isValid()) {
        bool cleared = false;
        bool set = false;
        if (rangeNumber < blocks.size()) {
            const BlockRange &range = blocks.at(rangeNumber);
            if (block.position() >= range.first()
                && ((block.position() + block.length() - 1) <= range.last() || !range.last()))
                set = TextDocumentLayout::setIfdefedOut(block);
            else
                cleared = TextDocumentLayout::clearIfdefedOut(block);
            if (block.contains(range.last()))
                ++rangeNumber;
        } else {
            cleared = TextDocumentLayout::clearIfdefedOut(block);
        }

        if (cleared || set) {
            needUpdate = true;
            int delta = TextDocumentLayout::braceDepthDelta(block);
            if (cleared)
                braceDepthDelta += delta;
            else if (set)
                braceDepthDelta -= delta;
        }

        if (braceDepthDelta) {
            TextDocumentLayout::changeBraceDepth(block,braceDepthDelta);
            TextDocumentLayout::changeFoldingIndent(block, braceDepthDelta); // ### C++ only, refactor!
        }

        block = block.next();
    }

    if (needUpdate)
        documentLayout->requestUpdate();

#ifdef WITH_TESTS
    emit ifdefedOutBlocksChanged(blocks);
#endif
}

const ExtraEncodingSettings &TextDocument::extraEncodingSettings() const
{
    return d->m_extraEncodingSettings;
}

void TextDocument::setIndenter(Indenter *indenter)
{
    // clear out existing code formatter data
    for (QTextBlock it = document()->begin(); it.isValid(); it = it.next()) {
        TextBlockUserData *userData = TextDocumentLayout::textUserData(it);
        if (userData)
            userData->setCodeFormatterData(nullptr);
    }
    d->m_indenter.reset(indenter);
}

Indenter *TextDocument::indenter() const
{
    return d->m_indenter.data();
}

bool TextDocument::isSaveAsAllowed() const
{
    return true;
}

FilePath TextDocument::fallbackSaveAsPath() const
{
    return d->m_defaultPath;
}

QString TextDocument::fallbackSaveAsFileName() const
{
    return d->m_suggestedFileName;
}

void TextDocument::setFallbackSaveAsPath(const FilePath &defaultPath)
{
    d->m_defaultPath = defaultPath;
}

void TextDocument::setFallbackSaveAsFileName(const QString &suggestedFileName)
{
    d->m_suggestedFileName = suggestedFileName;
}

QTextDocument *TextDocument::document() const
{
    return &d->m_document;
}

SyntaxHighlighter *TextDocument::syntaxHighlighter() const
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
bool TextDocument::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    QTextCursor cursor(&d->m_document);

    // When autosaving, we don't want to modify the document/location under the user's fingers.
    TextEditorWidget *editorWidget = nullptr;
    int savedPosition = 0;
    int savedAnchor = 0;
    int savedVScrollBarValue = 0;
    int savedHScrollBarValue = 0;
    int undos = d->m_document.availableUndoSteps();

    // When saving the current editor, make sure to maintain the cursor and scroll bar
    // positions for undo
    if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor()) {
        if (editor->document() == this) {
            editorWidget = editor->editorWidget();
            QTextCursor cur = editor->textCursor();
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

        if (d->m_storageSettings.m_cleanWhitespace) {
            cleanWhitespace(cursor,
                            d->m_storageSettings.m_inEntireDocument,
                            d->m_storageSettings.m_cleanIndentation);
        }
        if (d->m_storageSettings.m_addFinalNewLine)
          ensureFinalNewLine(cursor);
        cursor.endEditBlock();
    }

    // check if UTF8-BOM has to be added or removed
    Utils::TextFileFormat saveFormat = format();
    if (saveFormat.codec->name() == "UTF-8" && supportsUtf8Bom()) {
        switch (d->m_extraEncodingSettings.m_utf8BomSetting) {
        case ExtraEncodingSettings::AlwaysAdd:
            saveFormat.hasUtf8Bom = true;
            break;
        case ExtraEncodingSettings::OnlyKeep:
            break;
        case ExtraEncodingSettings::AlwaysDelete:
            saveFormat.hasUtf8Bom = false;
            break;
        }
    }

    const bool ok = write(filePath, saveFormat, plainText(), errorString);

    // restore text cursor and scroll bar positions
    if (autoSave && undos < d->m_document.availableUndoSteps()) {
        d->m_document.undo();
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
    d->m_autoSaveRevision = d->m_document.revision();
    if (autoSave)
        return true;

    // inform about the new filename
    d->m_document.setModified(false); // also triggers update of the block revisions
    setFilePath(filePath.absoluteFilePath());
    emit changed();
    return true;
}

QByteArray TextDocument::contents() const
{
    return plainText().toUtf8();
}

bool TextDocument::setContents(const QByteArray &contents)
{
    return setPlainText(QString::fromUtf8(contents));
}

void TextDocument::formatContents()
{
    d->m_indenter->format({{document()->firstBlock().blockNumber() + 1,
                            document()->lastBlock().blockNumber() + 1}});
}

bool TextDocument::shouldAutoSave() const
{
    return d->m_autoSaveRevision != d->m_document.revision();
}

void TextDocument::setFilePath(const Utils::FilePath &newName)
{
    if (newName == filePath())
        return;
    IDocument::setFilePath(newName.absoluteFilePath().cleanPath());
}

IDocument::ReloadBehavior TextDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (d->m_silentReload)
        return IDocument::BehaviorSilent;
    return BaseTextDocument::reloadBehavior(state, type);
}

bool TextDocument::isModified() const
{
    return d->m_document.isModified();
}

Core::IDocument::OpenResult TextDocument::open(QString *errorString,
                                               const Utils::FilePath &filePath,
                                               const Utils::FilePath &realFilePath)
{
    emit aboutToOpen(filePath, realFilePath);
    OpenResult success = openImpl(errorString, filePath, realFilePath, /*reload =*/ false);
    if (success == OpenResult::Success) {
        setMimeType(Utils::mimeTypeForFile(filePath, MimeMatchMode::MatchDefaultAndRemote).name());
        emit openFinishedSuccessfully();
    }
    return success;
}

Core::IDocument::OpenResult TextDocument::openImpl(QString *errorString,
                                                   const Utils::FilePath &filePath,
                                                   const Utils::FilePath &realFilePath,
                                                   bool reload)
{
    QStringList content;

    ReadResult readResult = Utils::TextFileFormat::ReadIOError;

    if (!filePath.isEmpty()) {
        readResult = read(realFilePath, &content, errorString);
        const int chunks = content.size();

        // Don't call setUndoRedoEnabled(true) when reload is true and filenames are different,
        // since it will reset the undo's clear index
        if (!reload || filePath == realFilePath)
            d->m_document.setUndoRedoEnabled(reload);

        QTextCursor c(&d->m_document);
        c.beginEditBlock();
        if (reload) {
            c.select(QTextCursor::Document);
            c.removeSelectedText();
        } else {
            d->m_document.clear();
        }

        if (chunks == 1) {
            c.insertText(content.at(0));
        } else if (chunks > 1) {
            QFutureInterface<void> interface;
            interface.setProgressRange(0, chunks);
            ProgressManager::addTask(interface.future(), Tr::tr("Opening File"),
                                     Constants::TASK_OPEN_FILE);
            interface.reportStarted();

            for (int i = 0; i < chunks; ++i) {
                c.insertText(content.at(i));
                interface.setProgressValue(i + 1);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }

            interface.reportFinished();
        }

        c.endEditBlock();

        // Don't call setUndoRedoEnabled(true) when reload is true and filenames are different,
        // since it will reset the undo's clear index
        if (!reload || filePath == realFilePath)
            d->m_document.setUndoRedoEnabled(true);

        auto documentLayout =
            qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
        QTC_ASSERT(documentLayout, return OpenResult::CannotHandle);
        documentLayout->lastSaveRevision = d->m_autoSaveRevision = d->m_document.revision();
        d->updateRevisions();
        d->m_document.setModified(filePath != realFilePath);
        setFilePath(filePath);
    }
    if (readResult == Utils::TextFileFormat::ReadIOError)
        return OpenResult::ReadError;
    return OpenResult::Success;
}

bool TextDocument::reload(QString *errorString, QTextCodec *codec)
{
    QTC_ASSERT(codec, return false);
    setCodec(codec);
    return reload(errorString);
}

bool TextDocument::reload(QString *errorString)
{
    return reload(errorString, filePath());
}

bool TextDocument::reload(QString *errorString, const FilePath &realFilePath)
{
    emit aboutToReload();
    auto documentLayout =
        qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    if (documentLayout)
        documentLayout->documentAboutToReload(this); // removes text marks non-permanently

    bool success = openImpl(errorString, filePath(), realFilePath, /*reload =*/true)
                   == OpenResult::Success;

    if (documentLayout)
        documentLayout->documentReloaded(this); // re-adds text marks
    emit reloadFinished(success);
    return success;
}

bool TextDocument::setPlainText(const QString &text)
{
    if (text.size() > EditorManager::maxTextFileSize()) {
        document()->setPlainText(TextEditorWidget::msgTextTooLarge(text.size()));
        d->resetRevisions();
        document()->setModified(false);
        return false;
    }
    document()->setPlainText(text);
    d->resetRevisions();
    document()->setModified(false);
    return true;
}

bool TextDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore) {
        if (type != TypeContents)
            return true;

        const bool wasModified = document()->isModified();
        {
            Utils::GuardLocker locker(d->m_modificationChangedGuard);
            // hack to ensure we clean the clear state in QTextDocument
            document()->setModified(false);
            document()->setModified(true);
        }
        if (!wasModified)
            modificationChanged(true);
        return true;
    }
    return reload(errorString);
}

void TextDocument::setSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    if (d->m_highlighter)
        delete d->m_highlighter;
    d->m_highlighter = highlighter;
    d->m_highlighter->setParent(this);
    d->m_highlighter->setDocument(&d->m_document);
}

void TextDocument::cleanWhitespace(const QTextCursor &cursor)
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

void TextDocument::cleanWhitespace(QTextCursor &cursor, bool inEntireDocument,
                                   bool cleanIndentation)
{
    const bool removeTrailingWhitespace = d->m_storageSettings.removeTrailingWhitespace(filePath().fileName());

    auto documentLayout = qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    Q_ASSERT(cursor.visualNavigation() == false);

    QTextBlock block = d->m_document.findBlock(cursor.selectionStart());
    QTextBlock end;
    if (cursor.hasSelection())
        end = d->m_document.findBlock(cursor.selectionEnd()-1).next();

    QVector<QTextBlock> blocks;
    while (block.isValid() && block != end) {
        if (inEntireDocument || block.revision() != documentLayout->lastSaveRevision) {
            blocks.append(block);
        }
        block = block.next();
    }
    if (blocks.isEmpty())
        return;

    const TabSettings currentTabSettings = tabSettings();
    const IndentationForBlock &indentations
        = d->m_indenter->indentationForBlocks(blocks, currentTabSettings);

    for (QTextBlock block : std::as_const(blocks)) {
        QString blockText = block.text();

        if (removeTrailingWhitespace)
            TabSettings::removeTrailingWhitespace(cursor, block);

        const int indent = indentations[block.blockNumber()];
        if (cleanIndentation && !currentTabSettings.isIndentationClean(block, indent)) {
            cursor.setPosition(block.position());
            const int firstNonSpace = TabSettings::firstNonSpace(blockText);
            if (firstNonSpace == blockText.length()) {
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            } else {
                int column = currentTabSettings.columnAt(blockText, firstNonSpace);
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace);
                QString indentationString = currentTabSettings.indentationString(0, column, column - indent, block);
                cursor.insertText(indentationString);
            }
        }
    }
}

void TextDocument::ensureFinalNewLine(QTextCursor& cursor)
{
    if (!d->m_storageSettings.m_addFinalNewLine)
        return;

    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    bool emptyFile = !cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);

    if (!emptyFile && cursor.selectedText().at(0) != QChar::ParagraphSeparator)
    {
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        cursor.insertText(QLatin1String("\n"));
    }
}

void TextDocument::modificationChanged(bool modified)
{
    if (d->m_modificationChangedGuard.isLocked())
        return;
    // we only want to update the block revisions when going back to the saved version,
    // e.g. with undo
    if (!modified)
        d->updateRevisions();
    emit changed();
}

void TextDocument::updateLayout() const
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->requestUpdate();
}

void TextDocument::scheduleUpdateLayout() const
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->scheduleUpdate();
}

TextMarks TextDocument::marks() const
{
    return d->m_marksCache;
}

bool TextDocument::addMark(TextMark *mark)
{
    if (mark->baseTextDocument())
        return false;
    QTC_ASSERT(mark->lineNumber() >= 1, return false);
    int blockNumber = mark->lineNumber() - 1;
    auto documentLayout = qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    QTC_ASSERT(documentLayout, return false);
    QTextBlock block = d->m_document.findBlockByNumber(blockNumber);

    if (block.isValid()) {
        TextBlockUserData *userData = TextDocumentLayout::userData(block);
        userData->addMark(mark);
        d->m_marksCache.append(mark);
        mark->updateLineNumber(blockNumber + 1);
        QTC_CHECK(mark->lineNumber() == blockNumber + 1); // Checks that the base class is called
        mark->updateBlock(block);
        mark->setBaseTextDocument(this);
        if (!mark->isVisible())
            return true;
        // Update document layout
        bool fullUpdate = !documentLayout->hasMarks;
        documentLayout->hasMarks = true;
        if (!documentLayout->hasLocationMarker && mark->isLocationMarker()) {
            documentLayout->hasLocationMarker = true;
            fullUpdate = true;
        }
        if (fullUpdate)
            documentLayout->scheduleUpdate();
        else
            documentLayout->requestExtraAreaUpdate();
        return true;
    }
    return false;
}

TextMarks TextDocument::marksAt(int line) const
{
    QTC_ASSERT(line >= 1, return TextMarks());
    int blockNumber = line - 1;
    QTextBlock block = d->m_document.findBlockByNumber(blockNumber);

    if (block.isValid()) {
        if (TextBlockUserData *userData = TextDocumentLayout::textUserData(block))
            return userData->marks();
    }
    return TextMarks();
}

void TextDocument::removeMarkFromMarksCache(TextMark *mark)
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(d->m_document.documentLayout());
    QTC_ASSERT(documentLayout, return);
    d->m_marksCache.removeAll(mark);

    auto scheduleLayoutUpdate = [documentLayout](){
        // make sure all destructors that may directly or indirectly call this function are
        // completed before updating.
        QMetaObject::invokeMethod(documentLayout, &QPlainTextDocumentLayout::requestUpdate,
                                  Qt::QueuedConnection);
    };

    if (mark->isLocationMarker()) {
        documentLayout->hasLocationMarker = false;
        scheduleLayoutUpdate();
    }

    if (d->m_marksCache.isEmpty()) {
        documentLayout->hasMarks = false;
        scheduleLayoutUpdate();
        return;
    }

    if (!mark->isVisible())
        return;

    documentLayout->requestExtraAreaUpdate();
}

static QSet<Id> &hiddenMarksIds()
{
    static QSet<Id> ids;
    return ids;
}

void TextDocument::temporaryHideMarksAnnotation(const Utils::Id &category)
{
    hiddenMarksIds().insert(category);
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (auto document : documents) {
        if (auto textDocument = qobject_cast<TextDocument*>(document)) {
            const TextMarks marks = textDocument->marks();
            for (const auto mark : marks) {
                if (mark->category().id == category)
                    mark->updateMarker();
            }
        }
    }
}

void TextDocument::showMarksAnnotation(const Utils::Id &category)
{
    hiddenMarksIds().remove(category);
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (auto document : documents) {
        if (auto textDocument = qobject_cast<TextDocument*>(document)) {
            const TextMarks marks = textDocument->marks();
            for (const auto mark : marks) {
                if (mark->category().id == category)
                    mark->updateMarker();
            }
        }
    }
}

bool TextDocument::marksAnnotationHidden(const Utils::Id &category)
{
    return hiddenMarksIds().contains(category);
}

void TextDocument::removeMark(TextMark *mark)
{
    QTextBlock block = d->m_document.findBlockByNumber(mark->lineNumber() - 1);
    if (auto data = static_cast<TextBlockUserData *>(block.userData())) {
        if (!data->removeMark(mark))
            qDebug() << "Could not find mark" << mark << "on line" << mark->lineNumber();
    }

    removeMarkFromMarksCache(mark);
    emit markRemoved(mark);
    mark->setBaseTextDocument(nullptr);
    scheduleUpdateLayout();
}

void TextDocument::updateMark(TextMark *mark)
{
    QTextBlock block = d->m_document.findBlockByNumber(mark->lineNumber() - 1);
    if (block.isValid()) {
        TextBlockUserData *userData = TextDocumentLayout::userData(block);
        // re-evaluate priority
        userData->removeMark(mark);
        userData->addMark(mark);
    }
    scheduleUpdateLayout();
}

void TextDocument::moveMark(TextMark *mark, int previousLine)
{
    QTextBlock block = d->m_document.findBlockByNumber(previousLine - 1);
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(block)) {
        if (!data->removeMark(mark))
            qDebug() << "Could not find mark" << mark << "on line" << previousLine;
    }
    removeMarkFromMarksCache(mark);
    mark->setBaseTextDocument(nullptr);
    addMark(mark);
}

} // namespace TextEditor
