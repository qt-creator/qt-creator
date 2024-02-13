// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellbuildconfiguration.h"
#include "haskellconstants.h"
#include "haskelleditorfactory.h"
#include "haskellmanager.h"
#include "haskellproject.h"
#include "haskellrunconfiguration.h"
#include "haskelltr.h"
#include "stackbuildstep.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <texteditor/snippets/snippetprovider.h>

namespace Haskell::Internal {

class HaskellPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Haskell.json")

public:
    void initialize() final
    {
        setupHaskellStackBuildStep();
        setupHaskellBuildConfiguration();

        setupHaskellRunSupport();

        setupHaskellEditor();

        setupHaskellProject();

        TextEditor::SnippetProvider::registerGroup(Constants::C_HASKELLSNIPPETSGROUP_ID,
                                                   Tr::tr("Haskell", "SnippetProvider"));

        setupHaskellActions(this);

        ProjectExplorer::JsonWizardFactory::addWizardPath(":/haskell/share/wizards/");
    }
};

} // Haskell::Internal

#include "haskellplugin.moc"
