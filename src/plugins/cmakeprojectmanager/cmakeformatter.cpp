// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeformatter.h"
#include "cmakeformattersettings.h"
#include "cmakeprojectconstants.h"

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

#include <QAction>
#include <QVersionNumber>

using namespace TextEditor;

namespace CMakeProjectManager {
namespace Internal {

void CMakeFormatter::updateActions(Core::IEditor *editor)
{
    const bool enabled = editor && CMakeFormatterSettings::instance()->isApplicable(editor->document());
    m_formatFile->setEnabled(enabled);
}

void CMakeFormatter::formatFile()
{
    formatCurrentFile(command());
}

Command CMakeFormatter::command() const
{
    Command command;
    command.setExecutable(CMakeFormatterSettings::instance()->command());
    command.setProcessing(Command::FileProcessing);
    command.addOption("--in-place");
    command.addOption("%file");
    return command;
}

bool CMakeFormatter::isApplicable(const Core::IDocument *document) const
{
    return CMakeFormatterSettings::instance()->isApplicable(document);
}

void CMakeFormatter::initialize()
{
    m_formatFile = new QAction(tr("Format &Current File"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(m_formatFile, Constants::CMAKEFORMATTER_ACTION_ID);
    connect(m_formatFile, &QAction::triggered, this, &CMakeFormatter::formatFile);

    Core::ActionManager::actionContainer(Constants::CMAKEFORMATTER_MENU_ID)->addAction(cmd);

    connect(CMakeFormatterSettings::instance(), &CMakeFormatterSettings::supportedMimeTypesChanged,
            [this] { updateActions(Core::EditorManager::currentEditor()); });
}

} // namespace Internal
} // namespace CMakeProjectManager
