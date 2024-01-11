// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslcompletionassist.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glsleditortr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <texteditor/texteditorconstants.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/mimeconstants.h>

#include <QMenu>

using namespace Core;
using namespace Utils;

namespace GlslEditor::Internal {

class GlslEditorPluginPrivate
{
public:
    GlslCompletionAssistProvider completionAssistProvider;
};

static GlslEditorPluginPrivate *dd = nullptr;

class GlslEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GLSLEditor.json")

public:
    ~GlslEditorPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new GlslEditorPluginPrivate;

        setupGlslEditorFactory();

        ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
        ActionContainer *glslToolsMenu = ActionManager::createMenu(Id(Constants::M_TOOLS_GLSL));
        glslToolsMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
        QMenu *menu = glslToolsMenu->menu();
        //: GLSL sub-menu in the Tools menu
        menu->setTitle(Tr::tr("GLSL"));
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

        // Insert marker for "Refactoring" menu:
        Command *sep = contextMenu->addSeparator();
        sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
        contextMenu->addSeparator();

        Command *cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
        contextMenu->addAction(cmd);
    }

    void extensionsInitialized() final
    {
        using namespace Utils::Constants;
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_VERT_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_FRAG_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_ES_VERT_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_ES_FRAG_MIMETYPE);
    }
};

} // GlslEditor::Internal

#include "glsleditorplugin.moc"
