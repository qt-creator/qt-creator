// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorerconstants.h"
#include "compilerexplorereditor.h"
#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/iplugin.h>

#include <utils/fsengine/fileiconprovider.h>

using namespace Core;
using namespace Utils;

namespace CompilerExplorer::Internal {

class CompilerExplorerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilerExplorer.json")

public:
    void initialize() final
    {
        setupCompilerExplorerEditor();

        FileIconProvider::registerIconForMimeType(QIcon(":/compilerexplorer/logos/ce.ico"),
                                                  "application/compiler-explorer");

        const Id menuId = "Tools.CompilerExplorer";
        MenuBuilder(menuId)
            .setTitle(Tr::tr("Compiler Explorer"))
            .addToContainer(Core::Constants::M_TOOLS);

        ActionBuilder(this, "CompilerExplorer.CompilerExplorerAction")
            .setText(Tr::tr("Open Compiler Explorer"))
            .addToContainer(menuId)
            .addOnTriggered(this, [] {
                QString name = "Compiler Explorer $";
                EditorManager::openEditorWithContents(Constants::CE_EDITOR_ID,
                                                      &name,
                                                      settings().defaultDocument().toUtf8());
            });
    }
};

} // namespace CompilerExplorer::Internal

#include "compilerexplorerplugin.moc"
