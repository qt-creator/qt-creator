/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "basevcssubmiteditorfactory.h"

#include "vcsbaseplugin.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <utils/qtcassert.h>

#include <QAction>

using namespace Core;

namespace VcsBase {

const char SUBMIT[] = "Vcs.Submit";
const char DIFF_SELECTED[] = "Vcs.DiffSelectedFiles";

VcsSubmitEditorFactory::VcsSubmitEditorFactory
        (const VcsBaseSubmitEditorParameters *parameters,
         const EditorCreator &editorCreator,
         VcsBasePlugin *plugin)
    : IEditorFactory(plugin), m_editorCreator(editorCreator)
{
    setId(parameters->id);
    setDisplayName(QLatin1String(parameters->displayName));
    addMimeType(parameters->mimeType);

    Context context(parameters->id);
    m_undoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);

    m_redoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);

    QTC_ASSERT(plugin, return);
    m_submitAction = new QAction(VcsBaseSubmitEditor::submitIcon(),
                                 plugin->commitDisplayName(), this);
    Command *command = ActionManager::registerAction(m_submitAction, SUBMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitAction, &QAction::triggered, plugin, &VcsBasePlugin::commitFromEditor);

    m_diffAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_diffAction, DIFF_SELECTED, context);
}

Core::IEditor *VcsSubmitEditorFactory::createEditor()
{
    VcsBaseSubmitEditor *editor = m_editorCreator();
    editor->registerActions(m_undoAction, m_redoAction, m_submitAction, m_diffAction);
    return editor;
}

} // namespace VcsBase
