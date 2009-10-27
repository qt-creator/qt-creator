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

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "makestep.h"
#include "cmakeopenprojectwizard.h"
#include "cmakebuildenvironmentwidget.h"

#include <projectexplorer/projectexplorerconstants.h>
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
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory(CMakeProject *project)
    : IBuildConfigurationFactory(project),
    m_project(project)
{
}

CMakeBuildConfigurationFactory::~CMakeBuildConfigurationFactory()
{
}

QStringList CMakeBuildConfigurationFactory::availableCreationTypes() const
{
    return QStringList() << "Create";
}

QString CMakeBuildConfigurationFactory::displayNameForType(const QString & /* type */) const
{
    return tr("Create");
}

bool CMakeBuildConfigurationFactory::create(const QString &type) const
{
    QTC_ASSERT(type == "Create", return false);

    //TODO configuration name should be part of the cmakeopenprojectwizard
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;
    BuildConfiguration *bc = new BuildConfiguration(buildConfigurationName);

    CMakeOpenProjectWizard copw(m_project->projectManager(),
                                m_project->sourceDirectory(),
                                m_project->buildDirectory(bc),
                                m_project->environment(bc));
    if (copw.exec() != QDialog::Accepted) {
        delete bc;
        return false;
    }
    m_project->addBuildConfiguration(bc); // this also makes the name unique
    // Default to all
    if (m_project->targets().contains("all"))
        m_project->makeStep()->setBuildTarget(buildConfigurationName, "all", true);
    bc->setValue("buildDirectory", copw.buildDirectory());
    bc->setValue("msvcVersion", copw.msvcVersion());
    m_project->parseCMakeLists();
    return true;
}

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_buildConfigurationFactory(new CMakeBuildConfigurationFactory(this)),
      m_rootNode(new CMakeProjectNode(m_fileName)),
      m_toolChain(0),
      m_insideFileChanged(false)
{
    m_file = new CMakeFile(this, fileName);
}

CMakeProject::~CMakeProject()
{
    delete m_rootNode;
    delete m_toolChain;
}

IBuildConfigurationFactory *CMakeProject::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void CMakeProject::slotActiveBuildConfiguration()
{
    BuildConfiguration *activeBC = activeBuildConfiguration();
    // Pop up a dialog asking the user to rerun cmake
    QFileInfo sourceFileInfo(m_fileName);

    QString cbpFile = CMakeManager::findCbpFile(QDir(buildDirectory(activeBC)));
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
                                    buildDirectory(activeBC),
                                    mode,
                                    environment(activeBC));
        copw.exec();
        activeBC->setValue("msvcVersion", copw.msvcVersion());
    }
    // reparse
    parseCMakeLists();
}

void CMakeProject::fileChanged(const QString &fileName)
{
    Q_UNUSED(fileName)
    if (m_insideFileChanged== true)
        return;
    m_insideFileChanged = true;
    slotActiveBuildConfiguration();
    m_insideFileChanged = false;
}

void CMakeProject::updateToolChain(const QString &compiler)
{
    //qDebug()<<"CodeBlocks Compilername"<<compiler
    ProjectExplorer::ToolChain *newToolChain = 0;
    if (compiler == "gcc") {
#ifdef Q_OS_WIN
        newToolChain = ProjectExplorer::ToolChain::createMinGWToolChain("gcc", QString());
#else
        newToolChain = ProjectExplorer::ToolChain::createGccToolChain("gcc");
#endif
    } else if (compiler == "msvc8") {
        newToolChain = ProjectExplorer::ToolChain::createMSVCToolChain(activeBuildConfiguration()->value("msvcVersion").toString(), false);
    } else {
        // TODO other toolchains
        qDebug()<<"Not implemented yet!!! Qt Creator doesn't know which toolchain to use for"<<compiler;
    }

    if (ProjectExplorer::ToolChain::equals(newToolChain, m_toolChain)) {
        delete newToolChain;
        newToolChain = 0;
    } else {
        delete m_toolChain;
        m_toolChain = newToolChain;
    }
}

ProjectExplorer::ToolChain *CMakeProject::toolChain(BuildConfiguration *configuration) const
{
    if (configuration != activeBuildConfiguration())
        qWarning()<<"CMakeProject asked for toolchain of a not active buildconfiguration";
    return m_toolChain;
}

void CMakeProject::changeBuildDirectory(BuildConfiguration *configuration, const QString &newBuildDirectory)
{
    configuration->setValue("buildDirectory", newBuildDirectory);
    parseCMakeLists();
}

QString CMakeProject::sourceDirectory() const
{
    return QFileInfo(m_fileName).absolutePath();
}

bool CMakeProject::parseCMakeLists()
{
    // Find cbp file
    QString cbpFile = CMakeManager::findCbpFile(buildDirectory(activeBuildConfiguration()));

    // setFolderName
    m_rootNode->setFolderName(QFileInfo(cbpFile).completeBaseName());
    CMakeCbpParser cbpparser;
    // Parsing
    //qDebug()<<"Parsing file "<<cbpFile;
    if (cbpparser.parseCbpFile(cbpFile)) {
        // ToolChain
        updateToolChain(cbpparser.compilerName());

        m_projectName = cbpparser.projectName();
        m_rootNode->setFolderName(cbpparser.projectName());

        //qDebug()<<"Building Tree";


        QList<ProjectExplorer::FileNode *> fileList = cbpparser.fileList();

        QSet<QString> projectFiles;
        if (cbpparser.hasCMakeFiles()) {
            fileList.append(cbpparser.cmakeFileList());
            foreach(ProjectExplorer::FileNode *node, cbpparser.cmakeFileList())
                projectFiles.insert(node->path());
        } else {
            // Manually add the CMakeLists.txt file
            QString cmakeListTxt = sourceDirectory() + "/CMakeLists.txt";
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
        m_targets = cbpparser.targets();
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
        QList<ProjectExplorer::HeaderPath> allHeaderPaths = m_toolChain->systemHeaderPaths();
        foreach (ProjectExplorer::HeaderPath headerPath, allHeaderPaths) {
            if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());
            else
                allIncludePaths.append(headerPath.path());
        }
        // This explicitly adds -I. to the include paths
        allIncludePaths.append(sourceDirectory());

        allIncludePaths.append(cbpparser.includeFiles());
        CppTools::CppModelManagerInterface *modelmanager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
        if (modelmanager) {
            CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
            if (pinfo.includePaths != allIncludePaths
                || pinfo.sourceFiles != m_files
                || pinfo.defines != m_toolChain->predefinedMacros()
                || pinfo.frameworkPaths != allFrameworkPaths)  {
                pinfo.includePaths = allIncludePaths;
                // TODO we only want C++ files, not all other stuff that might be in the project
                pinfo.sourceFiles = m_files;
                pinfo.defines = m_toolChain->predefinedMacros(); // TODO this is to simplistic
                pinfo.frameworkPaths = allFrameworkPaths;
                modelmanager->updateProjectInfo(pinfo);
                modelmanager->updateSourceFiles(pinfo.sourceFiles);
            }
        }

        // Create run configurations for m_targets
        //qDebug()<<"Create run configurations of m_targets";
        QMultiMap<QString, QSharedPointer<CMakeRunConfiguration> > existingRunConfigurations;
        foreach(QSharedPointer<ProjectExplorer::RunConfiguration> cmakeRunConfiguration, runConfigurations()) {
            if (QSharedPointer<CMakeRunConfiguration> rc = cmakeRunConfiguration.objectCast<CMakeRunConfiguration>()) {
                existingRunConfigurations.insert(rc->title(), rc);
            }
        }

        bool setActive = existingRunConfigurations.isEmpty();
        foreach(const CMakeTarget &ct, m_targets) {
            if (ct.executable.isEmpty())
                continue;
            if (ct.title.endsWith("/fast"))
                continue;
            QList<QSharedPointer<CMakeRunConfiguration> > list = existingRunConfigurations.values(ct.title);
            if (!list.isEmpty()) {
                // Already exists, so override the settings...
                foreach (QSharedPointer<CMakeRunConfiguration> rc, list) {
                    //qDebug()<<"Updating Run Configuration with title"<<ct.title;
                    //qDebug()<<"  Executable new:"<<ct.executable<< "old:"<<rc->executable();
                    //qDebug()<<"  WD new:"<<ct.workingDirectory<<"old:"<<rc->workingDirectory();
                    rc->setExecutable(ct.executable);
                    rc->setWorkingDirectory(ct.workingDirectory);
                }
                existingRunConfigurations.remove(ct.title);
            } else {
                // Does not exist yet
                //qDebug()<<"Adding new run configuration with title"<<ct.title;
                //qDebug()<<"  Executable:"<<ct.executable<<"WD:"<<ct.workingDirectory;
                QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(this, ct.executable, ct.workingDirectory, ct.title));
                addRunConfiguration(rc);
                // The first one gets the honour of beeing the active one
                if (setActive) {
                    setActiveRunConfiguration(rc);
                    setActive = false;
                }
            }
        }
        QMultiMap<QString, QSharedPointer<CMakeRunConfiguration> >::const_iterator it =
                existingRunConfigurations.constBegin();
        for( ; it != existingRunConfigurations.constEnd(); ++it) {
            QSharedPointer<CMakeRunConfiguration> rc = it.value();
            //qDebug()<<"Removing old RunConfiguration with title:"<<rc->title();
            //qDebug()<<"  Executable:"<<rc->executable()<<rc->workingDirectory();
            removeRunConfiguration(rc);
        }
        //qDebug()<<"\n";
    } else {
        // TODO report error
        qDebug()<<"Parsing failed";
        delete m_toolChain;
        m_toolChain = 0;
        return false;
    }
    return true;
}

QString CMakeProject::buildParser(BuildConfiguration *configuration) const
{
    Q_UNUSED(configuration)
    // TODO this is actually slightly wrong, but do i care?
    // this should call toolchain(configuration)
    if (!m_toolChain)
        return QString::null;
    if (m_toolChain->type() == ProjectExplorer::ToolChain::GCC
        || m_toolChain->type() == ProjectExplorer::ToolChain::LinuxICC
        || m_toolChain->type() == ProjectExplorer::ToolChain::MinGW) {
        return ProjectExplorer::Constants::BUILD_PARSER_GCC;
    } else if (m_toolChain->type() == ProjectExplorer::ToolChain::MSVC
               || m_toolChain->type() == ProjectExplorer::ToolChain::WINCE) {
        return ProjectExplorer::Constants::BUILD_PARSER_MSVC;
    }
    return QString::null;
}

QStringList CMakeProject::targets() const
{
    QStringList results;
    foreach (const CMakeTarget &ct, m_targets) {
        if (ct.executable.isEmpty())
            continue;
        if (ct.title.endsWith("/fast"))
            continue;
        results << ct.title;
    }
    return results;
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

    // remove old file nodes and check wheter folder nodes can be removed
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
    QStringList parts = relativePath.split("/", QString::SkipEmptyParts);
    ProjectExplorer::FolderNode *parent = rootNode;
    QString path = QFileInfo(rootNode->path()).path();
    foreach (const QString &part, parts) {
        path += "/" + part;
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
            tmp->setFolderName(part);
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

CMakeManager *CMakeProject::projectManager() const
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

ProjectExplorer::Environment CMakeProject::baseEnvironment(BuildConfiguration *configuration) const
{
    Environment env = useSystemEnvironment(configuration) ? Environment(QProcess::systemEnvironment()) : Environment();
    return env;
}

ProjectExplorer::Environment CMakeProject::environment(BuildConfiguration *configuration) const
{
    Environment env = baseEnvironment(configuration);
    env.modify(userEnvironmentChanges(configuration));
    return env;
}

void CMakeProject::setUseSystemEnvironment(BuildConfiguration *configuration, bool b)
{
    if (b == useSystemEnvironment(configuration))
        return;
    configuration->setValue("clearSystemEnvironment", !b);
    emit environmentChanged(configuration->name());
}

bool CMakeProject::useSystemEnvironment(BuildConfiguration *configuration) const
{
    bool b = !(configuration->value("clearSystemEnvironment").isValid() &&
               configuration->value("clearSystemEnvironment").toBool());
    return b;
}

QList<ProjectExplorer::EnvironmentItem> CMakeProject::userEnvironmentChanges(BuildConfiguration *configuration) const
{
    return EnvironmentItem::fromStringList(configuration->value("userEnvironmentChanges").toStringList());
}

void CMakeProject::setUserEnvironmentChanges(BuildConfiguration *configuration, const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    QStringList list = EnvironmentItem::toStringList(diff);
    if (list == configuration->value("userEnvironmentChanges"))
        return;
    configuration->setValue("userEnvironmentChanges", list);
    emit environmentChanged(configuration->name());
}

QString CMakeProject::buildDirectory(BuildConfiguration *configuration) const
{
    QString buildDirectory = configuration->value("buildDirectory").toString();
    if (buildDirectory.isEmpty())
        buildDirectory = sourceDirectory() + "/qtcreator-build";
    return buildDirectory;
}

ProjectExplorer::BuildConfigWidget *CMakeProject::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildConfigWidget*> CMakeProject::subConfigWidgets()
{
    QList<ProjectExplorer::BuildConfigWidget*> list;
    list <<  new CMakeBuildEnvironmentWidget(this);
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

void CMakeProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
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


bool CMakeProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);
    bool hasUserFile = !buildConfigurations().isEmpty();
    MakeStep *makeStep = 0;
    if (!hasUserFile) {
        // Ask the user for where he wants to build it
        // and the cmake command line

        CMakeOpenProjectWizard copw(m_manager, sourceDirectory(), ProjectExplorer::Environment::systemEnvironment());
        copw.exec();

        qDebug()<<"ccd.buildDirectory()"<<copw.buildDirectory();

        // Now create a standard build configuration
        makeStep = new MakeStep(this);

        insertBuildStep(0, makeStep);

        ProjectExplorer::BuildConfiguration *bc = new ProjectExplorer::BuildConfiguration("all");
        addBuildConfiguration(bc);
        bc->setValue("msvcVersion", copw.msvcVersion());
        if (!copw.buildDirectory().isEmpty())
            bc->setValue("buildDirectory", copw.buildDirectory());
        //TODO save arguments somewhere copw.arguments()

        MakeStep *cleanMakeStep = new MakeStep(this);
        insertCleanStep(0, cleanMakeStep);
        cleanMakeStep->setValue("clean", true);
        setActiveBuildConfiguration(bc);
    } else {
        // We have a user file, but we could still be missing the cbp file
        // or simply run createXml with the saved settings
        QFileInfo sourceFileInfo(m_fileName);
        QStringList needToCreate;
        QStringList needToUpdate;
        BuildConfiguration *activeBC = activeBuildConfiguration();
        QString cbpFile = CMakeManager::findCbpFile(QDir(buildDirectory(activeBC)));
        QFileInfo cbpFileFi(cbpFile);

        CMakeOpenProjectWizard::Mode mode = CMakeOpenProjectWizard::Nothing;
        if (!cbpFileFi.exists())
            mode = CMakeOpenProjectWizard::NeedToCreate;
        else if (cbpFileFi.lastModified() < sourceFileInfo.lastModified())
            mode = CMakeOpenProjectWizard::NeedToUpdate;

        if (mode != CMakeOpenProjectWizard::Nothing) {
            CMakeOpenProjectWizard copw(m_manager,
                                        sourceFileInfo.absolutePath(),
                                        buildDirectory(activeBC),
                                        mode,
                                        environment(activeBC));
            copw.exec();
            activeBC->setValue("msvcVersion", copw.msvcVersion());
        }
    }

    if (!hasUserFile && targets().contains("all"))
        makeStep->setBuildTarget("all", "all", true);

    m_watcher = new ProjectExplorer::FileWatcher(this);
    connect(m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
    bool result = parseCMakeLists(); // Gets the directory from the active buildconfiguration
    if (!result)
        return false;

    connect(this, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(slotActiveBuildConfiguration()));
    return true;
}

CMakeTarget CMakeProject::targetForTitle(const QString &title)
{
    foreach(const CMakeTarget &ct, m_targets)
        if (ct.title == title)
            return ct;
    return CMakeTarget();
}

ProjectExplorer::ToolChain::ToolChainType CMakeProject::toolChainType() const
{
    if (m_toolChain)
        return m_toolChain->type();
    return ProjectExplorer::ToolChain::UNKNOWN;
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

void CMakeFile::modified(ReloadBehavior *behavior)
{
    Q_UNUSED(behavior)
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeProject *project)
    : m_project(project)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);
    // TODO currently doesn't work
    // since creating the cbp file also creates makefiles
    // and then cmake builds in that directory instead of shadow building
    // We need our own generator for that to work

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

void CMakeBuildSettingsWidget::init(const QString &buildConfigurationName)
{
    m_buildConfiguration = buildConfigurationName;
    BuildConfiguration *bc = m_project->buildConfiguration(buildConfigurationName);
    m_pathLineEdit->setText(m_project->buildDirectory(bc));
    if (m_project->buildDirectory(bc) == m_project->sourceDirectory())
        m_changeButton->setEnabled(false);
    else
        m_changeButton->setEnabled(true);
}

void CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog()
{
    BuildConfiguration *bc = m_project->buildConfiguration(m_buildConfiguration);
    CMakeOpenProjectWizard copw(m_project->projectManager(),
                                m_project->sourceDirectory(),
                                m_project->buildDirectory(bc),
                                m_project->environment(bc));
    if (copw.exec() == QDialog::Accepted) {
        m_project->changeBuildDirectory(bc, copw.buildDirectory());
        m_pathLineEdit->setText(m_project->buildDirectory(bc));
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
    m_parsingCmakeUnit = false;
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!fileName.endsWith(".rule") && !m_processedUnits.contains(fileName)) {
                // Now check whether we found a virtual element beneath
                if (m_parsingCmakeUnit) {
                    m_cmakeFileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ProjectFileType, false));
                } else {
                    if (fileName.endsWith(".qrc"))
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

QList<CMakeTarget> CMakeCbpParser::targets()
{
    return m_targets;
}

QString CMakeCbpParser::compilerName() const
{
    return m_compiler;
}

void CMakeTarget::clear()
{
    executable = QString::null;
    makeCommand = QString::null;
    makeCleanCommand = QString::null;
    workingDirectory = QString::null;
    title = QString::null;
}

