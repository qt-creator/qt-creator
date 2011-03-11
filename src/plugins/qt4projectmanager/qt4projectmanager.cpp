/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4projectmanager.h"

#include "qt4projectmanagerconstants.h"
#include "qt4projectmanagerplugin.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "qt4target.h"
#include "profilereader.h"
#include "qmakestep.h"
#include "qt4buildconfiguration.h"
#include "wizards/qtquickapp.h"
#include "wizards/html5app.h"

#include <coreplugin/icore.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/variablemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::BuildStep;
using ProjectExplorer::FileType;
using ProjectExplorer::HeaderType;
using ProjectExplorer::SourceType;
using ProjectExplorer::FormType;
using ProjectExplorer::ResourceType;
using ProjectExplorer::UnknownFileType;

static const char * const kInstallBins = "CurrentProject:QT_INSTALL_BINS";

// Known file types of a Qt 4 project
static const char* qt4FileTypes[] = {
    "CppHeaderFiles",
    "CppSourceFiles",
    "Qt4FormFiles",
    "Qt4ResourceFiles"
};

// Test for form editor (loosely coupled)
static inline bool isFormWindowEditor(const QObject *o)
{
    return o && !qstrcmp(o->metaObject()->className(), "Designer::FormWindowEditor");
}

// Return contents of form editor (loosely coupled)
static inline QString formWindowEditorContents(const QObject *editor)
{
    const QVariant contentV = editor->property("contents");
    QTC_ASSERT(contentV.isValid(), return QString(); )
    return contentV.toString();
}

Qt4Manager::Qt4Manager(Qt4ProjectManagerPlugin *plugin)
  : m_plugin(plugin),
    m_contextProject(0),
    m_lastEditor(0),
    m_dirty(false)
{
}

Qt4Manager::~Qt4Manager()
{
}

void Qt4Manager::registerProject(Qt4Project *project)
{
    m_projects.append(project);
}

void Qt4Manager::unregisterProject(Qt4Project *project)
{
    m_projects.removeOne(project);
}

void Qt4Manager::notifyChanged(const QString &name)
{
    foreach (Qt4Project *pro, m_projects)
        pro->notifyChanged(name);
}

void Qt4Manager::init()
{
    connect(Core::EditorManager::instance(), SIGNAL(editorAboutToClose(Core::IEditor*)),
            this, SLOT(editorAboutToClose(Core::IEditor*)));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(editorChanged(Core::IEditor*)));

    Core::VariableManager *vm = Core::VariableManager::instance();
    vm->registerVariable(QLatin1String(kInstallBins),
        tr("Full path to the bin/ install directory of the current project's Qt version."));
    connect(vm, SIGNAL(variableUpdateRequested(QString)),
            this, SLOT(updateVariable(QString)));
}

void Qt4Manager::editorChanged(Core::IEditor *editor)
{
    // Handle old editor
    if (isFormWindowEditor(m_lastEditor)) {
        disconnect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));

        if (m_dirty) {
            const QString contents = formWindowEditorContents(m_lastEditor);
            foreach(Qt4Project *project, m_projects)
                project->rootProjectNode()->updateCodeModelSupportFromEditor(m_lastEditor->file()->fileName(), contents);
            m_dirty = false;
        }
    }

    m_lastEditor = editor;

    // Handle new editor
    if (isFormWindowEditor(m_lastEditor))
        connect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));
}

void Qt4Manager::editorAboutToClose(Core::IEditor *editor)
{
    if (m_lastEditor == editor) {
        // Oh no our editor is going to be closed
        // get the content first
        if (isFormWindowEditor(m_lastEditor)) {
            disconnect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));
            if (m_dirty) {
                const QString contents = formWindowEditorContents(m_lastEditor);
                foreach(Qt4Project *project, m_projects)
                    project->rootProjectNode()->updateCodeModelSupportFromEditor(m_lastEditor->file()->fileName(), contents);
                m_dirty = false;
            }
        }
        m_lastEditor = 0;
    }
}

void Qt4Manager::updateVariable(const QString &variable)
{
    if (variable == QLatin1String(kInstallBins)) {
        Qt4Project *qt4pro = qobject_cast<Qt4Project *>(projectExplorer()->currentProject());
        if (!qt4pro) {
            Core::VariableManager::instance()->remove(QLatin1String(kInstallBins));
            return;
        }
        QString value = qt4pro->activeTarget()->activeBuildConfiguration()
                ->qtVersion()->versionInfo().value(QLatin1String(kInstallBins));
        Core::VariableManager::instance()->insert(QLatin1String(kInstallBins), value);
    }
}

void Qt4Manager::uiEditorContentsChanged()
{
    // cast sender, get filename
    if (!m_dirty && isFormWindowEditor(sender()))
        m_dirty = true;
}

Core::Context Qt4Manager::projectContext() const
{
     return m_plugin->projectContext();
}

Core::Context Qt4Manager::projectLanguage() const
{
    return Core::Context(ProjectExplorer::Constants::LANG_CXX);
}

QString Qt4Manager::mimeType() const
{
    return QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE);
}

static void updateBoilerPlateCodeFiles(const AbstractMobileApp *app, const QString &proFile)
{
    const QList<AbstractGeneratedFileInfo> updates =
            app->fileUpdates(proFile);
    if (!updates.empty()) {
        const QString title = Qt4Manager::tr("Update of Generated Files");
        QStringList fileNames;
        foreach (const AbstractGeneratedFileInfo &info, updates)
            fileNames.append(QDir::toNativeSeparators(info.fileInfo.fileName()));
        const QString message =
                Qt4Manager::tr("The following files are either outdated or have been modified:<br><br>%1"
                               "<br><br>Do you want Qt Creator to update the files? Any changes will be lost.")
                .arg(fileNames.join(QLatin1String(", ")));
        if (QMessageBox::question(0, title, message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QString error;
            if (!app->updateFiles(updates, error))
                QMessageBox::critical(0, title, error);
        }
    }
}

ProjectExplorer::Project *Qt4Manager::openProject(const QString &fileName)
{
    Core::MessageManager *messageManager = Core::ICore::instance()->messageManager();

    // TODO Make all file paths relative & remove this hack
    // We convert the path to an absolute one here because qt4project.cpp
    // && profileevaluator use absolute/canonical file paths all over the place
    // Correct fix would be to remove these calls ...
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    if (canonicalFilePath.isEmpty()) {
        messageManager->printToOutputPane(tr("Failed opening project '%1': Project file does not exist").arg(QDir::toNativeSeparators(canonicalFilePath)));
        return 0;
    }

    foreach (ProjectExplorer::Project *pi, projectExplorer()->session()->projects()) {
        if (canonicalFilePath == pi->file()->fileName()) {
            messageManager->printToOutputPane(tr("Failed opening project '%1': Project already open").arg(QDir::toNativeSeparators(canonicalFilePath)));
            return 0;
        }
    }

    const QtQuickApp qtQuickApp;
    updateBoilerPlateCodeFiles(&qtQuickApp, canonicalFilePath);
    const Html5App html5App;
    updateBoilerPlateCodeFiles(&html5App, canonicalFilePath);

    Qt4Project *pro = new Qt4Project(this, canonicalFilePath);
    return pro;
}

ProjectExplorer::ProjectExplorerPlugin *Qt4Manager::projectExplorer() const
{
    return ProjectExplorer::ProjectExplorerPlugin::instance();
}

ProjectExplorer::Node *Qt4Manager::contextNode() const
{
    return m_contextNode;
}

void Qt4Manager::setContextNode(ProjectExplorer::Node *node)
{
    m_contextNode = node;
}

void Qt4Manager::setContextProject(ProjectExplorer::Project *project)
{
    m_contextProject = project;
}

ProjectExplorer::Project *Qt4Manager::contextProject() const
{
    return m_contextProject;
}

void Qt4Manager::runQMake()
{
    runQMake(projectExplorer()->startupProject(), 0);
}

void Qt4Manager::runQMakeContextMenu()
{
    runQMake(m_contextProject, m_contextNode);
}

void Qt4Manager::runQMake(ProjectExplorer::Project *p, ProjectExplorer::Node *node)
{
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->saveModifiedFiles())
        return;
    Qt4Project *qt4pro = qobject_cast<Qt4Project *>(p);
    QTC_ASSERT(qt4pro, return);

    if (!qt4pro->activeTarget() ||
        !qt4pro->activeTarget()->activeBuildConfiguration())
    return;

    Qt4BuildConfiguration *bc = qt4pro->activeTarget()->activeBuildConfiguration();
    QMakeStep *qs = bc->qmakeStep();

    if (!qs)
        return;
    //found qmakeStep, now use it
    qs->setForced(true);

    if (node != 0 && node != qt4pro->rootProjectNode())
        if (Qt4ProFileNode *profile = qobject_cast<Qt4ProFileNode *>(node))
            bc->setSubNodeBuild(profile);

    projectExplorer()->buildManager()->appendStep(qs);
    bc->setSubNodeBuild(0);
}

void Qt4Manager::buildSubDirContextMenu()
{
    handleSubDirContexMenu(BUILD);
}

void Qt4Manager::cleanSubDirContextMenu()
{
    handleSubDirContexMenu(CLEAN);
}

void Qt4Manager::rebuildSubDirContextMenu()
{
    handleSubDirContexMenu(REBUILD);
}

void Qt4Manager::handleSubDirContexMenu(Qt4Manager::Action action)
{
    Qt4Project *qt4pro = qobject_cast<Qt4Project *>(m_contextProject);
    QTC_ASSERT(qt4pro, return);

    if (!qt4pro->activeTarget() ||
        !qt4pro->activeTarget()->activeBuildConfiguration())
    return;

    Qt4BuildConfiguration *bc = qt4pro->activeTarget()->activeBuildConfiguration();
    if (m_contextNode != 0 && m_contextNode != qt4pro->rootProjectNode())
        if (Qt4ProFileNode *profile = qobject_cast<Qt4ProFileNode *>(m_contextNode))
            bc->setSubNodeBuild(profile);

    if (projectExplorer()->saveModifiedFiles()) {
        if (action == BUILD) {
            projectExplorer()->buildManager()->buildList(bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
        } else if (action == CLEAN) {
            projectExplorer()->buildManager()->buildList(bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
        } else if (action == REBUILD) {
            QList<ProjectExplorer::BuildStepList *> stepLists;
            stepLists << bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
            stepLists << bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
            projectExplorer()->buildManager()->buildLists(stepLists);
        }
    }

    bc->setSubNodeBuild(0);
}

QString Qt4Manager::fileTypeId(ProjectExplorer::FileType type)
{
    switch (type) {
    case HeaderType:
        return QLatin1String(qt4FileTypes[0]);
    case SourceType:
        return QLatin1String(qt4FileTypes[1]);
    case FormType:
        return QLatin1String(qt4FileTypes[2]);
    case ResourceType:
        return QLatin1String(qt4FileTypes[3]);
    case UnknownFileType:
    default:
        break;
    }
    return QString();
}

