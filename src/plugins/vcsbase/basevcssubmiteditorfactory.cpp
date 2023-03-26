// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basevcssubmiteditorfactory.h"

#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <utils/qtcassert.h>

using namespace Core;

namespace VcsBase {

const char SUBMIT[] = "Vcs.Submit";
const char DIFF_SELECTED[] = "Vcs.DiffSelectedFiles";

VcsSubmitEditorFactory::VcsSubmitEditorFactory
        (const VcsBaseSubmitEditorParameters &parameters,
         const EditorCreator &editorCreator,
         VcsBasePluginPrivate *plugin)
{
    setId(parameters.id);
    setDisplayName(QLatin1String(parameters.displayName));
    addMimeType(QLatin1String(parameters.mimeType));

    setEditorCreator([this, editorCreator, parameters] {
        VcsBaseSubmitEditor *editor = editorCreator();
        editor->setParameters(parameters);
        editor->registerActions(&m_undoAction, &m_redoAction, &m_submitAction, &m_diffAction);
        return editor;
    });

    Context context(parameters.id);
    m_undoAction.setText(Tr::tr("&Undo"));
    ActionManager::registerAction(&m_undoAction, Core::Constants::UNDO, context);

    m_redoAction.setText(Tr::tr("&Redo"));
    ActionManager::registerAction(&m_redoAction, Core::Constants::REDO, context);

    QTC_ASSERT(plugin, return);
    m_submitAction.setIcon(VcsBaseSubmitEditor::submitIcon());
    m_submitAction.setText(plugin->commitDisplayName());

    Command *command = ActionManager::registerAction(&m_submitAction, SUBMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    QObject::connect(&m_submitAction, &QAction::triggered, plugin, &VcsBasePluginPrivate::commitFromEditor);

    m_diffAction.setIcon(VcsBaseSubmitEditor::diffIcon());
    m_diffAction.setText(Tr::tr("Diff &Selected Files"));
    ActionManager::registerAction(&m_diffAction, DIFF_SELECTED, context);
}

VcsSubmitEditorFactory::~VcsSubmitEditorFactory() = default;

} // namespace VcsBase
