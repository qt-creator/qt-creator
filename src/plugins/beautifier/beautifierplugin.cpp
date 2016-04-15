/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "beautifierplugin.h"

#include "beautifierconstants.h"

#include "artisticstyle/artisticstyle.h"
#include "clangformat/clangformat.h"
#include "uncrustify/uncrustify.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <diffeditor/differ.h>
#include <texteditor/convenience.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QTextBlock>
#include <QtPlugin>

using namespace TextEditor;

namespace Beautifier {
namespace Internal {

FormatTask format(FormatTask task)
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
        Utils::TempFileSaver sourceFile(QDir::tempPath() + QLatin1String("/qtc_beautifier_XXXXXXXX.")
                                        + fi.suffix());
        sourceFile.setAutoRemove(true);
        sourceFile.write(task.sourceData.toUtf8());
        if (!sourceFile.finalize()) {
            task.error = QObject::tr("Cannot create temporary file \"%1\": %2.")
                    .arg(sourceFile.fileName()).arg(sourceFile.errorString());
            return task;
        }

        // Format temporary file
        QProcess process;
        QStringList options = task.command.options();
        options.replaceInStrings(QLatin1String("%file"), sourceFile.fileName());
        process.start(executable, options);
        if (!process.waitForFinished(5000)) {
            process.kill();
            task.error = QObject::tr("Cannot call %1 or some other error occurred. Time out "
                                     "reached while formatting file %2.")
                    .arg(executable).arg(task.filePath);
            return task;
        }
        const QByteArray output = process.readAllStandardError();
        if (!output.isEmpty())
            task.error = executable + QLatin1String(": ") + QString::fromUtf8(output);

        // Read text back
        Utils::FileReader reader;
        if (!reader.fetch(sourceFile.fileName(), QIODevice::Text)) {
            task.error = QObject::tr("Cannot read file \"%1\": %2.")
                    .arg(sourceFile.fileName()).arg(reader.errorString());
            return task;
        }
        task.formattedData = QString::fromUtf8(reader.data());
        return task;
    } break;

    case Command::PipeProcessing: {
        QProcess process;
        QStringList options = task.command.options();
        options.replaceInStrings(QLatin1String("%filename"), QFileInfo(task.filePath).fileName());
        options.replaceInStrings(QLatin1String("%file"), task.filePath);
        process.start(executable, options);
        if (!process.waitForStarted(3000)) {
            task.error = QObject::tr("Cannot call %1 or some other error occurred.")
                    .arg(executable);
            return task;
        }
        process.write(task.sourceData.toUtf8());
        process.closeWriteChannel();
        if (!process.waitForFinished(5000)) {
            process.kill();
            task.error = QObject::tr("Cannot call %1 or some other error occurred. Time out "
                                     "reached while formatting file %2.")
                    .arg(executable).arg(task.filePath);
            return task;
        }
        const QByteArray errorText = process.readAllStandardError();
        if (!errorText.isEmpty()) {
            task.error = QString::fromLatin1("%1: %2").arg(executable)
                    .arg(QString::fromUtf8(errorText));
            return task;
        }

        const bool addsNewline = task.command.pipeAddsNewline();
        const bool returnsCRLF = task.command.returnsCRLF();
        if (addsNewline || returnsCRLF) {
            task.formattedData = QString::fromUtf8(process.readAllStandardOutput());
            if (addsNewline)
                task.formattedData.remove(QRegExp(QLatin1String("(\\r\\n|\\n)$")));
            if (returnsCRLF)
                task.formattedData.replace(QLatin1String("\r\n"), QLatin1String("\n"));
            return task;
        }
        task.formattedData = QString::fromUtf8(process.readAllStandardOutput());
        return task;
    }
    }

    return task;
}

QString sourceData(TextEditorWidget *editor, int startPos, int endPos)
{
    return (startPos < 0)
            ? editor->toPlainText()
            : Convenience::textAt(editor->textCursor(), startPos, (endPos - startPos));
}

bool BeautifierPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    m_tools << new ArtisticStyle::ArtisticStyle(this);
    m_tools << new ClangFormat::ClangFormat(this);
    m_tools << new Uncrustify::Uncrustify(this);

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(QCoreApplication::translate("Beautifier", Constants::OPTION_TR_CATEGORY));
    menu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    foreach (BeautifierAbstractTool *tool, m_tools) {
        tool->initialize();
        const QList<QObject *> autoReleasedObjects = tool->autoReleaseObjects();
        foreach (QObject *object, autoReleasedObjects)
            addAutoReleasedObject(object);
    }

    updateActions();
    return true;
}

void BeautifierPlugin::extensionsInitialized()
{
    if (const Core::EditorManager *editorManager = Core::EditorManager::instance()) {
        connect(editorManager, &Core::EditorManager::currentEditorChanged,
                this, &BeautifierPlugin::updateActions);
    }
}

ExtensionSystem::IPlugin::ShutdownFlag BeautifierPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void BeautifierPlugin::updateActions(Core::IEditor *editor)
{
    foreach (BeautifierAbstractTool *tool, m_tools)
        tool->updateActions(editor);
}

void BeautifierPlugin::formatCurrentFile(const Command &command, int startPos, int endPos)
{
    if (TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget())
        formatEditorAsync(editor, command, startPos, endPos);
}

/**
 * Formats the text of @a editor using @a command. @a startPos and @a endPos specifies the range of
 * the editor's text that will be formatted. If @a startPos is negative the editor's entire text is
 * formatted.
 *
 * @pre @a endPos must be greater than or equal to @a startPos
 */
void BeautifierPlugin::formatEditor(TextEditorWidget *editor, const Command &command, int startPos,
                                    int endPos)
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
void BeautifierPlugin::formatEditorAsync(TextEditorWidget *editor, const Command &command,
                                         int startPos, int endPos)
{
    QTC_ASSERT(startPos <= endPos, return);

    const QString sd = sourceData(editor, startPos, endPos);
    if (sd.isEmpty())
        return;

    QFutureWatcher<FormatTask> *watcher = new QFutureWatcher<FormatTask>;
    const TextDocument *doc = editor->textDocument();
    connect(doc, &TextDocument::contentsChanged, watcher, &QFutureWatcher<FormatTask>::cancel);
    connect(watcher, &QFutureWatcherBase::finished, [this, watcher]() {
        if (watcher->isCanceled())
            showError(tr("File was modified."));
        else
            checkAndApplyTask(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(Utils::runAsync(&format, FormatTask(editor, doc->filePath().toString(), sd,
                                                           command, startPos, endPos)));
}

/**
 * Checks the state of @a task and if the formatting was successful calls updateEditorText() with
 * the respective members of @a task.
 */
void BeautifierPlugin::checkAndApplyTask(const FormatTask &task)
{
    if (!task.error.isEmpty()) {
        showError(task.error);
        return;
    }

    if (task.formattedData.isEmpty()) {
        showError(tr("Could not format file %1.").arg(task.filePath));
        return;
    }

    QPlainTextEdit *textEditor = task.editor;
    if (!textEditor) {
        showError(tr("File %1 was closed.").arg(task.filePath));
        return;
    }

    const QString formattedData = (task.startPos < 0)
            ? task.formattedData
            : QString(textEditor->toPlainText()).replace(
                  task.startPos, (task.endPos - task.startPos), task.formattedData);

    updateEditorText(textEditor, formattedData);
}

/**
 * Sets the text of @a editor to @a text. Instead of replacing the entire text, however, only the
 * actually changed parts are updated while preserving the cursor position, the folded
 * blocks, and the scroll bar position.
 */
void BeautifierPlugin::updateEditorText(QPlainTextEdit *editor, const QString &text)
{
    const QString editorText = editor->toPlainText();
    if (editorText == text)
        return;

    // Calculate diff
    DiffEditor::Differ differ;
    const QList<DiffEditor::Diff> diff = differ.diff(editorText, text);

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
    foreach (const DiffEditor::Diff &d, diff) {
        switch (d.command) {
        case DiffEditor::Diff::Insert:
        {
            // Adjust cursor position if we do work in front of the cursor.
            if (charactersInfrontOfCursor > 0) {
                const int size = d.text.size();
                charactersInfrontOfCursor += size;
                newCursorPos += size;
            }
            // Adjust folded blocks, if a new block is added.
            if (d.text.contains(QLatin1Char('\n'))) {
                const int newLineCount = d.text.count(QLatin1Char('\n'));
                const int number = cursor.blockNumber();
                const int total = foldedBlocks.size();
                for (int i = 0; i < total; ++i) {
                    if (foldedBlocks.at(i) > number)
                        foldedBlocks[i] += newLineCount;
                }
            }
            cursor.insertText(d.text);
        }
            break;
        case DiffEditor::Diff::Delete:
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
            if (d.text.contains(QLatin1Char('\n'))) {
                const int newLineCount = d.text.count(QLatin1Char('\n'));
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
        }
            break;
        case DiffEditor::Diff::Equal:
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
    foreach (const int blockId, foldedBlocks) {
        const QTextBlock block = doc->findBlockByNumber(qMax(0, blockId));
        if (block.isValid())
            TextDocumentLayout::doFoldOrUnfold(block, false);
    }

    editor->document()->setModified(true);
}

void BeautifierPlugin::showError(const QString &error)
{
    Core::MessageManager::write(tr("Error in Beautifier: %1").arg(error.trimmed()));
}

QString BeautifierPlugin::msgCannotGetConfigurationFile(const QString &command)
{
    return tr("Cannot get configuration file for %1.").arg(command);
}

QString BeautifierPlugin::msgFormatCurrentFile()
{
    //: Menu entry
    return tr("Format Current File");
}

QString BeautifierPlugin::msgFormatSelectedText()
{
    //: Menu entry
    return tr("Format Selected Text");
}

QString BeautifierPlugin::msgCommandPromptDialogTitle(const QString &command)
{
    //: File dialog title for path chooser when choosing binary
    return tr("%1 Command").arg(command);
}

} // namespace Internal
} // namespace Beautifier
