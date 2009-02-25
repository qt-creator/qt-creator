/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cmakeproject.h"
#include "ui_cmakeconfigurewidget.h"
#include "cmakeconfigurewidget.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "cmakestep.h"
#include "makestep.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;


// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the make we need to call
// What is the actual compiler executable
// DEFINES

// Open Questions
// Who sets up the environment for cl.exe ? INCLUDEPATH and so on



CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager), m_fileName(fileName), m_rootNode(new CMakeProjectNode(m_fileName))
{
    m_file = new CMakeFile(this, fileName);
}

CMakeProject::~CMakeProject()
{
    delete m_rootNode;
}

// TODO also call this method if the CMakeLists.txt file changed, which is also called if the CMakeList.txt is updated
// TODO make this function work even if it is reparsing
void CMakeProject::parseCMakeLists()
{
    QString sourceDirectory = QFileInfo(m_fileName).absolutePath();
    m_manager->createXmlFile(cmakeStep()->userArguments(activeBuildConfiguration()), sourceDirectory, buildDirectory(activeBuildConfiguration()));

    QString cbpFile = findCbpFile(buildDirectory(activeBuildConfiguration()));
    CMakeCbpParser cbpparser;
    qDebug()<<"Parsing file "<<cbpFile;
    if (cbpparser.parseCbpFile(cbpFile)) {
        m_projectName = cbpparser.projectName();
        qDebug()<<"Building Tree";
        // TODO do a intelligent updating of the tree
        buildTree(m_rootNode, cbpparser.fileList());
        foreach (ProjectExplorer::FileNode *fn, cbpparser.fileList())
            m_files.append(fn->path());
        m_files.sort();

        qDebug()<<"Adding Targets";
        m_targets = cbpparser.targets();
        qDebug()<<"Printing targets";
        foreach(CMakeTarget ct, m_targets) {
            qDebug()<<ct.title<<" with executable:"<<ct.executable;
            qDebug()<<"WD:"<<ct.workingDirectory;
            qDebug()<<ct.makeCommand<<ct.makeCleanCommand;
            qDebug()<<"";
        }

        qDebug()<<"Updating CodeModel";
        CppTools::CppModelManagerInterface *modelmanager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
        if (modelmanager) {
            CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
            pinfo.includePaths = cbpparser.includeFiles();
            // TODO we only want C++ files, not all other stuff that might be in the project
            pinfo.sourceFiles = m_files;
            // TODO defines
            // TODO gcc preprocessor files
            modelmanager->updateProjectInfo(pinfo);
        }
    } else {
        // TODO report error
    }
}

QStringList CMakeProject::targets() const
{
    QStringList results;
    foreach(const CMakeTarget &ct, m_targets)
        results << ct.title;
    return results;
}

QString CMakeProject::findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   TODO the cbp file is named like the project() command in the CMakeList.txt file
    //   so this method below could find the wrong cbp file, if the user changes the project()
    //   name
    foreach (const QString &cbpFile , directory.entryList()) {
        if (cbpFile.endsWith(".cbp"))
            return directory.path() + "/" + cbpFile;
    }
    return QString::null;
}


void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list)
{
    //m_rootNode->addFileNodes(fileList, m_rootNode);
    qSort(list.begin(), list.end(), ProjectExplorer::ProjectNode::sortNodesByPath);
    foreach (ProjectExplorer::FileNode *fn, list) {
        // Get relative path to rootNode
        QString parentDir = QFileInfo(fn->path()).absolutePath();
        ProjectExplorer::FolderNode *folder = findOrCreateFolder(rootNode, parentDir);
        rootNode->addFileNodes(QList<ProjectExplorer::FileNode *>()<< fn, folder);
    }
    //m_rootNode->addFileNodes(list, rootNode);
}

ProjectExplorer::FolderNode *CMakeProject::findOrCreateFolder(CMakeProjectNode *rootNode, QString directory)
{
    QString relativePath = QDir(QFileInfo(rootNode->path()).path()).relativeFilePath(directory);
    QStringList parts = relativePath.split("/");
    ProjectExplorer::FolderNode *parent = rootNode;
    foreach (const QString &part, parts) {
        // Find folder in subFolders
        bool found = false;
        foreach (ProjectExplorer::FolderNode *folder, parent->subFolderNodes()) {
            if (QFileInfo(folder->path()).fileName() == part) {
                // yeah found something :)
                parent = folder;
                found = true;
                break;
            }
        }
        if (!found) {
            // No FolderNode yet, so create it
            ProjectExplorer::FolderNode *tmp = new ProjectExplorer::FolderNode(part);
            rootNode->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << tmp, parent);
            parent = tmp;
        }
    }
    return parent;
}

QString CMakeProject::name() const
{
    return m_projectName;
}

Core::IFile *CMakeProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *CMakeProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> CMakeProject::dependsOn()
{
    return QList<Project *>();
}

bool CMakeProject::isApplication() const
{
    return true;
}

ProjectExplorer::Environment CMakeProject::environment(const QString &buildConfiguration) const
{
    Q_UNUSED(buildConfiguration)
    //TODO
    return ProjectExplorer::Environment::systemEnvironment();
}

QString CMakeProject::buildDirectory(const QString &buildConfiguration) const
{
    QString buildDirectory = value(buildConfiguration, "buildDirectory").toString();
    if (buildDirectory.isEmpty())
        buildDirectory = QFileInfo(m_fileName).absolutePath() + "/qtcreator-build";
    return buildDirectory;
}

ProjectExplorer::BuildStepConfigWidget *CMakeProject::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildStepConfigWidget*> CMakeProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

// This method is called for new build configurations
// You should probably set some default values in this method
 void CMakeProject::newBuildConfiguration(const QString &buildConfiguration)
 {
     // Default to all
     makeStep()->setBuildTarget(buildConfiguration, "all", true);
 }

ProjectExplorer::ProjectNode *CMakeProject::rootProjectNode() const
{
    return m_rootNode;
}


QStringList CMakeProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    // TODO
    return m_files;
}

void CMakeProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    // TODO
    Project::saveSettingsImpl(writer);
}

MakeStep *CMakeProject::makeStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        MakeStep *ms = qobject_cast<MakeStep *>(bs);
        if (ms)
            return ms;
    }
    return 0;
}

CMakeStep *CMakeProject::cmakeStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        if (CMakeStep *cs = qobject_cast<CMakeStep *>(bs))
            return cs;
    }
    return 0;
}

void CMakeProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    // TODO
    Project::restoreSettingsImpl(reader);
    bool hasUserFile = !buildConfigurations().isEmpty();
    if (!hasUserFile) {
        // Ask the user for where he wants to build it
        // and the cmake command line

        // TODO check wheter there's already a CMakeCache.txt in the src directory,
        // then we don't need to ask, we simply need to build in the src directory

        CMakeConfigureDialog ccd(Core::ICore::instance()->mainWindow(), m_manager, QFileInfo(m_fileName).absolutePath());
        ccd.exec();

        qDebug()<<"ccd.buildDirectory()"<<ccd.buildDirectory();

        // Now create a standard build configuration
        CMakeStep *cmakeStep = new CMakeStep(this);
        MakeStep *makeStep = new MakeStep(this);

        insertBuildStep(0, cmakeStep);
        insertBuildStep(1, makeStep);

        addBuildConfiguration("all");
        setActiveBuildConfiguration("all");
        makeStep->setBuildTarget("all", "all", true);
        if (!ccd.buildDirectory().isEmpty())
        setValue("all", "buildDirectory", ccd.buildDirectory());
        cmakeStep->setUserArguments("all", ccd.arguments());
    }

    parseCMakeLists(); // Gets the directory from the active buildconfiguration

    if (!hasUserFile) {
        // Create run configurations for m_targets
        qDebug()<<"Create run configurations of m_targets";
        bool setActive = false;
        foreach(const CMakeTarget &ct, m_targets) {
            if (ct.executable.isEmpty())
                continue;
            QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(this, ct.executable, ct.workingDirectory));
            addRunConfiguration(rc);
            // The first one gets the honour of beeing the active one
            if (!setActive) {
                setActiveRunConfiguration(rc);
                setActive = true;
            }
        }

    }
}


CMakeFile::CMakeFile(CMakeProject *parent, QString fileName)
    : Core::IFile(parent), m_project(parent), m_fileName(fileName)
{

}

bool CMakeFile::save(const QString &fileName)
{
    // TODO
    // Once we have an texteditor open for this file, we probably do
    // need to implement this, don't we.
    Q_UNUSED(fileName);
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

void CMakeFile::modified(ReloadBehavior *behavior)
{
    Q_UNUSED(behavior);
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeProject *project)
    : m_project(project)
{
    QFormLayout *fl = new QFormLayout(this);
    setLayout(fl);
    m_pathChooser = new Core::Utils::PathChooser(this);
    m_pathChooser->setEnabled(false);
    // TODO currently doesn't work
    // since creating the cbp file also creates makefiles
    // and then cmake builds in that directory instead of shadow building
    // We need our own generator for that to work
    connect(m_pathChooser, SIGNAL(changed()), this, SLOT(buildDirectoryChanged()));
    fl->addRow("Build directory:", m_pathChooser);
}

QString CMakeBuildSettingsWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildSettingsWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    m_pathChooser->setPath(m_project->buildDirectory(buildConfiguration));
}

void CMakeBuildSettingsWidget::buildDirectoryChanged()
{
    m_project->setValue(m_buildConfiguration, "buildDirectory", m_pathChooser->path());
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
            parseTarget();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTarget()
{
    m_targetType = false;
    m_target.clear();

    if (attributes().hasAttribute("title"))
        m_target.title = attributes().value("title").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (m_targetType || m_target.title == "all" || m_target.title == "install") {
                m_targets.append(m_target);
            }
            return;
        } else if (name() == "Compiler") {
            parseCompiler();
        } else if (name() == "Option") {
            parseTargetOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetOption()
{
    if (attributes().hasAttribute("output"))
        m_target.executable = attributes().value("output").toString();
    else if (attributes().hasAttribute("type") && (attributes().value("type") == "1" || attributes().value("type") == "0"))
        m_targetType = true;
    else if (attributes().hasAttribute("working_dir"))
        m_target.workingDirectory = attributes().value("working_dir").toString();
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
            parseTargetBuild();
        } else if (name() == "Clean") {
            parseTargetClean();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetBuild()
{
    if (attributes().hasAttribute("command"))
        m_target.makeCommand = attributes().value("command").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetClean()
{
    if (attributes().hasAttribute("command"))
        m_target.makeCleanCommand = attributes().value("command").toString();
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
    if (!fileName.endsWith(".rule"))
        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::SourceType, false));
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
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

QStringList CMakeCbpParser::includeFiles()
{
    return m_includeFiles;
}

QList<CMakeTarget> CMakeCbpParser::targets()
{
    return m_targets;
}

void CMakeTarget::clear()
{
    executable = QString::null;
    makeCommand = QString::null;
    makeCleanCommand = QString::null;
    workingDirectory = QString::null;
    title = QString::null;
}

