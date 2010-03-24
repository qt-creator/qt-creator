/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "cmaketarget.h"
#include "makestep.h"
#include "cmakeopenprojectwizard.h"
#include "cmakebuildconfiguration.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QInputDialog>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;
using ProjectExplorer::Environment;
using ProjectExplorer::EnvironmentItem;

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the make we need to call
// What is the actual compiler executable
// DEFINES

// Open Questions
// Who sets up the environment for cl.exe ? INCLUDEPATH and so on

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_rootNode(new CMakeProjectNode(m_fileName)),
      m_insideFileChanged(false),
      m_targetFactory(new CMakeTargetFactory(this))
{
    m_file = new CMakeFile(this, fileName);

    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            SLOT(targetAdded(ProjectExplorer::Target*)));
}

CMakeProject::~CMakeProject()
{
    delete m_rootNode;
}

void CMakeProject::fileChanged(const QString &fileName)
{
    Q_UNUSED(fileName)
    if (!activeTarget() ||
        !activeTarget()->activeBuildConfiguration())
        return;

    if (m_insideFileChanged)
        return;
    m_insideFileChanged = true;
    changeActiveBuildConfiguration(activeTarget()->activeBuildConfiguration());
    m_insideFileChanged = false;
}

void CMakeProject::changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc || bc->target() != activeTarget())
        return;

    CMakeBuildConfiguration * cmakebc(qobject_cast<CMakeBuildConfiguration *>(bc));
    if (!cmakebc)
        return;

    // Pop up a dialog asking the user to rerun cmake
    QFileInfo sourceFileInfo(m_fileName);

    QString cbpFile = CMakeManager::findCbpFile(QDir(bc->buildDirectory()));
    QFileInfo cbpFileFi(cbpFile);
    CMakeOpenProjectWizard::Mode mode = CMakeOpenProjectWizard::Nothing;
    if (!cbpFileFi.exists()) {
        mode = CMakeOpenProjectWizard::NeedToCreate;
    } else {
        foreach(const QString &file, m_watchedFiles) {
            if (QFileInfo(file).lastModified() > cbpFileFi.lastModified()) {
                mode = CMakeOpenProjectWizard::NeedToUpdate;
                break;
            }
        }
    }

    if (mode != CMakeOpenProjectWizard::Nothing) {
        CMakeOpenProjectWizard copw(m_manager,
                                    sourceFileInfo.absolutePath(),
                                    cmakebc->buildDirectory(),
                                    mode,
                                    cmakebc->environment());
        copw.exec();
        cmakebc->setMsvcVersion(copw.msvcVersion());
    }
    // reparse
    parseCMakeLists();
}

void CMakeProject::targetAdded(ProjectExplorer::Target *t)
{
    if (!t)
        return;

    connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            SLOT(changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
}

void CMakeProject::changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory)
{
    bc->setBuildDirectory(newBuildDirectory);
    parseCMakeLists();
}

QString CMakeProject::defaultBuildDirectory() const
{
    return projectDirectory() + QLatin1String("/qtcreator-build");
}

bool CMakeProject::parseCMakeLists()
{
    if (!activeTarget() ||
        !activeTarget()->activeBuildConfiguration())
        return false;

    // Find cbp file
    CMakeBuildConfiguration *activeBC = activeTarget()->activeBuildConfiguration();
    QString cbpFile = CMakeManager::findCbpFile(activeBC->buildDirectory());

    // setFolderName
    m_rootNode->setDisplayName(QFileInfo(cbpFile).completeBaseName());
    CMakeCbpParser cbpparser;
    // Parsing
    //qDebug()<<"Parsing file "<<cbpFile;
    if (!cbpparser.parseCbpFile(cbpFile)) {
        // TODO report error
        qDebug()<<"Parsing failed";
        // activeBC->updateToolChain(QString::null);
        emit buildTargetsChanged();
        return false;
    }

    // ToolChain
    // activeBC->updateToolChain(cbpparser.compilerName());
    m_projectName = cbpparser.projectName();
    m_rootNode->setDisplayName(cbpparser.projectName());

    //qDebug()<<"Building Tree";
    QList<ProjectExplorer::FileNode *> fileList = cbpparser.fileList();
    QSet<QString> projectFiles;
    if (cbpparser.hasCMakeFiles()) {
        fileList.append(cbpparser.cmakeFileList());
        foreach(const ProjectExplorer::FileNode *node, cbpparser.cmakeFileList())
            projectFiles.insert(node->path());
    } else {
        // Manually add the CMakeLists.txt file
        QString cmakeListTxt = projectDirectory() + "/CMakeLists.txt";
        fileList.append(new ProjectExplorer::FileNode(cmakeListTxt, ProjectExplorer::ProjectFileType, false));
        projectFiles.insert(cmakeListTxt);
    }

    QSet<QString> added = projectFiles;
    added.subtract(m_watchedFiles);
    foreach(const QString &add, added)
        m_watcher->addFile(add);
    foreach(const QString &remove, m_watchedFiles.subtract(projectFiles))
        m_watcher->removeFile(remove);
    m_watchedFiles = projectFiles;

    m_files.clear();
    foreach (ProjectExplorer::FileNode *fn, fileList)
        m_files.append(fn->path());
    m_files.sort();

    buildTree(m_rootNode, fileList);

    //qDebug()<<"Adding Targets";
    m_buildTargets = cbpparser.buildTargets();
//        qDebug()<<"Printing targets";
//        foreach(CMakeTarget ct, m_targets) {
//            qDebug()<<ct.title<<" with executable:"<<ct.executable;
//            qDebug()<<"WD:"<<ct.workingDirectory;
//            qDebug()<<ct.makeCommand<<ct.makeCleanCommand;
//            qDebug()<<"";
//        }

    //qDebug()<<"Updating CodeModel";

    QStringList allIncludePaths;
    QStringList allFrameworkPaths;
    QList<ProjectExplorer::HeaderPath> allHeaderPaths = activeBC->toolChain()->systemHeaderPaths();
    foreach (const ProjectExplorer::HeaderPath &headerPath, allHeaderPaths) {
        if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
            allFrameworkPaths.append(headerPath.path());
        else
            allIncludePaths.append(headerPath.path());
    }
    // This explicitly adds -I. to the include paths
    allIncludePaths.append(projectDirectory());

    allIncludePaths.append(cbpparser.includeFiles());
    CppTools::CppModelManagerInterface *modelmanager =
            ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
    if (modelmanager) {
        CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
        if (pinfo.includePaths != allIncludePaths
            || pinfo.sourceFiles != m_files
            || pinfo.defines != activeBC->toolChain()->predefinedMacros()
            || pinfo.frameworkPaths != allFrameworkPaths)  {
            pinfo.includePaths = allIncludePaths;
            // TODO we only want C++ files, not all other stuff that might be in the project
            pinfo.sourceFiles = m_files;
            pinfo.defines = activeBC->toolChain()->predefinedMacros(); // TODO this is to simplistic
            pinfo.frameworkPaths = allFrameworkPaths;
            modelmanager->updateProjectInfo(pinfo);
            modelmanager->updateSourceFiles(pinfo.sourceFiles);
        }
    }

    emit buildTargetsChanged();
    return true;
}

QList<CMakeBuildTarget> CMakeProject::buildTargets() const
{
    return m_buildTargets;
}

QStringList CMakeProject::buildTargetTitles() const
{
    QStringList results;
    foreach (const CMakeBuildTarget &ct, m_buildTargets) {
        if (ct.executable.isEmpty())
            continue;
        if (ct.title.endsWith(QLatin1String("/fast")))
            continue;
        results << ct.title;
    }
    return results;
}

bool CMakeProject::hasBuildTarget(const QString &title) const
{
    foreach (const CMakeBuildTarget &ct, m_buildTargets) {
        if (ct.executable.isEmpty())
            continue;
        if (ct.title.endsWith(QLatin1String("/fast")))
            continue;
        if (ct.title == title)
            return true;
    }
    return false;
}

void CMakeProject::gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list)
{
    foreach(ProjectExplorer::FolderNode *folder, parent->subFolderNodes())
        gatherFileNodes(folder, list);
    foreach(ProjectExplorer::FileNode *file, parent->fileNodes())
        list.append(file);
}

void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> newList)
{
    // Gather old list
    QList<ProjectExplorer::FileNode *> oldList;
    gatherFileNodes(rootNode, oldList);
    qSort(oldList.begin(), oldList.end(), ProjectExplorer::ProjectNode::sortNodesByPath);
    qSort(newList.begin(), newList.end(), ProjectExplorer::ProjectNode::sortNodesByPath);

    // generate added and deleted list
    QList<ProjectExplorer::FileNode *>::const_iterator oldIt  = oldList.constBegin();
    QList<ProjectExplorer::FileNode *>::const_iterator oldEnd = oldList.constEnd();
    QList<ProjectExplorer::FileNode *>::const_iterator newIt  = newList.constBegin();
    QList<ProjectExplorer::FileNode *>::const_iterator newEnd = newList.constEnd();

    QList<ProjectExplorer::FileNode *> added;
    QList<ProjectExplorer::FileNode *> deleted;


    while(oldIt != oldEnd && newIt != newEnd) {
        if ( (*oldIt)->path() == (*newIt)->path()) {
            delete *newIt;
            ++oldIt;
            ++newIt;
        } else if ((*oldIt)->path() < (*newIt)->path()) {
            deleted.append(*oldIt);
            ++oldIt;
        } else {
            added.append(*newIt);
            ++newIt;
        }
    }

    while (oldIt != oldEnd) {
        deleted.append(*oldIt);
        ++oldIt;
    }

    while (newIt != newEnd) {
        added.append(*newIt);
        ++newIt;
    }

    // add added nodes
    foreach (ProjectExplorer::FileNode *fn, added) {
//        qDebug()<<"added"<<fn->path();
        // Get relative path to rootNode
        QString parentDir = QFileInfo(fn->path()).absolutePath();
        ProjectExplorer::FolderNode *folder = findOrCreateFolder(rootNode, parentDir);
        rootNode->addFileNodes(QList<ProjectExplorer::FileNode *>()<< fn, folder);
    }

    // remove old file nodes and check whether folder nodes can be removed
    foreach (ProjectExplorer::FileNode *fn, deleted) {
        ProjectExplorer::FolderNode *parent = fn->parentFolderNode();
//        qDebug()<<"removed"<<fn->path();
        rootNode->removeFileNodes(QList<ProjectExplorer::FileNode *>() << fn, parent);
        // Check for empty parent
        while (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty()) {
            ProjectExplorer::FolderNode *grandparent = parent->parentFolderNode();
            rootNode->removeFolderNodes(QList<ProjectExplorer::FolderNode *>() << parent, grandparent);
            parent = grandparent;
            if (parent == rootNode)
                break;
        }
    }
}

ProjectExplorer::FolderNode *CMakeProject::findOrCreateFolder(CMakeProjectNode *rootNode, QString directory)
{
    QString relativePath = QDir(QFileInfo(rootNode->path()).path()).relativeFilePath(directory);
    QStringList parts = relativePath.split(QLatin1Char('/'), QString::SkipEmptyParts);
    ProjectExplorer::FolderNode *parent = rootNode;
    QString path = QFileInfo(rootNode->path()).path();
    foreach (const QString &part, parts) {
        path += QLatin1Char('/');
        path += part;
        // Find folder in subFolders
        bool found = false;
        foreach (ProjectExplorer::FolderNode *folder, parent->subFolderNodes()) {
            if (folder->path() == path) {
                // yeah found something :)
                parent = folder;
                found = true;
                break;
            }
        }
        if (!found) {
            // No FolderNode yet, so create it
            ProjectExplorer::FolderNode *tmp = new ProjectExplorer::FolderNode(path);
            tmp->setDisplayName(part);
            rootNode->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << tmp, parent);
            parent = tmp;
        }
    }
    return parent;
}

QString CMakeProject::displayName() const
{
    return m_projectName;
}

QString CMakeProject::id() const
{
    return QLatin1String("CMakeProjectManager.CMakeProject");
}

Core::IFile *CMakeProject::file() const
{
    return m_file;
}

CMakeTargetFactory *CMakeProject::targetFactory() const
{
    return m_targetFactory;
}

CMakeManager *CMakeProject::projectManager() const
{
    return m_manager;
}

CMakeTarget *CMakeProject::activeTarget() const
{
    return static_cast<CMakeTarget *>(Project::activeTarget());
}

QList<ProjectExplorer::Project *> CMakeProject::dependsOn()
{
    return QList<Project *>();
}

ProjectExplorer::BuildConfigWidget *CMakeProject::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildConfigWidget*> CMakeProject::subConfigWidgets()
{
    QList<ProjectExplorer::BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

ProjectExplorer::ProjectNode *CMakeProject::rootProjectNode() const
{
    return m_rootNode;
}


QStringList CMakeProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files;
}

bool CMakeProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    bool hasUserFile = activeTarget();
    if (!hasUserFile) {
        CMakeTarget *t = targetFactory()->create(this, QLatin1String(DEFAULT_CMAKE_TARGET_ID));
        Q_ASSERT(t);
        Q_ASSERT(t->activeBuildConfiguration());

        // Ask the user for where he wants to build it
        // and the cmake command line

        CMakeOpenProjectWizard copw(m_manager, projectDirectory(), ProjectExplorer::Environment::systemEnvironment());
        if (copw.exec() != QDialog::Accepted)
            return false;

        CMakeBuildConfiguration *bc =
                static_cast<CMakeBuildConfiguration *>(t->buildConfigurations().at(0));
        bc->setMsvcVersion(copw.msvcVersion());
        if (!copw.buildDirectory().isEmpty())
            bc->setBuildDirectory(copw.buildDirectory());

        addTarget(t);
    } else {
        // We have a user file, but we could still be missing the cbp file
        // or simply run createXml with the saved settings
        QFileInfo sourceFileInfo(m_fileName);
        CMakeBuildConfiguration *activeBC = activeTarget()->activeBuildConfiguration();
        QString cbpFile = CMakeManager::findCbpFile(QDir(activeBC->buildDirectory()));
        QFileInfo cbpFileFi(cbpFile);

        CMakeOpenProjectWizard::Mode mode = CMakeOpenProjectWizard::Nothing;
        if (!cbpFileFi.exists())
            mode = CMakeOpenProjectWizard::NeedToCreate;
        else if (cbpFileFi.lastModified() < sourceFileInfo.lastModified())
            mode = CMakeOpenProjectWizard::NeedToUpdate;

        if (mode != CMakeOpenProjectWizard::Nothing) {
            CMakeOpenProjectWizard copw(m_manager,
                                        sourceFileInfo.absolutePath(),
                                        activeBC->buildDirectory(),
                                        mode,
                                        activeBC->environment());
            if (copw.exec() != QDialog::Accepted)
                return false;
            activeBC->setMsvcVersion(copw.msvcVersion());
        }
    }

    m_watcher = new ProjectExplorer::FileWatcher(this);
    connect(m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));

    if (!parseCMakeLists()) // Gets the directory from the active buildconfiguration
        return false;

    if (!hasUserFile && hasBuildTarget("all")) {
        MakeStep *makeStep = qobject_cast<MakeStep *>(
                activeTarget()->activeBuildConfiguration()->steps(ProjectExplorer::Build).at(0));
        Q_ASSERT(makeStep);
        makeStep->setBuildTarget("all", true);
    }

    foreach (Target *t, targets()) {
        connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
        connect(t, SIGNAL(environmentChanged()),
                this, SLOT(changeEnvironment()));
    }
    return true;
}

CMakeBuildTarget CMakeProject::buildTargetForTitle(const QString &title)
{
    foreach(const CMakeBuildTarget &ct, m_buildTargets)
        if (ct.title == title)
            return ct;
    return CMakeBuildTarget();
}

// CMakeFile

CMakeFile::CMakeFile(CMakeProject *parent, QString fileName)
    : Core::IFile(parent), m_project(parent), m_fileName(fileName)
{

}

bool CMakeFile::save(const QString &fileName)
{
    // Once we have an texteditor open for this file, we probably do
    // need to implement this, don't we.
    Q_UNUSED(fileName)
    return false;
}

QString CMakeFile::fileName() const
{
    return m_fileName;
}

QString CMakeFile::defaultPath() const
{
    return QString();
}

QString CMakeFile::suggestedFileName() const
{
    return QString();
}

QString CMakeFile::mimeType() const
{
    return Constants::CMAKEMIMETYPE;
}


bool CMakeFile::isModified() const
{
    return false;
}

bool CMakeFile::isReadOnly() const
{
    return true;
}

bool CMakeFile::isSaveAsAllowed() const
{
    return false;
}

Core::IFile::ReloadBehavior CMakeFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

void CMakeFile::reload(ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag)
    Q_UNUSED(type)
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeProject *project)
    : m_project(project), m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_pathLineEdit);

    m_changeButton = new QPushButton(this);
    m_changeButton->setText(tr("&Change"));
    connect(m_changeButton, SIGNAL(clicked()), this, SLOT(openChangeBuildDirectoryDialog()));
    hbox->addWidget(m_changeButton);

    fl->addRow("Build directory:", hbox);
}

QString CMakeBuildSettingsWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<CMakeBuildConfiguration *>(bc);
    m_pathLineEdit->setText(m_buildConfiguration->buildDirectory());
    if (m_buildConfiguration->buildDirectory() == m_project->projectDirectory())
        m_changeButton->setEnabled(false);
    else
        m_changeButton->setEnabled(true);
}

void CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog()
{
    CMakeOpenProjectWizard copw(m_project->projectManager(),
                                m_project->projectDirectory(),
                                m_buildConfiguration->buildDirectory(),
                                m_buildConfiguration->environment());
    if (copw.exec() == QDialog::Accepted) {
        m_project->changeBuildDirectory(m_buildConfiguration, copw.buildDirectory());
        m_pathLineEdit->setText(m_buildConfiguration->buildDirectory());
    }
}

/////
// CMakeCbpParser
////

bool CMakeCbpParser::parseCbpFile(const QString &fileName)
{
    QFile fi(fileName);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        setDevice(&fi);

        while (!atEnd()) {
            readNext();
            if (name() == "CodeBlocks_project_file") {
                parseCodeBlocks_project_file();
            } else if (isStartElement()) {
                parseUnknownElement();
            }
        }
        fi.close();
        m_includeFiles.sort();
        m_includeFiles.removeDuplicates();
        return true;
    }
    return false;
}

void CMakeCbpParser::parseCodeBlocks_project_file()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Project") {
            parseProject();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseProject()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Option") {
            parseOption();
        } else if (name() == "Unit") {
            parseUnit();
        } else if (name() == "Build") {
            parseBuild();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuild()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Target") {
            parseBuildTarget();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTarget()
{
    m_buildTargetType = false;
    m_buildTarget.clear();

    if (attributes().hasAttribute("title"))
        m_buildTarget.title = attributes().value("title").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (m_buildTargetType || m_buildTarget.title == "all" || m_buildTarget.title == "install") {
                m_buildTargets.append(m_buildTarget);
            }
            return;
        } else if (name() == "Compiler") {
            parseCompiler();
        } else if (name() == "Option") {
            parseBuildTargetOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTargetOption()
{
    if (attributes().hasAttribute("output"))
        m_buildTarget.executable = attributes().value("output").toString();
    else if (attributes().hasAttribute("type") && (attributes().value("type") == "1" || attributes().value("type") == "0"))
        m_buildTargetType = true;
    else if (attributes().hasAttribute("working_dir"))
        m_buildTarget.workingDirectory = attributes().value("working_dir").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "MakeCommand") {
            parseMakeCommand();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

QString CMakeCbpParser::projectName() const
{
    return m_projectName;
}

void CMakeCbpParser::parseOption()
{
    if (attributes().hasAttribute("title"))
        m_projectName = attributes().value("title").toString();

    if (attributes().hasAttribute("compiler"))
        m_compiler = attributes().value("compiler").toString();

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if(isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseMakeCommand()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Build") {
            parseBuildTargetBuild();
        } else if (name() == "Clean") {
            parseBuildTargetClean();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTargetBuild()
{
    if (attributes().hasAttribute("command"))
        m_buildTarget.makeCommand = attributes().value("command").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTargetClean()
{
    if (attributes().hasAttribute("command"))
        m_buildTarget.makeCleanCommand = attributes().value("command").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseCompiler()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Add") {
            parseAdd();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseAdd()
{
    m_includeFiles.append(attributes().value("directory").toString());
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnit()
{
    //qDebug()<<stream.attributes().value("filename");
    QString fileName = attributes().value("filename").toString();
    m_parsingCmakeUnit = false;
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!fileName.endsWith(QLatin1String(".rule")) && !m_processedUnits.contains(fileName)) {
                // Now check whether we found a virtual element beneath
                if (m_parsingCmakeUnit) {
                    m_cmakeFileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ProjectFileType, false));
                } else {
                    if (fileName.endsWith(QLatin1String(".qrc")))
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ResourceType, false));
                    else
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::SourceType, false));
                }
                m_processedUnits.insert(fileName);
            }
            return;
        } else if (name() == "Option") {
            parseUnitOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnitOption()
{
    if (attributes().hasAttribute("virtualFolder"))
        m_parsingCmakeUnit = true;

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

QList<ProjectExplorer::FileNode *> CMakeCbpParser::fileList()
{
    return m_fileList;
}

QList<ProjectExplorer::FileNode *> CMakeCbpParser::cmakeFileList()
{
    return m_cmakeFileList;
}

bool CMakeCbpParser::hasCMakeFiles()
{
    return !m_cmakeFileList.isEmpty();
}

QStringList CMakeCbpParser::includeFiles()
{
    return m_includeFiles;
}

QList<CMakeBuildTarget> CMakeCbpParser::buildTargets()
{
    return m_buildTargets;
}

QString CMakeCbpParser::compilerName() const
{
    return m_compiler;
}

void CMakeBuildTarget::clear()
{
    executable.clear();
    makeCommand.clear();
    makeCleanCommand.clear();
    workingDirectory.clear();
    title.clear();
}

