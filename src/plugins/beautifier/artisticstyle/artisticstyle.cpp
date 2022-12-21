// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tested with version 2.01, 2.02, 2.02.1, 2.03 and 2.04

#include "artisticstyle.h"

#include "artisticstyleconstants.h"
#include "artisticstyleoptionspage.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

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

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QMenu>
#include <QVersionNumber>

using namespace TextEditor;

namespace Beautifier {
namespace Internal {

ArtisticStyle::ArtisticStyle()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu("ArtisticStyle.Menu");
    menu->menu()->setTitle(tr("&Artistic Style"));

    m_formatFile = new QAction(BeautifierPlugin::msgFormatCurrentFile(), this);
    menu->addAction(Core::ActionManager::registerAction(m_formatFile, "ArtisticStyle.FormatFile"));
    connect(m_formatFile, &QAction::triggered, this, &ArtisticStyle::formatFile);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    connect(&m_settings, &ArtisticStyleSettings::supportedMimeTypesChanged,
            this, [this] { updateActions(Core::EditorManager::currentEditor()); });
}

QString ArtisticStyle::id() const
{
    return QLatin1String(Constants::ARTISTICSTYLE_DISPLAY_NAME);
}

void ArtisticStyle::updateActions(Core::IEditor *editor)
{
    m_formatFile->setEnabled(editor && m_settings.isApplicable(editor->document()));
}

void ArtisticStyle::formatFile()
{
    const QString cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        tr(Constants::ARTISTICSTYLE_DISPLAY_NAME)));
    } else {
        formatCurrentFile(command(cfgFileName));
    }
}

QString ArtisticStyle::configurationFile() const
{
    if (m_settings.useCustomStyle())
        return m_settings.styleFileName(m_settings.customStyle());

    if (m_settings.useOtherFiles()) {
        if (const ProjectExplorer::Project *project
                = ProjectExplorer::ProjectTree::currentProject()) {
            const Utils::FilePaths astyleRcfiles = project->files(
                [](const ProjectExplorer::Node *n) { return n->filePath().endsWith(".astylerc"); });
            for (const Utils::FilePath &file : astyleRcfiles) {
                const QFileInfo fi = file.toFileInfo();
                if (fi.isReadable())
                    return file.toString();
            }
        }
    }

    if (m_settings.useSpecificConfigFile()) {
        const Utils::FilePath file = m_settings.specificConfigFile();
        if (file.exists())
            return file.toUserOutput();
    }

    if (m_settings.useHomeFile()) {
        const QDir homeDirectory = QDir::home();
        QString file = homeDirectory.filePath(".astylerc");
        if (QFile::exists(file))
            return file;
        file = homeDirectory.filePath("astylerc");
        if (QFile::exists(file))
            return file;
    }

    return QString();
}

Command ArtisticStyle::command() const
{
    const QString cfgFile = configurationFile();
    return cfgFile.isEmpty() ? Command() : command(cfgFile);
}

bool ArtisticStyle::isApplicable(const Core::IDocument *document) const
{
    return m_settings.isApplicable(document);
}

Command ArtisticStyle::command(const QString &cfgFile) const
{
    Command command;
    command.setExecutable(m_settings.command().toString());
    command.addOption("-q");
    command.addOption("--options=" + cfgFile);

    const QVersionNumber version = m_settings.version();
    if (version > QVersionNumber(2, 3)) {
        command.setProcessing(Command::PipeProcessing);
        if (version == QVersionNumber(2, 4))
            command.setPipeAddsNewline(true);
        command.setReturnsCRLF(Utils::HostOsInfo::isWindowsHost());
        command.addOption("-z2");
    } else {
        command.addOption("%file");
    }

    return command;
}

} // namespace Internal
} // namespace Beautifier
