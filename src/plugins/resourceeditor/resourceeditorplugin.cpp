/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "resourceeditorplugin.h"

#include "resourceeditorw.h"
#include "resourceeditorconstants.h"
#include "resourcewizard.h"
#include "resourceeditorfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QAction>

using namespace ResourceEditor::Internal;

ResourceEditorPlugin::ResourceEditorPlugin() :
    m_wizard(0),
    m_editor(0),
    m_redoAction(0),
    m_undoAction(0)
{
}

ResourceEditorPlugin::~ResourceEditorPlugin()
{
    removeObject(m_editor);
    removeObject(m_wizard);
}

bool ResourceEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/resourceeditor/ResourceEditor.mimetypes.xml"), errorMessage))
        return false;

    m_editor = new ResourceEditorFactory(this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a Qt Resource file (.qrc) that you can add to a Qt Widget Project."));
    wizardParameters.setDisplayName(tr("Qt Resource file"));
    wizardParameters.setId(QLatin1String("F.Resource"));
    wizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizardParameters.setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));

    m_wizard = new ResourceWizard(wizardParameters, this);
    addObject(m_wizard);

    errorMessage->clear();

    // Register undo and redo
    const Core::Context context(Constants::C_RESOURCEEDITOR);
    m_undoAction = new QAction(tr("&Undo"), this);
    m_redoAction = new QAction(tr("&Redo"), this);
    m_refreshAction = new QAction(tr("Recheck existence of referenced files"), this);
    Core::ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);
    Core::ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);
    Core::ActionManager::registerAction(m_refreshAction, Constants::REFRESH, context);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(onUndo()));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(onRedo()));
    connect(m_refreshAction, SIGNAL(triggered()), this, SLOT(onRefresh()));

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

void ResourceEditorPlugin::onRefresh()
{
    currentEditor()->onRefresh();
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
            Core::EditorManager::currentEditor());
    QTC_ASSERT(focusEditor, return 0);
    return focusEditor;
}

Q_EXPORT_PLUGIN(ResourceEditorPlugin)
