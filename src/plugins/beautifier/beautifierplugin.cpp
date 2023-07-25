// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "beautifierplugin.h"

#include "beautifierconstants.h"
#include "beautifiertr.h"
#include "generalsettings.h"

#include "artisticstyle/artisticstyle.h"
#include "clangformat/clangformat.h"
#include "uncrustify/uncrustify.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/formattexteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>

#include <QMenu>

using namespace TextEditor;

namespace Beautifier::Internal {

bool isAutoFormatApplicable(const Core::IDocument *document,
                            const QList<Utils::MimeType> &allowedMimeTypes)
{
    if (!document)
        return false;

    if (allowedMimeTypes.isEmpty())
        return true;

    const Utils::MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());
    return Utils::anyOf(allowedMimeTypes, [&documentMimeType](const Utils::MimeType &mime) {
        return documentMimeType.inherits(mime.name());
    });
}

class BeautifierPluginPrivate : public QObject
{
public:
    BeautifierPluginPrivate();

    void updateActions(Core::IEditor *editor = nullptr);

    void autoFormatOnSave(Core::IDocument *document);

    ArtisticStyle artisticStyleBeautifier;
    ClangFormat clangFormatBeautifier;
    Uncrustify uncrustifyBeautifier;
};

static BeautifierPluginPrivate *dd = nullptr;

void BeautifierPlugin::initialize()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("Bea&utifier"));
    menu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
}

void BeautifierPlugin::extensionsInitialized()
{
    dd = new BeautifierPluginPrivate;
}

ExtensionSystem::IPlugin::ShutdownFlag BeautifierPlugin::aboutToShutdown()
{
    delete dd;
    dd = nullptr;
    return SynchronousShutdown;
}

BeautifierPluginPrivate::BeautifierPluginPrivate()
{
    for (BeautifierTool *tool : BeautifierTool::allTools())
        generalSettings().autoFormatTools.addOption(tool->id());

    updateActions();

    const Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &BeautifierPluginPrivate::updateActions);
    connect(editorManager, &Core::EditorManager::aboutToSave,
            this, &BeautifierPluginPrivate::autoFormatOnSave);
}

void BeautifierPluginPrivate::updateActions(Core::IEditor *editor)
{
    for (BeautifierTool *tool : BeautifierTool::allTools())
        tool->updateActions(editor);
}

void BeautifierPluginPrivate::autoFormatOnSave(Core::IDocument *document)
{
    if (!generalSettings().autoFormatOnSave())
        return;

    if (!isAutoFormatApplicable(document, generalSettings().allowedMimeTypes()))
        return;

    // Check if file is contained in the current project (if wished)
    if (generalSettings().autoFormatOnlyCurrentProject()) {
        const ProjectExplorer::Project *pro = ProjectExplorer::ProjectTree::currentProject();
        if (!pro
            || pro->files([document](const ProjectExplorer::Node *n) {
                      return ProjectExplorer::Project::SourceFiles(n)
                             && n->filePath() == document->filePath();
                  })
                   .isEmpty()) {
            return;
        }
    }

    // Find tool to use by id and format file!
    const QString id = generalSettings().autoFormatTools.stringValue();
    const QList<BeautifierTool *> &tools = BeautifierTool::allTools();
    auto tool = std::find_if(std::begin(tools), std::end(tools),
                             [&id](const BeautifierTool *t){return t->id() == id;});
    if (tool != std::end(tools)) {
        if (!(*tool)->isApplicable(document))
            return;
        const TextEditor::Command command = (*tool)->textCommand();
        if (!command.isValid())
            return;
        const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForDocument(document);
        if (editors.isEmpty())
            return;
        if (auto widget = TextEditorWidget::fromEditor(editors.first()))
            TextEditor::formatEditor(widget, command);
    }
}

} // Beautifier::Internal
