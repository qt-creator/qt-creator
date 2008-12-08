/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakestep.h"
#include "makestep.h"

#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <QtCore/QDebug>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager), m_fileName(fileName), m_rootNode(new CMakeProjectNode(m_fileName))
{
    //TODO
    m_file = new CMakeFile(this, fileName);
    QDir dir = QFileInfo(m_fileName).absoluteDir();
    QString cbpFile = findCbpFile(dir);
    if (cbpFile.isEmpty())
        cbpFile = createCbpFile(dir);

    CMakeCbpParser cbpparser;
    if (cbpparser.parseCbpFile(cbpFile)) {
        buildTree(m_rootNode, cbpparser.fileList());
        foreach(ProjectExplorer::FileNode *fn, cbpparser.fileList())
            m_files.append(fn->path());
        m_files.sort();

        CppTools::CppModelManagerInterface *modelmanager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
        if (modelmanager) {
            CppTools::CppModelManagerInterface::ProjectInfo *pinfo = modelmanager->projectInfo(this);
            pinfo->includePaths = cbpparser.includeFiles();
            // TODO we only want C++ files, not all other stuff that might be in the project
            pinfo->sourceFiles = m_files;
            // TODO defines
        }
    } else {
        // TODO report error
    }
}

CMakeProject::~CMakeProject()
{
    delete m_rootNode;
}

QString CMakeProject::findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   TODO the cbp file is named like the project() command in the CMakeList.txt file
    //   so this method below could find the wrong cbp file, if the user changes the project()
    //   name
    foreach(const QString &cbpFile , directory.entryList())
    {
        if (cbpFile.endsWith(".cbp")) {
            return directory.path() + "/" + cbpFile;
        }
    }
    return QString::null;
}


QString CMakeProject::createCbpFile(const QDir &)
{
    // TODO create a cbp file.
    //  Issue: Where to create it? We want to do that in the build directory
    //         but at this stage we don't know the build directory yet
    //         So create it in a temp directory?
    //  Issue: We want to reuse whatever CMakeCache.txt that is alread there, which
    //         would indicate, creating it in the build directory
    //         Or we could use a temp directory and use -C builddirectory
    return QString::null;
}

void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list)
{
    //m_rootNode->addFileNodes(fileList, m_rootNode);
    qSort(list.begin(), list.end(), ProjectExplorer::ProjectNode::sortNodesByPath);
    foreach( ProjectExplorer::FileNode *fn, list) {
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
    foreach(const QString &part, parts) {
        // Find folder in subFolders
        bool found = false;
        foreach(ProjectExplorer::FolderNode *folder, parent->subFolderNodes()) {
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
    // TODO
    return "";
}

Core::IFile *CMakeProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *CMakeProject::projectManager() const
{
    return m_manager;
}

QList<Core::IFile *> CMakeProject::dependencies()
{
    return QList<Core::IFile *>();
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
    Q_UNUSED(buildConfiguration)
    //TODO
    return QFileInfo(m_fileName).absolutePath();
}

ProjectExplorer::BuildStepConfigWidget *CMakeProject::createConfigWidget()
{
    return new CMakeBuildSettingsWidget;
}

QList<ProjectExplorer::BuildStepConfigWidget*> CMakeProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

// This method is called for new build configurations
// You should probably set some default values in this method
 void CMakeProject::newBuildConfiguration(const QString &buildConfiguration)
 {
     Q_UNUSED(buildConfiguration);
     //TODO
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
    Q_UNUSED(writer)
}

void CMakeProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    // TODO
    Q_UNUSED(reader);
    if (buildConfigurations().isEmpty()) {
        // No build configuration, adding those

        // TODO do we want to create one build configuration per target?
        // or how do we want to handle that?

        CMakeStep *cmakeStep = new CMakeStep(this);
        MakeStep *makeStep = new MakeStep(this);

        insertBuildStep(0, cmakeStep);
        insertBuildStep(1, makeStep);

        addBuildConfiguration("all");
        setActiveBuildConfiguration("all");
    }
    // Restoring is fine
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


CMakeBuildSettingsWidget::CMakeBuildSettingsWidget()
{

}

QString CMakeBuildSettingsWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildSettingsWidget::init(const QString &buildConfiguration)
{
    Q_UNUSED(buildConfiguration);
    // TODO
}

bool CMakeCbpParser::parseCbpFile(const QString &fileName)
{
    QFile fi(fileName);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        setDevice(&fi);

        while(!atEnd()) {
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
    while(!atEnd()) {
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
    while(!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
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
    while(!atEnd()) {
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
    m_targetOutput.clear();
    m_targetType = false;
    while(!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (m_targetType && !m_targetOutput.isEmpty()) {
                qDebug()<<"found target "<<m_targetOutput;
                m_targets.insert(m_targetOutput);
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
        m_targetOutput = attributes().value("output").toString();
    else if (attributes().hasAttribute("type") && attributes().value("type") == "1")
        m_targetType = true;
    while(!atEnd()) {
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
    while(!atEnd()) {
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
    while(!atEnd()) {
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
    while(!atEnd()) {
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
