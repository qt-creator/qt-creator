/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "resourceeditorplugin.h"

#include "resourceeditorw.h"
#include "resourceeditorconstants.h"
#include "resourcewizard.h"
#include "resourceeditorfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>

#include <QtCore/qplugin.h>
#include <QtGui/QAction>

using namespace ResourceEditor::Internal;

ResourceEditorPlugin::ResourceEditorPlugin() :
    m_wizard(0),
    m_editor(0),
    m_core(NULL),
    m_redoAction(NULL),
    m_undoAction(NULL)
{
}

ResourceEditorPlugin::~ResourceEditorPlugin()
{
    removeObject(m_editor);
    removeObject(m_wizard);
}

bool ResourceEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    if (!m_core->mimeDatabase()->addMimeTypes(QLatin1String(":/resourceeditor/ResourceEditor.mimetypes.xml"), error_message))
        return false;

    m_editor = new ResourceEditorFactory(m_core, this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Resource file"));
    wizardParameters.setName(tr("Resource file"));
    wizardParameters.setCategory(QLatin1String("Qt"));
    wizardParameters.setTrCategory(tr("Qt"));

    m_wizard = new ResourceWizard(wizardParameters, m_core, this);
    addObject(m_wizard);

    error_message->clear();

    // Register undo and redo
    Core::ActionManager * const actionManager = m_core->actionManager();
    int const pluginId = m_core->uniqueIDManager()->uniqueIdentifier(
            Constants::C_RESOURCEEDITOR);
    QList<int> const idList = QList<int>() << pluginId;
    m_undoAction = new QAction(tr("&Undo"), this);
    m_redoAction = new QAction(tr("&Redo"), this);
    actionManager->registerAction(m_undoAction, Core::Constants::UNDO, idList);
    actionManager->registerAction(m_redoAction, Core::Constants::REDO, idList);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(onUndo()));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(onRedo()));

    return true;
}

void ResourceEditorPlugin::extensionsInitialized()
{
}

void ResourceEditorPlugin::onUndo()
{
    currentEditor()->onUndo();
}

void ResourceEditorPlugin::onRedo()
{
    currentEditor()->onRedo();
}

void ResourceEditorPlugin::onUndoStackChanged(ResourceEditorW const *editor,
        bool canUndo, bool canRedo)
{
    if (editor == currentEditor()) {
        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);
    }
}

ResourceEditorW * ResourceEditorPlugin::currentEditor() const
{
    ResourceEditorW * const focusEditor = qobject_cast<ResourceEditorW *>(
            m_core->editorManager()->currentEditor());
    QTC_ASSERT(focusEditor, return 0);
    return focusEditor;
}

Q_EXPORT_PLUGIN(ResourceEditorPlugin)
