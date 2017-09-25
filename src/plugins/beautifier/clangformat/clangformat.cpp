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

// Tested with version 3.3, 3.4 and 3.4.1

#include "clangformat.h"

#include "clangformatconstants.h"
#include "clangformatoptionspage.h"
#include "clangformatsettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QMenu>
#include <QTextBlock>

namespace Beautifier {
namespace Internal {
namespace ClangFormat {

ClangFormat::ClangFormat(BeautifierPlugin *parent) :
    BeautifierAbstractTool(parent),
    m_beautifierPlugin(parent),
    m_settings(new ClangFormatSettings)
{
}

ClangFormat::~ClangFormat()
{
    delete m_settings;
}

QString ClangFormat::id() const
{
    return QLatin1String(Constants::ClangFormat::DISPLAY_NAME);
}

bool ClangFormat::initialize()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::ClangFormat::MENU_ID);
    menu->menu()->setTitle(tr("&ClangFormat"));

    m_formatFile = new QAction(BeautifierPlugin::msgFormatCurrentFile(), this);
    Core::Command *cmd
            = Core::ActionManager::registerAction(m_formatFile,
                                                  Constants::ClangFormat::ACTION_FORMATFILE);
    menu->addAction(cmd);
    connect(m_formatFile, &QAction::triggered, this, &ClangFormat::formatFile);

    m_formatRange = new QAction(BeautifierPlugin::msgFormatAtCursor(), this);
    cmd = Core::ActionManager::registerAction(m_formatRange,
                                              Constants::ClangFormat::ACTION_FORMATATCURSOR);
    menu->addAction(cmd);
    connect(m_formatRange, &QAction::triggered, this, &ClangFormat::formatAtCursor);

    m_disableFormattingSelectedText
        = new QAction(BeautifierPlugin::msgDisableFormattingSelectedText(), this);
    cmd = Core::ActionManager::registerAction(
        m_disableFormattingSelectedText, Constants::ClangFormat::ACTION_DISABLEFORMATTINGSELECTED);
    menu->addAction(cmd);
    connect(m_disableFormattingSelectedText, &QAction::triggered,
            this, &ClangFormat::disableFormattingSelectedText);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    connect(m_settings, &ClangFormatSettings::supportedMimeTypesChanged,
            [this] { updateActions(Core::EditorManager::currentEditor()); });

    return true;
}

void ClangFormat::updateActions(Core::IEditor *editor)
{
    const bool enabled = editor && m_settings->isApplicable(editor->document());
    m_formatFile->setEnabled(enabled);
    m_formatRange->setEnabled(enabled);
}

QList<QObject *> ClangFormat::autoReleaseObjects()
{
    return {new ClangFormatOptionsPage(m_settings, this)};
}

void ClangFormat::formatFile()
{
    m_beautifierPlugin->formatCurrentFile(command());
}

void ClangFormat::formatAtCursor()
{
    const TextEditor::TextEditorWidget *widget
            = TextEditor::TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCursor tc = widget->textCursor();
    if (tc.hasSelection()) {
        const int offset = tc.selectionStart();
        const int length = tc.selectionEnd() - offset;
        m_beautifierPlugin->formatCurrentFile(command(offset, length));
    } else {
        // Pretend that the current line was selected.
        // Note that clang-format will extend the range to the next bigger
        // syntactic construct if needed.
        const QTextBlock block = tc.block();
        const int offset = block.position();
        const int length = block.length();
        m_beautifierPlugin->formatCurrentFile(command(offset, length));
    }
}

void ClangFormat::disableFormattingSelectedText()
{
    TextEditor::TextEditorWidget *widget = TextEditor::TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCursor tc = widget->textCursor();
    if (!tc.hasSelection())
        return;

    // Insert start marker
    const QTextBlock selectionStartBlock = tc.document()->findBlock(tc.selectionStart());
    QTextCursor insertCursor(tc.document());
    insertCursor.beginEditBlock();
    insertCursor.setPosition(selectionStartBlock.position());
    insertCursor.insertText("// clang-format off\n");
    const int positionToRestore = tc.position();

    // Insert end marker
    QTextBlock selectionEndBlock = tc.document()->findBlock(tc.selectionEnd());
    insertCursor.setPosition(selectionEndBlock.position() + selectionEndBlock.length() - 1);
    insertCursor.insertText("\n// clang-format on");
    insertCursor.endEditBlock();

    // Reset the cursor position in order to clear the selection.
    QTextCursor restoreCursor(tc.document());
    restoreCursor.setPosition(positionToRestore);
    widget->setTextCursor(restoreCursor);

    // The indentation of these markers might be undesired, so reformat.
    // This is not optimal because two undo steps will be needed to remove the markers.
    const int reformatTextLength = insertCursor.position() - selectionStartBlock.position();
    m_beautifierPlugin->formatCurrentFile(command(selectionStartBlock.position(),
                                                  reformatTextLength));
}

Command ClangFormat::command() const
{
    Command command;
    command.setExecutable(m_settings->command());
    command.setProcessing(Command::PipeProcessing);

    if (m_settings->usePredefinedStyle()) {
        const QString predefinedStyle = m_settings->predefinedStyle();
        command.addOption("-style=" + predefinedStyle);
        if (predefinedStyle == "File") {
            const QString fallbackStyle = m_settings->fallbackStyle();
            if (fallbackStyle != "Default")
                command.addOption("-fallback-style=" + fallbackStyle);
        }

        command.addOption("-assume-filename=%file");
    } else {
        command.addOption("-style=file");
        const QString path =
                QFileInfo(m_settings->styleFileName(m_settings->customStyle())).absolutePath();
        command.addOption("-assume-filename=" + path + QDir::separator() + "%filename");
    }

    return command;
}

bool ClangFormat::isApplicable(const Core::IDocument *document) const
{
    return m_settings->isApplicable(document);
}

Command ClangFormat::command(int offset, int length) const
{
    Command c = command();
    c.addOption("-offset=" + QString::number(offset));
    c.addOption("-length=" + QString::number(length));
    return c;
}

} // namespace ClangFormat
} // namespace Internal
} // namespace Beautifier
