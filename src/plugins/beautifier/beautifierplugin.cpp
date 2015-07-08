/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <utils/QtConcurrentTools>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QTextBlock>
#include <QTimer>
#include <QtPlugin>

using namespace TextEditor;

namespace Beautifier {
namespace Internal {

BeautifierPlugin::BeautifierPlugin() :
    m_asyncFormatMapper(new QSignalMapper)
{
    connect(m_asyncFormatMapper,
            static_cast<void (QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped),
            this, &BeautifierPlugin::formatCurrentFileContinue);
    connect(this, &BeautifierPlugin::pipeError, this, &BeautifierPlugin::showError);
}

BeautifierPlugin::~BeautifierPlugin()
{
    m_asyncFormatMapper->deleteLater();
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
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    foreach (BeautifierAbstractTool *tool, m_tools) {
        tool->initialize();
        const QList<QObject *> autoReleasedObjects = tool->autoReleaseObjects();
        foreach (QObject *object, autoReleasedObjects)
            addAutoReleasedObject(object);
    }

    // The single shot is needed, otherwise the menu will stay disabled even
    // when the submenu's actions get enabled later on.
    QTimer::singleShot(0, this, SLOT(updateActions()));
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

// Use pipeError() instead of calling showError() because this function may run in another thread.
QString BeautifierPlugin::format(const QString &text, const Command &command,
                                 const QString &fileName, bool *timeout)
{
    const QString executable = command.executable();
    if (executable.isEmpty())
        return QString();

    switch (command.processing()) {
    case Command::FileProcessing: {
        // Save text to temporary file
        const QFileInfo fi(fileName);
        Utils::TempFileSaver sourceFile(QDir::tempPath() + QLatin1String("/qtc_beautifier_XXXXXXXX.")
                                        + fi.suffix());
        sourceFile.setAutoRemove(true);
        sourceFile.write(text.toUtf8());
        if (!sourceFile.finalize()) {
            emit pipeError(tr("Cannot create temporary file \"%1\": %2.")
                           .arg(sourceFile.fileName()).arg(sourceFile.errorString()));
            return QString();
        }

        // Format temporary file
        QProcess process;
        QStringList options = command.options();
        options.replaceInStrings(QLatin1String("%file"), sourceFile.fileName());
        process.start(executable, options);
        if (!process.waitForFinished(5000)) {
            if (timeout)
                *timeout = true;
            process.kill();
            emit pipeError(tr("Cannot call %1 or some other error occurred.").arg(executable));
            return QString();
        }
        const QByteArray output = process.readAllStandardError();
        if (!output.isEmpty())
            emit pipeError(executable + QLatin1String(": ") + QString::fromUtf8(output));

        // Read text back
        Utils::FileReader reader;
        if (!reader.fetch(sourceFile.fileName(), QIODevice::Text)) {
            emit pipeError(tr("Cannot read file \"%1\": %2.")
                           .arg(sourceFile.fileName()).arg(reader.errorString()));
            return QString();
        }
        return QString::fromUtf8(reader.data());
    } break;

    case Command::PipeProcessing: {
        QProcess process;
        QStringList options = command.options();
        options.replaceInStrings(QLatin1String("%file"), fileName);
        process.start(executable, options);
        if (!process.waitForStarted(3000)) {
            emit pipeError(tr("Cannot call %1 or some other error occurred.").arg(executable));
            return QString();
        }
        process.write(text.toUtf8());
        process.closeWriteChannel();
        if (!process.waitForFinished(5000)) {
            if (timeout)
                *timeout = true;
            process.kill();
            emit pipeError(tr("Cannot call %1 or some other error occurred.").arg(executable));
            return QString();
        }
        const QByteArray errorText = process.readAllStandardError();
        if (!errorText.isEmpty()) {
            emit pipeError(QString::fromLatin1("%1: %2").arg(executable)
                           .arg(QString::fromUtf8(errorText)));
            return QString();
        }

        const bool addsNewline = command.pipeAddsNewline();
        const bool returnsCRLF = command.returnsCRLF();
        if (addsNewline || returnsCRLF) {
            QString formatted = QString::fromUtf8(process.readAllStandardOutput());
            if (addsNewline)
                formatted.remove(QRegExp(QLatin1String("(\\r\\n|\\n)$")));
            if (returnsCRLF)
                formatted.replace(QLatin1String("\r\n"), QLatin1String("\n"));
            return formatted;
        }
        return QString::fromUtf8(process.readAllStandardOutput());
    }
    }

    return QString();
}

void BeautifierPlugin::formatCurrentFile(const Command &command, int startPos, int endPos)
{
    QTC_ASSERT(startPos <= endPos, return);

    if (TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget()) {
        if (const TextDocument *doc = widget->textDocument()) {
            const QString sourceData = (startPos < 0)
                    ? widget->toPlainText()
                    : Convenience::textAt(widget->textCursor(), startPos, (endPos - startPos));
            if (sourceData.isEmpty())
                return;
            const FormatTask task = FormatTask(widget, doc->filePath().toString(), sourceData,
                                               command, startPos, endPos);

            QFutureWatcher<FormatTask> *watcher = new QFutureWatcher<FormatTask>;
            connect(doc, &TextDocument::contentsChanged,
                    watcher, &QFutureWatcher<FormatTask>::cancel);
            connect(watcher, &QFutureWatcherBase::finished, m_asyncFormatMapper,
                    static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
            m_asyncFormatMapper->setMapping(watcher, watcher);
            watcher->setFuture(QtConcurrent::run(&BeautifierPlugin::formatAsync, this, task));
        }
    }
}

void BeautifierPlugin::formatAsync(QFutureInterface<FormatTask> &future, FormatTask task)
{
    task.formattedData = format(task.sourceData, task.command, task.filePath, &task.timeout);
    future.reportResult(task);
}

void BeautifierPlugin::formatCurrentFileContinue(QObject *watcher)
{
    QFutureWatcher<FormatTask> *futureWatcher = static_cast<QFutureWatcher<FormatTask>*>(watcher);
    if (!futureWatcher) {
        if (watcher)
            watcher->deleteLater();
        return;
    }

    if (futureWatcher->isCanceled()) {
        showError(tr("File was modified."));
        futureWatcher->deleteLater();
        return;
    }

    const FormatTask task = futureWatcher->result();
    futureWatcher->deleteLater();

    if (task.timeout) {
        showError(tr("Time out reached while formatting file %1.").arg(task.filePath));
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

    const QString sourceData = textEditor->toPlainText();
    const QString formattedData = (task.startPos < 0)
            ? task.formattedData
            : QString(sourceData).replace(task.startPos, (task.endPos - task.startPos),
                                          task.formattedData);
    if (sourceData == formattedData)
        return;


    // Since QTextCursor does not work properly with folded blocks, all blocks must be unfolded.
    // To restore the current state at the end, keep track of which block is folded.
    QList<int> foldedBlocks;
    QTextBlock block = textEditor->document()->firstBlock();
    while (block.isValid()) {
        if (const TextBlockUserData *userdata = static_cast<TextBlockUserData *>(block.userData())) {
            if (userdata->folded()) {
                foldedBlocks << block.blockNumber();
                TextDocumentLayout::doFoldOrUnfold(block, true);
            }
        }
        block = block.next();
    }
    textEditor->update();

    // Save the current viewport position of the cursor to ensure the same vertical position after
    // the formatted text has set to the editor.
    int absoluteVerticalCursorOffset = textEditor->cursorRect().y();

    // Calculate diff
    DiffEditor::Differ differ;
    const QList<DiffEditor::Diff> diff = differ.diff(sourceData, formattedData);

    // Update changed lines and keep track of the cursor position
    QTextCursor cursor = textEditor->textCursor();
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
    textEditor->setTextCursor(cursor);

    // Adjust vertical scrollbar
    absoluteVerticalCursorOffset = textEditor->cursorRect().y() - absoluteVerticalCursorOffset;
    const double fontHeight = QFontMetrics(textEditor->document()->defaultFont()).height();
    textEditor->verticalScrollBar()->setValue(textEditor->verticalScrollBar()->value()
                                              + absoluteVerticalCursorOffset / fontHeight);
    // Restore folded blocks
    const QTextDocument *doc = textEditor->document();
    foreach (const int blockId, foldedBlocks) {
        const QTextBlock block = doc->findBlockByNumber(qMax(0, blockId));
        if (block.isValid())
            TextDocumentLayout::doFoldOrUnfold(block, false);
    }

    textEditor->document()->setModified(true);
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
