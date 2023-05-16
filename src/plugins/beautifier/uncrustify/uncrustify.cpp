// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tested with version 0.59 and 0.60

#include "uncrustify.h"

#include "uncrustifyconstants.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"
#include "../beautifiertr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/formattexteditor.h>
#include <texteditor/texteditor.h>

#include <utils/filepath.h>

#include <QAction>
#include <QMenu>
#include <QVersionNumber>

using namespace TextEditor;

namespace Beautifier::Internal {

Uncrustify::Uncrustify()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu("Uncrustify.Menu");
    menu->menu()->setTitle(Tr::tr("&Uncrustify"));

    m_formatFile = new QAction(BeautifierPlugin::msgFormatCurrentFile(), this);
    Core::Command *cmd
            = Core::ActionManager::registerAction(m_formatFile, "Uncrustify.FormatFile");
    menu->addAction(cmd);
    connect(m_formatFile, &QAction::triggered, this, &Uncrustify::formatFile);

    m_formatRange = new QAction(BeautifierPlugin::msgFormatSelectedText(), this);
    cmd = Core::ActionManager::registerAction(m_formatRange, "Uncrustify.FormatSelectedText");
    menu->addAction(cmd);
    connect(m_formatRange, &QAction::triggered, this, &Uncrustify::formatSelectedText);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    connect(&m_settings.supportedMimeTypes, &Utils::BaseAspect::changed,
            this, [this] { updateActions(Core::EditorManager::currentEditor()); });
}

QString Uncrustify::id() const
{
    return QLatin1String(Constants::UNCRUSTIFY_DISPLAY_NAME);
}

void Uncrustify::updateActions(Core::IEditor *editor)
{
    const bool enabled = editor && m_settings.isApplicable(editor->document());
    m_formatFile->setEnabled(enabled);
    m_formatRange->setEnabled(enabled);
}

void Uncrustify::formatFile()
{
    const QString cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        Tr::tr(Constants::UNCRUSTIFY_DISPLAY_NAME)));
    } else {
        formatCurrentFile(command(cfgFileName));
    }
}

void Uncrustify::formatSelectedText()
{
    const QString cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        Tr::tr(Constants::UNCRUSTIFY_DISPLAY_NAME)));
        return;
    }

    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    QTextCursor tc = widget->textCursor();
    if (tc.hasSelection()) {
        // Extend selection to full lines
        const int posSelectionEnd = tc.selectionEnd();
        tc.setPosition(tc.selectionStart());
        tc.movePosition(QTextCursor::StartOfLine);
        const int startPos = tc.position();
        tc.setPosition(posSelectionEnd);
        // Don't extend the selection if the cursor is at the start of the line
        if (tc.positionInBlock() > 0)
            tc.movePosition(QTextCursor::EndOfLine);
        const int endPos = tc.position();
        formatCurrentFile(command(cfgFileName, true), startPos, endPos);
    } else if (m_settings.formatEntireFileFallback()) {
        formatFile();
    }
}

QString Uncrustify::configurationFile() const
{
    if (m_settings.useCustomStyle())
        return m_settings.styleFileName(m_settings.customStyle());

    if (m_settings.useOtherFiles()) {
        if (const ProjectExplorer::Project *project
                = ProjectExplorer::ProjectTree::currentProject()) {
            const Utils::FilePaths files = project->files(
                [](const ProjectExplorer::Node *n) { return n->filePath().endsWith("cfg"); });
            for (const Utils::FilePath &file : files) {
                const QFileInfo fi = file.toFileInfo();
                if (fi.isReadable() && fi.fileName() == "uncrustify.cfg")
                    return file.toString();
            }
        }
    }

    if (m_settings.useSpecificConfigFile()) {
        const Utils::FilePath file = m_settings.specificConfigFile();
        if (file.exists())
            return file.toString();
    }

    if (m_settings.useHomeFile()) {
        const QString file = QDir::home().filePath("uncrustify.cfg");
        if (QFile::exists(file))
            return file;
    }

    return QString();
}

Command Uncrustify::command() const
{
    const QString cfgFile = configurationFile();
    return cfgFile.isEmpty() ? Command() : command(cfgFile, false);
}

bool Uncrustify::isApplicable(const Core::IDocument *document) const
{
    return m_settings.isApplicable(document);
}

Command Uncrustify::command(const QString &cfgFile, bool fragment) const
{
    Command command;
    command.setExecutable(m_settings.command());
    command.setProcessing(Command::PipeProcessing);
    if (m_settings.version() >= QVersionNumber(0, 62)) {
        command.addOption("--assume");
        command.addOption("%file");
    } else {
        command.addOption("-l");
        command.addOption("cpp");
    }
    command.addOption("-L");
    command.addOption("1-2");
    if (fragment)
        command.addOption("--frag");
    command.addOption("-c");
    command.addOption(cfgFile);
    return command;
}

} // Beautifier::Internal
