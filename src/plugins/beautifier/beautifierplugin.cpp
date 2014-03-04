/**************************************************************************
**
** Copyright (c) 2014 Lorenz Haas
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
#include <texteditor/basetextdocument.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QTextBlock>
#include <QTimer>
#include <QtPlugin>

namespace Beautifier {
namespace Internal {

BeautifierPlugin::BeautifierPlugin()
{
}

BeautifierPlugin::~BeautifierPlugin()
{
}

bool BeautifierPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    m_tools << new ArtisticStyle::ArtisticStyle(this);
    m_tools << new ClangFormat::ClangFormat(this);
    m_tools << new Uncrustify::Uncrustify(this);

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(QLatin1String("Beautifier"));
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    for (int i = 0, total = m_tools.count(); i < total; ++i) {
        BeautifierAbstractTool *tool = m_tools.at(i);
        tool->initialize();
        const QList<QObject *> autoReleasedObjects = tool->autoReleaseObjects();
        for (int j = 0, total = autoReleasedObjects.count(); j < total; ++j)
            addAutoReleasedObject(autoReleasedObjects.at(j));
    }

    // The single shot is needed, otherwise the menu will stay disabled even
    // when the submenu's actions get enabled later on.
    QTimer::singleShot(0, this, SLOT(updateActions()));
    return true;
}

void BeautifierPlugin::extensionsInitialized()
{
    if (const Core::EditorManager *editorManager = Core::EditorManager::instance()) {
        connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
                this, SLOT(updateActions(Core::IEditor*)));
    }
}

ExtensionSystem::IPlugin::ShutdownFlag BeautifierPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void BeautifierPlugin::updateActions(Core::IEditor *editor)
{
    for (int i = 0, total = m_tools.count(); i < total; ++i)
        m_tools.at(i)->updateActions(editor);
}

QString BeautifierPlugin::format(const QString &text, QStringList command, const QString &fileName)
{
    if (command.isEmpty())
        return QString();

    // Save text to temporary file
    QFileInfo fi(fileName);
    Utils::TempFileSaver sourceFile(QLatin1String("qtc_beautifier_XXXXXXXX.") + fi.suffix());
    sourceFile.setAutoRemove(true);
    sourceFile.write(text.toUtf8());
    if (!sourceFile.finalize()) {
        showError(tr("Cannot create temporary file \"%1\": %2.")
                  .arg(sourceFile.fileName()).arg(sourceFile.errorString()));
        return QString();
    }

    // Format temporary file
    QProcess process;
    command.replaceInStrings(QLatin1String("%file"), sourceFile.fileName());
    const QString processProgram = command.takeFirst();
    process.start(processProgram, command);
    if (!process.waitForFinished()) {
        showError(tr("Cannot call %1 or some other error occurred.").arg(processProgram));
        return QString();
    }
    const QByteArray output = process.readAllStandardError();
    if (!output.isEmpty())
        showError(processProgram + QLatin1String(": ") + QString::fromLocal8Bit(output));

    // Read text back
    Utils::FileReader reader;
    if (!reader.fetch(sourceFile.fileName(), QIODevice::Text)) {
        showError(tr("Cannot read file \"%1\": %2.")
                  .arg(sourceFile.fileName()).arg(reader.errorString()));
        return QString();
    }
    return QString::fromUtf8(reader.data());
}

void BeautifierPlugin::formatCurrentFile(QStringList command)
{
    QPlainTextEdit *textEditor = 0;
    if (TextEditor::BaseTextEditor *editor
            = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::currentEditor()))
        textEditor = qobject_cast<QPlainTextEdit *>(editor->editorWidget());
    if (!textEditor)
        return;

    const QString sourceData = textEditor->toPlainText();
    if (sourceData.isEmpty())
        return;

    const QString formattedData = format(sourceData, command,
                                         Core::EditorManager::currentDocument()->filePath());
    if ((sourceData == formattedData) || formattedData.isEmpty())
        return;

    // Since QTextCursor does not work properly with folded blocks, all blocks must be unfolded.
    // To restore the current state at the end, keep track of which block is folded.
    QList<int> foldedBlocks;
    QTextBlock block = textEditor->document()->firstBlock();
    while (block.isValid()) {
        if (const TextEditor::TextBlockUserData *userdata
                = static_cast<TextEditor::TextBlockUserData *>(block.userData())) {
            if (userdata->folded()) {
                foldedBlocks << block.blockNumber();
                TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(block, true);
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
    const int diffSize = diff.size();
    for (int i = 0; i < diffSize; ++i) {
        const DiffEditor::Diff d = diff.at(i);
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
    const int total = foldedBlocks.size();
    for (int i = 0; i < total; ++i) {
        QTextBlock block = doc->findBlockByNumber(qMax(0, foldedBlocks.at(i)));
        if (block.isValid())
            TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(block, false);
    }

    textEditor->document()->setModified(true);
}

void BeautifierPlugin::showError(const QString &error)
{
    Core::MessageManager::write(tr("ERROR in Beautifier: %1").arg(error.trimmed()));
}

} // namespace Internal
} // namespace Beautifier

Q_EXPORT_PLUGIN(Beautifier::Internal::BeautifierPlugin)
