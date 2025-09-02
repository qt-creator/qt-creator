// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "refactoringchanges.h"

#include "icodestylepreferencesfactory.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"
#include "texteditorsettings.h"
#include "textindenter.h"

#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QDebug>

#include <memory>

using namespace Core;
using namespace Utils;

namespace TextEditor {

RefactoringFile::RefactoringFile(QTextDocument *document, const FilePath &filePath)
    : m_filePath(filePath)
    , m_document(document)
{ }

RefactoringFile::RefactoringFile(TextEditorWidget *editor)
    : m_filePath(editor->textDocument()->filePath())
    , m_editor(editor)
{ }

RefactoringFile::RefactoringFile(const FilePath &filePath) : m_filePath(filePath)
{
    QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);
    if (!editors.isEmpty()) {
        auto editorWidget = TextEditorWidget::fromEditor(editors.first());
        if (editorWidget && !editorWidget->isReadOnly())
            m_editor = editorWidget;
    }
}

bool RefactoringFile::create(const QString &contents, bool reindent, bool openInEditor)
{
    if (m_filePath.isEmpty() || m_filePath.exists() || m_editor || m_document)
        return false;

    // Create a text document for the new file:
    m_document = new QTextDocument;
    QTextCursor cursor(m_document);
    cursor.beginEditBlock();
    cursor.insertText(contents);

    // Reindent the contents:
    if (reindent) {
        cursor.select(QTextCursor::Document);
        m_formattingCursors = {{cursor, false}};
        doFormatting();
    }
    cursor.endEditBlock();

    // Write the file to disk:
    TextFileFormat format;
    format.setCodecName(EditorManager::defaultTextCodecName());
    const Result<> saveOk = format.writeFile(m_filePath, m_document->toPlainText());
    delete m_document;
    m_document = nullptr;
    if (!saveOk)
        return false;

    fileChanged();

    if (openInEditor)
        openEditor(/*bool activate =*/ false, -1, -1);

    return true;
}

RefactoringFile::~RefactoringFile()
{
    delete m_document;
}

bool RefactoringFile::isValid() const
{
    if (m_filePath.isEmpty())
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
    if (!m_document) {
        QString fileContents;
        if (!m_filePath.isEmpty()) {
            QTextCodec *defaultCodec = EditorManager::defaultTextCodec();
            TextFileFormat::ReadResult result = TextFileFormat::readFile(m_filePath,
                                                                         defaultCodec,
                                                                         &fileContents,
                                                                         &m_textFileFormat);
            if (result.code != TextFileFormat::ReadSuccess) {
                qWarning() << "Could not read " << m_filePath << ". Error: " << result.error;
                m_textFileFormat.setCodec(nullptr);
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
    if (!m_filePath.isEmpty()) {
        if (QTextDocument *doc = mutableDocument())
            return QTextCursor(doc);
    }

    return QTextCursor();
}

FilePath RefactoringFile::filePath() const
{
    return m_filePath;
}

TextEditorWidget *RefactoringFile::editor() const
{
    return m_editor;
}

int RefactoringFile::position(int line, int column) const
{
    QTC_ASSERT(line != 0, return -1);
    QTC_ASSERT(column != 0, return -1);
    if (const QTextDocument *doc = document())
        return doc->findBlockByNumber(line - 1).position() + column - 1;
    return -1;
}

void RefactoringFile::lineAndColumn(int offset, int *line, int *column) const
{
    QTC_ASSERT(line, return);
    QTC_ASSERT(column, return);
    QTC_ASSERT(offset >= 0, return);
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

ChangeSet RefactoringFile::changeSet() const
{
    return m_changes;
}

void RefactoringFile::setChangeSet(const ChangeSet &changeSet)
{
    if (m_filePath.isEmpty())
        return;

    m_changes = changeSet;
    m_formattingCursors.clear();
}

void RefactoringFile::setOpenEditor(bool activate, int pos)
{
    m_openEditor = true;
    m_activateEditor = activate;
    m_editorCursorPosition = pos;
}

bool RefactoringFile::apply()
{
    if (m_changes.isEmpty())
        return true;

    // test file permissions
    if (!m_filePath.isWritableFile()) {
        ReadOnlyFilesDialog roDialog(m_filePath, ICore::dialogParent());
        roDialog.setShowFailWarning(true, Tr::tr("Refactoring cannot be applied."));
        if (roDialog.exec() == ReadOnlyFilesDialog::RO_Cancel)
            return false;
    }

    // open / activate / goto position
    bool ensureCursorVisible = false;
    if (m_openEditor && !m_filePath.isEmpty()) {
        int line = -1, column = -1;
        if (m_editorCursorPosition != -1) {
            lineAndColumn(m_editorCursorPosition, &line, &column);
            ensureCursorVisible = true;
        }
        m_editor = openEditor(m_activateEditor, line, column);
        m_openEditor = false;
        m_activateEditor = false;
        m_editorCursorPosition = -1;
    }

    const bool withUnmodifiedEditor = m_editor && !m_editor->textDocument()->isModified();
    bool result = true;

    // apply changes, if any
    if (!m_changes.isEmpty()) {
        QTextDocument *doc = mutableDocument();
        if (doc) {
            QTextCursor c = cursor();
            if (m_appliedOnce)
                c.joinPreviousEditBlock();
            else
                c.beginEditBlock();

            // apply changes
            setupFormattingRanges(m_changes.operationList());
            m_changes.apply(&c);
            m_changes.clear();

            // Do indentation and formatting.
            doFormatting();

            c.endEditBlock();

            // if this document doesn't have an editor, write the result to a file
            if (!m_editor && m_textFileFormat.codec()) {
                QTC_ASSERT(!m_filePath.isEmpty(), return false);
                // suppress "file has changed" warnings if the file is open in a read-only editor
                Core::FileChangeBlocker block(m_filePath);
                if (const Result<> res = m_textFileFormat.writeFile(m_filePath, doc->toPlainText())) {
                    Core::DocumentManager::notifyFilesChangedInternally({m_filePath});
                } else {
                    qWarning() << "Could not apply changes to" << m_filePath
                               << ". Error: " << res.error();
                    result = false;
                }
            }

            fileChanged();
            if (withUnmodifiedEditor && EditorManager::autoSaveAfterRefactoring())
                DocumentManager::saveDocument(m_editor->textDocument(), m_filePath);
        }
    }

    if (m_editor && ensureCursorVisible)
        m_editor->ensureCursorVisible();

    m_appliedOnce = true;
    return result;
}

bool RefactoringFile::apply(const Utils::ChangeSet &changeSet)
{
    setChangeSet(changeSet);
    return apply();
}

void RefactoringFile::setupFormattingRanges(const QList<ChangeSet::EditOp> &replaceList)
{
    QTextDocument * const doc = m_editor ? m_editor->document() : m_document;
    QTC_ASSERT(doc, return);

    for (const ChangeSet::EditOp &op : replaceList) {
        if (!op.format1())
            continue;
        QTextCursor cursor(doc);
        switch (op.type()) {
        case ChangeSet::EditOp::Replace:
        case ChangeSet::EditOp::Insert:
        case ChangeSet::EditOp::Remove: {
            cursor.setKeepPositionOnInsert(true);
            cursor.setPosition(op.pos1 + op.length1);
            cursor.setPosition(op.pos1, QTextCursor::KeepAnchor);
            const bool advance = op.type() != ChangeSet::EditOp::Remove && !op.text().isEmpty()
                                 && op.text().front() == '\n';
            m_formattingCursors << std::make_pair(cursor, advance);
            break;
        }
        case ChangeSet::EditOp::Flip:
        case ChangeSet::EditOp::Move:
            cursor.setKeepPositionOnInsert(true);
            cursor.setPosition(op.pos1 + op.length1);
            cursor.setPosition(op.pos1, QTextCursor::KeepAnchor);
            m_formattingCursors << std::make_pair(cursor, false);
            cursor.setPosition(op.pos2 + op.length2);
            cursor.setPosition(op.pos2, QTextCursor::KeepAnchor);
            m_formattingCursors << m_formattingCursors << std::make_pair(cursor, false);
            break;
        case ChangeSet::EditOp::Copy:
            cursor.setKeepPositionOnInsert(true);
            cursor.setPosition(op.pos2, QTextCursor::KeepAnchor);
            m_formattingCursors << m_formattingCursors << std::make_pair(cursor, false);
            break;
        }
    }
}

void RefactoringFile::doFormatting()
{
    if (m_formattingCursors.empty())
        return;

    QTextDocument *document = nullptr;
    Indenter *indenter = nullptr;
    std::unique_ptr<Indenter> indenterOwner;
    TabSettings tabSettings;
    if (m_editor) {
        document = m_editor->document();
        indenter = m_editor->textDocument()->indenter();
        tabSettings = m_editor->textDocument()->tabSettings();
    } else {
        document = m_document;
        ICodeStylePreferencesFactory * const factory
            = TextEditorSettings::codeStyleFactory(indenterId());
        indenterOwner.reset(factory ? factory->createIndenter(document)
                                    : new PlainTextIndenter(document));
        indenter = indenterOwner.get();
        indenter->setFileName(filePath());
        tabSettings = TabSettings::settingsForFile(filePath());
    }
    QTC_ASSERT(document, return);
    QTC_ASSERT(indenter, return);

    for (auto &[formattingCursor, advance] : m_formattingCursors) {
        if (advance)
            formattingCursor.setPosition(formattingCursor.position() + 1, QTextCursor::KeepAnchor);
    }
    Utils::sort(m_formattingCursors, [](const auto &tc1, const auto &tc2) {
        return tc1.first.selectionStart() < tc2.first.selectionStart();
    });
    static const QString clangFormatLineRemovalBlocker("");
    for (auto &[formattingCursor, _] : m_formattingCursors) {
        const QTextBlock firstBlock = document->findBlock(formattingCursor.selectionStart());
        const QTextBlock lastBlock = document->findBlock(formattingCursor.selectionEnd());
        QTextBlock b = firstBlock;
        while (true) {
            QTC_ASSERT(b.isValid(), break);
            if (b.text().simplified().isEmpty()) {
                QTextCursor c(b);
                c.movePosition(QTextCursor::EndOfBlock);
                c.insertText(clangFormatLineRemovalBlocker);
            }
            if (b == lastBlock)
                break;
            b = b.next();
        }
    }

    const int firstSelectedBlock
        = document->findBlock(m_formattingCursors.first().first.selectionStart()).blockNumber();
    for (const auto &[tc, _] : std::as_const(m_formattingCursors))
        indenter->autoIndent(tc, tabSettings);

    for (QTextBlock b = document->findBlockByNumber(firstSelectedBlock);
         b.isValid(); b = b.next()) {
        QString blockText = b.text();
        if (blockText.remove(clangFormatLineRemovalBlocker) == b.text())
            continue;
        QTextCursor c(b);
        c.select(QTextCursor::LineUnderCursor);
        c.removeSelectedText();
        c.insertText(blockText);
    }
}

TextEditorWidget *RefactoringFile::openEditor(bool activate, int line, int column)
{
    EditorManager::OpenEditorFlags flags = EditorManager::IgnoreNavigationHistory;
    if (activate)
        flags |= EditorManager::SwitchSplitIfAlreadyVisible;
    else
        flags |= EditorManager::DoNotChangeCurrentEditor;
    if (line != -1) {
        // openEditorAt uses a 1-based line and a 0-based column!
        column -= 1;
    }
    IEditor *editor = EditorManager::openEditorAt(Link{m_filePath, line, column}, Id(), flags);

    return TextEditorWidget::fromEditor(editor);
}

RefactoringFileFactory::~RefactoringFileFactory() = default;

RefactoringFilePtr PlainRefactoringFileFactory::file(const Utils::FilePath &filePath) const
{
    return RefactoringFilePtr(new RefactoringFile(filePath));
}

} // namespace TextEditor
