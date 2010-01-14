/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlprojectplugin.h"
#include "qmlprojectmanager.h"
#include "qmlprojectwizard.h"
#include "qmlnewprojectwizard.h"
#include "qmlprojectconstants.h"
#include "qmlprojectfileseditor.h"
#include "qmlproject.h"
#include "qmltaskmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <texteditor/texteditoractionhandler.h>

#include <projectexplorer/taskwindow.h>
#include <qmleditor/qmlmodelmanagerinterface.h>

#include <QtCore/QtPlugin>

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;

QmlProjectPlugin::QmlProjectPlugin() :
        m_projectFilesEditorFactory(0),
        m_qmlTaskManager(0)
{ }

QmlProjectPlugin::~QmlProjectPlugin()
{
    removeObject(m_projectFilesEditorFactory);
    delete m_projectFilesEditorFactory;
}

bool QmlProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    using namespace Core;

    ICore *core = ICore::instance();
    Core::MimeDatabase *mimeDB = core->mimeDatabase();

    const QLatin1String mimetypesXml(":qmlproject/QmlProject.mimetypes.xml");

    if (! mimeDB->addMimeTypes(mimetypesXml, errorMessage))
        return false;

    Manager *manager = new Manager;

    TextEditor::TextEditorActionHandler *actionHandler =
            new TextEditor::TextEditorActionHandler(Constants::C_FILESEDITOR);

    m_projectFilesEditorFactory = new ProjectFilesFactory(manager, actionHandler);
    addObject(m_projectFilesEditorFactory);

    m_qmlTaskManager = new QmlTaskManager(this);

    addAutoReleasedObject(manager);
    addAutoReleasedObject(new QmlRunConfigurationFactory);
    addAutoReleasedObject(new QmlRunControlFactory);
    addAutoReleasedObject(new QmlNewProjectWizard);
    addAutoReleasedObject(new QmlProjectWizard);
    return true;
}

void QmlProjectPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    ProjectExplorer::TaskWindow *taskWindow = pluginManager->getObject<ProjectExplorer::TaskWindow>();
    m_qmlTaskManager->setTaskWindow(taskWindow);

    QmlEditor::QmlModelManagerInterface *modelManager = pluginManager->getObject<QmlEditor::QmlModelManagerInterface>();
    Q_ASSERT(modelManager);
    connect(modelManager, SIGNAL(documentUpdated(Qml::QmlDocument::Ptr)),
            m_qmlTaskManager, SLOT(documentUpdated(Qml::QmlDocument::Ptr)));
}

Q_EXPORT_PLUGIN(QmlProjectPlugin)
