/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
#include "profileeditor.h"
#include "qmakestep.h"
#include "qt4buildconfiguration.h"
#include "addlibrarywizard.h"
#include "wizards/qtquickapp.h"
#include "wizards/html5app.h"

#include <coreplugin/icore.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/variablemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <qtsupport/profilereader.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <QFileDialog>
#include <QMessageBox>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::BuildStep;
using ProjectExplorer::FileType;
using ProjectExplorer::HeaderType;
using ProjectExplorer::SourceType;
using ProjectExplorer::FormType;
using ProjectExplorer::ResourceType;
using ProjectExplorer::UnknownFileType;

static const char kInstallBins[] = "CurrentProject:QT_INSTALL_BINS";

// Known file types of a Qt 4 project
static const char *qt4FileTypes[] = {
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
    QTC_ASSERT(contentV.isValid(), return QString());
    return contentV.toString();
}

Qt4Manager::Qt4Manager(Qt4ProjectManagerPlugin *plugin)
  : m_plugin(plugin),
    m_contextNode(0),
    m_contextProject(0),
    m_contextFile(0),
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
    vm->registerVariable(kInstallBins,
        tr("Full path to the bin directory of the current project's Qt version."));
    connect(vm, SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));
}

void Qt4Manager::editorChanged(Core::IEditor *editor)
{
    // Handle old editor
    if (isFormWindowEditor(m_lastEditor)) {
        disconnect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));

        if (m_dirty) {
            const QString contents = formWindowEditorContents(m_lastEditor);
            foreach (Qt4Project *project, m_projects)
                project->rootQt4ProjectNode()->updateCodeModelSupportFromEditor(m_lastEditor->document()->fileName(), contents);
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
                foreach (Qt4Project *project, m_projects)
                    project->rootQt4ProjectNode()->updateCodeModelSupportFromEditor(m_lastEditor->document()->fileName(), contents);
                m_dirty = false;
            }
        }
        m_lastEditor = 0;
    }
}

void Qt4Manager::updateVariable(const QByteArray &variable)
{
    if (variable == kInstallBins) {
        Qt4Project *qt4pro = qobject_cast<Qt4Project *>(ProjectExplorer::ProjectExplorerPlugin::currentProject());
        if (!qt4pro) {
            Core::VariableManager::instance()->remove(kInstallBins);
            return;
        }
        QString value;
        const QtSupport::BaseQtVersion *qtv = 0;
        if (ProjectExplorer::Target *t = qt4pro->activeTarget())
            qtv = QtSupport::QtProfileInformation::qtVersion(t->profile());
        else
            qtv = QtSupport::QtProfileInformation::qtVersion(ProjectExplorer::ProfileManager::instance()->defaultProfile());

        if (qtv)
            value = qtv->versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
        Core::VariableManager::instance()->insert(kInstallBins, value);
    }
}

void Qt4Manager::uiEditorContentsChanged()
{
    // cast sender, get filename
    if (!m_dirty && isFormWindowEditor(sender()))
        m_dirty = true;
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

ProjectExplorer::Project *Qt4Manager::openProject(const QString &fileName, QString *errorString)
{
    // TODO Make all file paths relative & remove this hack
    // We convert the path to an absolute one here because qt4project.cpp
    // && profileevaluator use absolute/canonical file paths all over the place
    // Correct fix would be to remove these calls ...
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    if (canonicalFilePath.isEmpty()) {
        if (errorString)
         *errorString = tr("Failed opening project '%1': Project file does not exist").arg(QDir::toNativeSeparators(fileName));
        return 0;
    }

    foreach (ProjectExplorer::Project *pi, projectExplorer()->session()->projects()) {
        if (canonicalFilePath == pi->document()->fileName()) {
            if (errorString)
                *errorString = tr("Failed opening project '%1': Project already open").arg(QDir::toNativeSeparators(canonicalFilePath));
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

ProjectExplorer::Project *Qt4Manager::contextProject() const
{
    return m_contextProject;
}

void Qt4Manager::setContextProject(ProjectExplorer::Project *project)
{
    m_contextProject = project;
}

ProjectExplorer::FileNode *Qt4Manager::contextFile() const
{
    return m_contextFile;
}

void Qt4Manager::setContextFile(ProjectExplorer::FileNode *file)
{
    m_contextFile = file;
}

void Qt4Manager::addLibrary()
{
    ProFileEditorWidget *editor =
        qobject_cast<ProFileEditorWidget*>(Core::EditorManager::currentEditor()->widget());
    if (editor)
        addLibrary(editor->editorDocument()->fileName(), editor);
}

void Qt4Manager::addLibraryContextMenu()
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode();
    if (qobject_cast<Qt4ProFileNode *>(node))
        addLibrary(node->path());
}

void Qt4Manager::addLibrary(const QString &fileName, ProFileEditorWidget *editor)
{
    AddLibraryWizard wizard(fileName, Core::EditorManager::instance());
    if (wizard.exec() != QDialog::Accepted)
        return;

    TextEditor::BaseTextEditor *editable = 0;
    if (editor) {
        editable = editor->editor();
    } else {
        editable = qobject_cast<TextEditor::BaseTextEditor *>
                (Core::EditorManager::openEditor(fileName, Qt4ProjectManager::Constants::PROFILE_EDITOR_ID));
    }
    if (!editable)
        return;

    const int endOfDoc = editable->position(TextEditor::ITextEditor::EndOfDoc);
    editable->setCursorPosition(endOfDoc);
    QString snippet = wizard.snippet();

    // add extra \n in case the last line is not empty
    int line, column;
    editable->convertPosition(endOfDoc, &line, &column);
    if (!editable->textAt(endOfDoc - column, column).simplified().isEmpty())
        snippet = QLatin1Char('\n') + snippet;

    editable->insert(snippet);
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

    Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(qt4pro->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    //found qmakeStep, now use it
    qs->setForced(true);

    if (node != 0 && node != qt4pro->rootProjectNode())
        if (Qt4ProFileNode *profile = qobject_cast<Qt4ProFileNode *>(node))
            bc->setSubNodeBuild(profile);

    projectExplorer()->buildManager()->appendStep(qs, tr("QMake"));
    bc->setSubNodeBuild(0);
}

void Qt4Manager::buildSubDirContextMenu()
{
    handleSubDirContextMenu(BUILD, false);
}

void Qt4Manager::cleanSubDirContextMenu()
{
    handleSubDirContextMenu(CLEAN, false);
}

void Qt4Manager::rebuildSubDirContextMenu()
{
    handleSubDirContextMenu(REBUILD, false);
}

void Qt4Manager::buildFileContextMenu()
{
    handleSubDirContextMenu(BUILD, true);
}

void Qt4Manager::handleSubDirContextMenu(Qt4Manager::Action action, bool isFileBuild)
{
    Qt4Project *qt4pro = qobject_cast<Qt4Project *>(m_contextProject);
    QTC_ASSERT(qt4pro, return);

    if (!qt4pro->activeTarget() ||
        !qt4pro->activeTarget()->activeBuildConfiguration())
    return;

    if (!m_contextNode || !m_contextFile)
        isFileBuild = false;
    Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(qt4pro->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    if (m_contextNode != 0 && (m_contextNode != qt4pro->rootProjectNode() || isFileBuild))
        if (Qt4ProFileNode *profile = qobject_cast<Qt4ProFileNode *>(m_contextNode))
            bc->setSubNodeBuild(profile);

    if (isFileBuild)
        bc->setFileNodeBuild(m_contextFile);
    if (projectExplorer()->saveModifiedFiles()) {
        const Core::Id buildStep = Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        const Core::Id cleanStep = Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        if (action == BUILD) {
            const QString name = ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(buildStep);
            projectExplorer()->buildManager()->buildList(bc->stepList(buildStep), name);
        } else if (action == CLEAN) {
            const QString name = ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(cleanStep);
            projectExplorer()->buildManager()->buildList(bc->stepList(cleanStep), name);
        } else if (action == REBUILD) {
            QStringList names;
            names << ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(cleanStep)
                  << ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(buildStep);

            QList<ProjectExplorer::BuildStepList *> stepLists;
            stepLists << bc->stepList(cleanStep) << bc->stepList(buildStep);
            projectExplorer()->buildManager()->buildLists(stepLists, names);
        }
    }

    bc->setSubNodeBuild(0);
    bc->setFileNodeBuild(0);
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

