/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "formattexteditor.h"

#include "textdocument.h"
#include "textdocumentlayout.h"
#include "texteditor.h"

#include <coreplugin/messagemanager.h>

#include <utils/differ.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/temporarydirectory.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFutureWatcher>
#include <QScrollBar>
#include <QTextBlock>

using namespace Utils;

namespace TextEditor {

void formatCurrentFile(const Command &command, int startPos, int endPos)
{
    if (TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget())
        formatEditorAsync(editor, command, startPos, endPos);
}

static QString sourceData(TextEditorWidget *editor, int startPos, int endPos)
{
    return (startPos < 0)
            ? editor->toPlainText()
            : Utils::Text::textAt(editor->textCursor(), startPos, (endPos - startPos));
}

static FormatTask format(FormatTask task)
{
    task.error.clear();
    task.formattedData.clear();

    const QString executable = task.command.executable();
    if (executable.isEmpty())
        return task;

    switch (task.command.processing()) {
    case Command::FileProcessing: {
        // Save text to temporary file
        const QFileInfo fi(task.filePath);
        Utils::TempFileSaver sourceFile(Utils::TemporaryDirectory::masterDirectoryPath()
                                        + "/qtc_beautifier_XXXXXXXX."
                                        + fi.suffix());
        sourceFile.setAutoRemove(true);
        sourceFile.write(task.sourceData.toUtf8());
        if (!sourceFile.finalize()) {
            task.error = QString(QT_TRANSLATE_NOOP("TextEditor",
                                                   "Cannot create temporary file \"%1\": %2."))
                    .arg(sourceFile.fileName(), sourceFile.errorString());
            return task;
        }

        // Format temporary file
        QStringList options = task.command.options();
        options.replaceInStrings(QLatin1String("%file"), sourceFile.fileName());
        Utils::SynchronousProcess process;
        process.setTimeoutS(5);
        Utils::SynchronousProcessResponse response = process.runBlocking({executable, options});
        if (response.result != Utils::SynchronousProcessResponse::Finished) {
            task.error = QString(QT_TRANSLATE_NOOP("TextEditor", "Failed to format: %1."))
                    .arg(response.exitMessage(executable, 5));
            return task;
        }
        const QString output = response.stdErr();
        if (!output.isEmpty())
            task.error = executable + QLatin1String(": ") + output;

        // Read text back
        Utils::FileReader reader;
        if (!reader.fetch(sourceFile.fileName(), QIODevice::Text)) {
            task.error = QString(QT_TRANSLATE_NOOP("TextEditor", "Cannot read file \"%1\": %2."))
                    .arg(sourceFile.fileName(), reader.errorString());
            return task;
        }
        task.formattedData = QString::fromUtf8(reader.data());
    }
    return task;

    case Command::PipeProcessing: {
        QProcess process;
        QStringList options = task.command.options();
        options.replaceInStrings("%filename", QFileInfo(task.filePath).fileName());
        options.replaceInStrings("%file", task.filePath);
        process.start(executable, options);
        if (!process.waitForStarted(3000)) {
            task.error = QString(QT_TRANSLATE_NOOP("TextEditor",
                                                   "Cannot call %1 or some other error occurred."))
                    .arg(executable);
            return task;
        }
        process.write(task.sourceData.toUtf8());
        process.closeWriteChannel();
        if (!process.waitForFinished(5000) && process.state() == QProcess::Running) {
            process.kill();
            task.error = QString(QT_TRANSLATE_NOOP("TextEditor",
                                                   "Cannot call %1 or some other error occurred. Timeout "
                                                   "reached while formatting file %2."))
                    .arg(executable, task.filePath);
            return task;
        }
        const QByteArray errorText = process.readAllStandardError();
        if (!errorText.isEmpty()) {
            task.error = QString::fromLatin1("%1: %2").arg(executable,
                                                           QString::fromUtf8(errorText));
            return task;
        }

        task.formattedData = QString::fromUtf8(process.readAllStandardOutput());

        if (task.command.pipeAddsNewline() && task.formattedData.endsWith('\n')) {
            task.formattedData.chop(1);
            if (task.formattedData.endsWith('\r'))
                task.formattedData.chop(1);
        }
        if (task.command.returnsCRLF())
            task.formattedData.replace("\r\n", "\n");

        return task;
    }
    }

    return task;
}

/**
 * Sets the text of @a editor to @a text. Instead of replacing the entire text, however, only the
 * actually changed parts are updated while preserving the cursor position, the folded
 * blocks, and the scroll bar position.
 */
static void updateEditorText(QPlainTextEdit *editor, const QString &text)
{
    const QString editorText = editor->toPlainText();
    if (editorText == text)
        return;

    // Calculate diff
    Differ differ;
    const QList<Diff> diff = differ.diff(editorText, text);

    // Since QTextCursor does not work properly with folded blocks, all blocks must be unfolded.
    // To restore the current state at the end, keep track of which block is folded.
    QList<int> foldedBlocks;
    QTextBlock block = editor->document()->firstBlock();
    while (block.isValid()) {
        if (const TextBlockUserData *userdata = static_cast<TextBlockUserData *>(block.userData())) {
            if (userdata->folded()) {
                foldedBlocks << block.blockNumber();
                TextDocumentLayout::doFoldOrUnfold(block, true);
            }
        }
        block = block.next();
    }
    editor->update();

    // Save the current viewport position of the cursor to ensure the same vertical position after
    // the formatted text has set to the editor.
    int absoluteVerticalCursorOffset = editor->cursorRect().y();

    // Update changed lines and keep track of the cursor position
    QTextCursor cursor = editor->textCursor();
    int charactersInfrontOfCursor = cursor.position();
    int newCursorPos = charactersInfrontOfCursor;
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    for (const Diff &d : diff) {
        switch (d.command) {
        case Diff::Insert:
        {
            // Adjust cursor position if we do work in front of the cursor.
            if (charactersInfrontOfCursor > 0) {
                const int size = d.text.size();
                charactersInfrontOfCursor += size;
                newCursorPos += size;
            }
            // Adjust folded blocks, if a new block is added.
            if (d.text.contains('\n')) {
                const int newLineCount = d.text.count('\n');
                const int number = cursor.blockNumber();
                const int total = foldedBlocks.size();
                for (int i = 0; i < total; ++i) {
                    if (foldedBlocks.at(i) > number)
                        foldedBlocks[i] += newLineCount;
                }
            }
            cursor.insertText(d.text);
            break;
        }

        case Diff::Delete:
        {
            // Adjust cursor position if we do work in front of the cursor.
            if (charactersInfrontOfCursor > 0) {
                const int size = d.text.size();
                charactersInfrontOfCursor -= size;
                newCursorPos -= size;
                // Cursor was inside the deleted text, so adjust the new cursor position
                if (charactersInfrontOfCursor < 0)
                    newCursorPos -= charactersInfrontOfCursor;
            }
            // Adjust folded blocks, if at least one block is being deleted.
            if (d.text.contains('\n')) {
                const int newLineCount = d.text.count('\n');
                const int number = cursor.blockNumber();
                for (int i = 0, total = foldedBlocks.size(); i < total; ++i) {
                    if (foldedBlocks.at(i) > number) {
                        foldedBlocks[i] -= newLineCount;
                        if (foldedBlocks[i] < number) {
                            foldedBlocks.removeAt(i);
                            --i;
                            --total;
                        }
                    }
                }
            }
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, d.text.size());
            cursor.removeSelectedText();
            break;
        }

        case Diff::Equal:
            // Adjust cursor position
            charactersInfrontOfCursor -= d.text.size();
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, d.text.size());
            break;
        }
    }
    cursor.endEditBlock();
    cursor.setPosition(newCursorPos);
    editor->setTextCursor(cursor);

    // Adjust vertical scrollbar
    absoluteVerticalCursorOffset = editor->cursorRect().y() - absoluteVerticalCursorOffset;
    const double fontHeight = QFontMetrics(editor->document()->defaultFont()).height();
    editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->value()
                                              + absoluteVerticalCursorOffset / fontHeight);
    // Restore folded blocks
    const QTextDocument *doc = editor->document();
    for (int blockId : qAsConst(foldedBlocks)) {
        const QTextBlock block = doc->findBlockByNumber(qMax(0, blockId));
        if (block.isValid())
            TextDocumentLayout::doFoldOrUnfold(block, false);
    }

    editor->document()->setModified(true);
}

static void showError(const QString &error)
{
    Core::MessageManager::write(
                QString(QT_TRANSLATE_NOOP("TextEditor", "Error in text formatting: %1"))
                .arg(error.trimmed()));
}

/**
 * Checks the state of @a task and if the formatting was successful calls updateEditorText() with
 * the respective members of @a task.
 */
static void checkAndApplyTask(const FormatTask &task)
{
    if (!task.error.isEmpty()) {
        showError(task.error);
        return;
    }

    if (task.formattedData.isEmpty()) {
        showError(QString(QT_TRANSLATE_NOOP("TextEditor", "Could not format file %1.")).arg(
                      task.filePath));
        return;
    }

    QPlainTextEdit *textEditor = task.editor;
    if (!textEditor) {
        showError(QString(QT_TRANSLATE_NOOP("TextEditor", "File %1 was closed.")).arg(
                      task.filePath));
        return;
    }

    const QString formattedData = (task.startPos < 0)
            ? task.formattedData
            : QString(textEditor->toPlainText()).replace(
                  task.startPos, (task.endPos - task.startPos), task.formattedData);

    updateEditorText(textEditor, formattedData);
}

/**
 * Formats the text of @a editor using @a command. @a startPos and @a endPos specifies the range of
 * the editor's text that will be formatted. If @a startPos is negative the editor's entire text is
 * formatted.
 *
 * @pre @a endPos must be greater than or equal to @a startPos
 */
void formatEditor(TextEditorWidget *editor, const Command &command, int startPos, int endPos)
{
    QTC_ASSERT(startPos <= endPos, return);

    const QString sd = sourceData(editor, startPos, endPos);
    if (sd.isEmpty())
        return;
    checkAndApplyTask(format(FormatTask(editor, editor->textDocument()->filePath().toString(), sd,
                                        command, startPos, endPos)));
}

/**
 * Behaves like formatEditor except that the formatting is done asynchronously.
 */
void formatEditorAsync(TextEditorWidget *editor, const Command &command, int startPos, int endPos)
{
    QTC_ASSERT(startPos <= endPos, return);

    const QString sd = sourceData(editor, startPos, endPos);
    if (sd.isEmpty())
        return;

    auto watcher = new QFutureWatcher<FormatTask>;
    const TextDocument *doc = editor->textDocument();
    QObject::connect(doc, &TextDocument::contentsChanged, watcher, &QFutureWatcher<FormatTask>::cancel);
    QObject::connect(watcher, &QFutureWatcherBase::finished, [watcher] {
        if (watcher->isCanceled())
            showError(QString(QT_TRANSLATE_NOOP("TextEditor", "File was modified.")));
        else
            checkAndApplyTask(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(Utils::runAsync(&format, FormatTask(editor, doc->filePath().toString(), sd,
                                                           command, startPos, endPos)));
}

} // namespace TextEditor
