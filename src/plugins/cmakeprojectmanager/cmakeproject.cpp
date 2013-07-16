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

#include "cmakeproject.h"

#include "cmakebuildconfiguration.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "makestep.h"
#include "cmakeopenprojectwizard.h"
#include "cmakeuicodemodelsupport.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/target.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectmacroexpander.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/variablemanager.h>

#include <QMap>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QFormLayout>
#include <QInputDialog>
#include <QFileSystemWatcher>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the make we need to call
// What is the actual compiler executable
// DEFINES

// Open Questions
// Who sets up the environment for cl.exe ? INCLUDEPATH and so on

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

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager),
      m_activeTarget(0),
      m_fileName(fileName),
      m_rootNode(new CMakeProjectNode(m_fileName)),
      m_lastEditor(0)
{
    setProjectContext(Core::Context(CMakeProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    m_file = new CMakeFile(this, fileName);

    connect(this, SIGNAL(buildTargetsChanged()),
            this, SLOT(updateRunConfigurations()));
}

CMakeProject::~CMakeProject()
{
    // Remove CodeModel support
    CppTools::CppModelManagerInterface *modelManager
            = CppTools::CppModelManagerInterface::instance();
    QMap<QString, CMakeUiCodeModelSupport *>::const_iterator it, end;
    it = m_uiCodeModelSupport.constBegin();
    end = m_uiCodeModelSupport.constEnd();
    for (; it!=end; ++it) {
        modelManager->removeEditorSupport(it.value());
        delete it.value();
    }

    m_codeModelFuture.cancel();
    delete m_rootNode;
}

void CMakeProject::fileChanged(const QString &fileName)
{
    Q_UNUSED(fileName)

    parseCMakeLists();
}

void CMakeProject::changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc)
        return;

    CMakeBuildConfiguration *cmakebc = static_cast<CMakeBuildConfiguration *>(bc);

    // Pop up a dialog asking the user to rerun cmake
    QString cbpFile = CMakeManager::findCbpFile(QDir(bc->buildDirectory()));
    QFileInfo cbpFileFi(cbpFile);
    CMakeOpenProjectWizard::Mode mode = CMakeOpenProjectWizard::Nothing;
    if (!cbpFileFi.exists()) {
        mode = CMakeOpenProjectWizard::NeedToCreate;
    } else {
        foreach (const QString &file, m_watchedFiles) {
            if (QFileInfo(file).lastModified() > cbpFileFi.lastModified()) {
                mode = CMakeOpenProjectWizard::NeedToUpdate;
                break;
            }
        }
    }

    if (mode != CMakeOpenProjectWizard::Nothing) {
        CMakeOpenProjectWizard copw(m_manager, mode,
                                    CMakeOpenProjectWizard::BuildInfo(cmakebc));
        if (copw.exec() == QDialog::Accepted)
            cmakebc->setUseNinja(copw.useNinja()); // NeedToCreate can change the Ninja setting
    }

    // reparse
    parseCMakeLists();
}

void CMakeProject::activeTargetWasChanged(Target *target)
{
    if (m_activeTarget) {
        disconnect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    }

    m_activeTarget = target;

    if (!m_activeTarget)
        return;

    connect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    changeActiveBuildConfiguration(m_activeTarget->activeBuildConfiguration());
}

void CMakeProject::changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory)
{
    bc->setBuildDirectory(newBuildDirectory);
    parseCMakeLists();
}

QString CMakeProject::shadowBuildDirectory(const QString &projectFilePath, const Kit *k, const QString &bcName)
{
    if (projectFilePath.isEmpty())
        return QString();
    QFileInfo info(projectFilePath);

    const QString projectName = QFileInfo(info.absolutePath()).fileName();
    ProjectExplorer::ProjectMacroExpander expander(projectFilePath, projectName, k, bcName);
    QDir projectDir = QDir(projectDirectory(projectFilePath));
    QString buildPath = Utils::expandMacros(Core::DocumentManager::buildDirectory(), &expander);
    return QDir::cleanPath(projectDir.absoluteFilePath(buildPath));
}

bool CMakeProject::parseCMakeLists()
{
    if (!activeTarget() ||
        !activeTarget()->activeBuildConfiguration()) {
        return false;
    }

    CMakeBuildConfiguration *activeBC = static_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    foreach (Core::IEditor *editor, Core::EditorManager::instance()->openedEditors())
        if (isProjectFile(editor->document()->fileName()))
            editor->document()->infoBar()->removeInfo(Core::Id("CMakeEditor.RunCMake"));

    // Find cbp file
    QString cbpFile = CMakeManager::findCbpFile(activeBC->buildDirectory());

    if (cbpFile.isEmpty()) {
        emit buildTargetsChanged();
        return false;
    }

    // setFolderName
    m_rootNode->setDisplayName(QFileInfo(cbpFile).completeBaseName());
    CMakeCbpParser cbpparser;
    // Parsing
    //qDebug()<<"Parsing file "<<cbpFile;
    if (!cbpparser.parseCbpFile(cbpFile)) {
        // TODO report error
        emit buildTargetsChanged();
        return false;
    }

    foreach (const QString &file, m_watcher->files())
        if (file != cbpFile)
            m_watcher->removePath(file);

    // how can we ensure that it is completely written?
    m_watcher->addPath(cbpFile);

    m_projectName = cbpparser.projectName();
    m_rootNode->setDisplayName(cbpparser.projectName());

    //qDebug()<<"Building Tree";
    QList<ProjectExplorer::FileNode *> fileList = cbpparser.fileList();
    QSet<QString> projectFiles;
    if (cbpparser.hasCMakeFiles()) {
        fileList.append(cbpparser.cmakeFileList());
        foreach (const ProjectExplorer::FileNode *node, cbpparser.cmakeFileList())
            projectFiles.insert(node->path());
    } else {
        // Manually add the CMakeLists.txt file
        QString cmakeListTxt = projectDirectory() + QLatin1String("/CMakeLists.txt");
        bool generated = false;
        fileList.append(new ProjectExplorer::FileNode(cmakeListTxt, ProjectExplorer::ProjectFileType, generated));
        projectFiles.insert(cmakeListTxt);
    }

    m_watchedFiles = projectFiles;

    m_files.clear();
    foreach (ProjectExplorer::FileNode *fn, fileList)
        m_files.append(fn->path());
    m_files.sort();

    buildTree(m_rootNode, fileList);

    //qDebug()<<"Adding Targets";
    m_buildTargets = cbpparser.buildTargets();
//        qDebug()<<"Printing targets";
//        foreach (CMakeBuildTarget ct, m_buildTargets) {
//            qDebug()<<ct.title<<" with executable:"<<ct.executable;
//            qDebug()<<"WD:"<<ct.workingDirectory;
//            qDebug()<<ct.makeCommand<<ct.makeCleanCommand;
//            qDebug()<<"";
//        }


    // TOOD this code ain't very pretty ...
    m_uicCommand.clear();
    QFile cmakeCache(activeBC->buildDirectory() + QLatin1String("/CMakeCache.txt"));
    cmakeCache.open(QIODevice::ReadOnly);
    while (!cmakeCache.atEnd()) {
        QByteArray line = cmakeCache.readLine();
        if (line.startsWith("QT_UIC_EXECUTABLE")) {
            if (int pos = line.indexOf('='))
                m_uicCommand = QString::fromLocal8Bit(line.mid(pos + 1).trimmed());
            break;
        }
    }
    cmakeCache.close();

    createUiCodeModelSupport();

    Kit *k = activeTarget()->kit();
    ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (!tc) {
        emit buildTargetsChanged();
        emit fileListChanged();
        return true;
    }

    QStringList cxxflags;
    bool found = false;
    foreach (const CMakeBuildTarget &buildTarget, m_buildTargets) {
        QString makeCommand = QDir::fromNativeSeparators(buildTarget.makeCommand);
        int startIndex = makeCommand.indexOf(QLatin1Char('\"'));
        int endIndex = makeCommand.indexOf(QLatin1Char('\"'), startIndex + 1);
        if (startIndex == -1 || endIndex == -1)
            continue;
        startIndex += 1;
        QString makefile = makeCommand.mid(startIndex, endIndex - startIndex);
        int slashIndex = makefile.lastIndexOf(QLatin1Char('/'));
        makefile.truncate(slashIndex);
        makefile.append(QLatin1String("/CMakeFiles/") + buildTarget.title + QLatin1String(".dir/flags.make"));
        QFile file(makefile);
        if (file.exists()) {
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.startsWith(QLatin1String("CXX_FLAGS ="))) {
                    // Skip past =
                    cxxflags = line.mid(11).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
                    found = true;
                    break;
                }
            }
        }
        if (found)
            break;
    }
    // Attempt to find build.ninja file and obtain FLAGS (CXX_FLAGS) from there if no suitable flags.make were
    // found
    if (!found && !cbpparser.buildTargets().isEmpty()) {
        // Get "all" target's working directory
        QString buildNinjaFile = QDir::fromNativeSeparators(cbpparser.buildTargets().at(0).workingDirectory);
        buildNinjaFile += QLatin1String("/build.ninja");
        QFile buildNinja(buildNinjaFile);
        if (buildNinja.exists()) {
            buildNinja.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream stream(&buildNinja);
            bool cxxFound = false;
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                // Look for a build rule which invokes CXX_COMPILER
                if (line.startsWith(QLatin1String("build"))) {
                    cxxFound = line.indexOf(QLatin1String("CXX_COMPILER")) != -1;
                } else if (cxxFound && line.startsWith(QLatin1String("FLAGS ="))) {
                    // Skip past =
                    cxxflags = line.mid(7).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
                    break;
                }
            }
        }
    }

    CppTools::CppModelManagerInterface *modelmanager =
            CppTools::CppModelManagerInterface::instance();
    if (modelmanager) {
        CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
        pinfo.clearProjectParts();

        CppTools::ProjectPart::Ptr part(new CppTools::ProjectPart);

        part->evaluateToolchain(tc,
                                cxxflags,
                                cxxflags,
                                SysRootKitInformation::sysRoot(k));

        // This explicitly adds -I. to the include paths
        part->includePaths += projectDirectory();
        part->includePaths += cbpparser.includeFiles();
        part->defines += cbpparser.defines();

        CppTools::ProjectFileAdder adder(part->files);
        foreach (const QString &file, m_files)
            adder.maybeAdd(file);

        pinfo.appendProjectPart(part);
        m_codeModelFuture.cancel();
        m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);

        setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, !part->files.isEmpty());
    }

    emit buildTargetsChanged();
    emit fileListChanged();

    return true;
}

bool CMakeProject::isProjectFile(const QString &fileName)
{
    return m_watchedFiles.contains(fileName);
}

QList<CMakeBuildTarget> CMakeProject::buildTargets() const
{
    return m_buildTargets;
}

QStringList CMakeProject::buildTargetTitles(bool runnable) const
{
    QStringList results;
    foreach (const CMakeBuildTarget &ct, m_buildTargets) {
        if (runnable && (ct.executable.isEmpty() || ct.library))
            continue;
        results << ct.title;
    }
    return results;
}

bool CMakeProject::hasBuildTarget(const QString &title) const
{
    foreach (const CMakeBuildTarget &ct, m_buildTargets) {
        if (ct.title == title)
            return true;
    }
    return false;
}

void CMakeProject::gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list)
{
    foreach (ProjectExplorer::FolderNode *folder, parent->subFolderNodes())
        gatherFileNodes(folder, list);
    foreach (ProjectExplorer::FileNode *file, parent->fileNodes())
        list.append(file);
}

bool sortNodesByPath(Node *a, Node *b)
{
    return a->path() < b->path();
}

void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> newList)
{
    // Gather old list
    QList<ProjectExplorer::FileNode *> oldList;
    gatherFileNodes(rootNode, oldList);
    qSort(oldList.begin(), oldList.end(), sortNodesByPath);
    qSort(newList.begin(), newList.end(), sortNodesByPath);

    // generate added and deleted list
    QList<ProjectExplorer::FileNode *>::const_iterator oldIt  = oldList.constBegin();
    QList<ProjectExplorer::FileNode *>::const_iterator oldEnd = oldList.constEnd();
    QList<ProjectExplorer::FileNode *>::const_iterator newIt  = newList.constBegin();
    QList<ProjectExplorer::FileNode *>::const_iterator newEnd = newList.constEnd();

    QList<ProjectExplorer::FileNode *> added;
    QList<ProjectExplorer::FileNode *> deleted;

    while (oldIt != oldEnd && newIt != newEnd) {
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

Core::Id CMakeProject::id() const
{
    return Core::Id(Constants::CMAKEPROJECT_ID);
}

Core::IDocument *CMakeProject::document() const
{
    return m_file;
}

CMakeManager *CMakeProject::projectManager() const
{
    return m_manager;
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
        CMakeOpenProjectWizard copw(m_manager, projectDirectory(), Utils::Environment::systemEnvironment());
        if (copw.exec() != QDialog::Accepted)
            return false;
        Kit *k = copw.kit();
        Target *t = new Target(this, k);
        CMakeBuildConfiguration *bc(new CMakeBuildConfiguration(t));
        bc->setDefaultDisplayName(QLatin1String("all"));
        bc->setUseNinja(copw.useNinja());
        bc->setBuildDirectory(copw.buildDirectory());
        ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

        // Now create a standard build configuration
        buildSteps->insertStep(0, new MakeStep(buildSteps));

        MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
        cleanSteps->insertStep(0, cleanMakeStep);
        cleanMakeStep->setAdditionalArguments(QLatin1String("clean"));
        cleanMakeStep->setClean(true);

        t->addBuildConfiguration(bc);

        DeployConfigurationFactory *fac = ExtensionSystem::PluginManager::instance()->getObject<DeployConfigurationFactory>();
        ProjectExplorer::DeployConfiguration *dc = fac->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID);
        t->addDeployConfiguration(dc);

        addTarget(t);
    } else {
        // We have a user file, but we could still be missing the cbp file
        // or simply run createXml with the saved settings
        QFileInfo sourceFileInfo(m_fileName);
        CMakeBuildConfiguration *activeBC = qobject_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
        if (!activeBC)
            return false;
        QString cbpFile = CMakeManager::findCbpFile(QDir(activeBC->buildDirectory()));
        QFileInfo cbpFileFi(cbpFile);

        CMakeOpenProjectWizard::Mode mode = CMakeOpenProjectWizard::Nothing;
        if (!cbpFileFi.exists())
            mode = CMakeOpenProjectWizard::NeedToCreate;
        else if (cbpFileFi.lastModified() < sourceFileInfo.lastModified())
            mode = CMakeOpenProjectWizard::NeedToUpdate;

        if (mode != CMakeOpenProjectWizard::Nothing) {
            CMakeOpenProjectWizard copw(m_manager, mode,
                                        CMakeOpenProjectWizard::BuildInfo(activeBC));
            if (copw.exec() != QDialog::Accepted)
                return false;
            else
                activeBC->setUseNinja(copw.useNinja());
        }
    }

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));

    parseCMakeLists();

    if (!hasUserFile && hasBuildTarget(QLatin1String("all"))) {
        MakeStep *makeStep = qobject_cast<MakeStep *>(
                    activeTarget()->activeBuildConfiguration()->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->at(0));
        Q_ASSERT(makeStep);
        makeStep->setBuildTarget(QLatin1String("all"), true);
    }

    connect(Core::EditorManager::instance(), SIGNAL(editorAboutToClose(Core::IEditor*)),
            this, SLOT(editorAboutToClose(Core::IEditor*)));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(editorChanged(Core::IEditor*)));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    m_activeTarget = activeTarget();
    if (m_activeTarget)
        connect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetWasChanged(ProjectExplorer::Target*)));

    return true;
}

bool CMakeProject::setupTarget(Target *t)
{
    CMakeBuildConfigurationFactory *factory
            = ExtensionSystem::PluginManager::instance()->getObject<CMakeBuildConfigurationFactory>();
    CMakeBuildConfiguration *bc = factory->create(t, Constants::CMAKE_BC_ID, QLatin1String("all"));
    if (!bc)
        return false;

    t->addBuildConfiguration(bc);

    DeployConfigurationFactory *fac = ExtensionSystem::PluginManager::instance()->getObject<DeployConfigurationFactory>();
    ProjectExplorer::DeployConfiguration *dc = fac->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID);
    t->addDeployConfiguration(dc);
    return true;
}

CMakeBuildTarget CMakeProject::buildTargetForTitle(const QString &title)
{
    foreach (const CMakeBuildTarget &ct, m_buildTargets)
        if (ct.title == title)
            return ct;
    return CMakeBuildTarget();
}

QString CMakeProject::uicCommand() const
{
    return m_uicCommand;
}

QString CMakeProject::uiHeaderFile(const QString &uiFile)
{
    QFileInfo fi(uiFile);
    Utils::FileName project = Utils::FileName::fromString(projectDirectory());
    Utils::FileName baseDirectory = Utils::FileName::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        Utils::FileName cmakeListsTxt = baseDirectory;
        cmakeListsTxt.appendPath(QLatin1String("CMakeLists.txt"));
        if (cmakeListsTxt.toFileInfo().exists())
            break;
        QDir dir(baseDirectory.toString());
        dir.cdUp();
        baseDirectory = Utils::FileName::fromString(dir.absolutePath());
    }

    QDir srcDirRoot = QDir(project.toString());
    QString relativePath = srcDirRoot.relativeFilePath(baseDirectory.toString());
    QDir buildDir = QDir(activeTarget()->activeBuildConfiguration()->buildDirectory());
    QString uiHeaderFilePath = buildDir.absoluteFilePath(relativePath);
    uiHeaderFilePath += QLatin1String("/ui_");
    uiHeaderFilePath += fi.completeBaseName();
    uiHeaderFilePath += QLatin1String(".h");

    return QDir::cleanPath(uiHeaderFilePath);
}

void CMakeProject::updateRunConfigurations()
{
    foreach (Target *t, targets())
        updateRunConfigurations(t);
}

// TODO Compare with updateDefaultRunConfigurations();
void CMakeProject::updateRunConfigurations(Target *t)
{
    // *Update* runconfigurations:
    QMultiMap<QString, CMakeRunConfiguration*> existingRunConfigurations;
    QList<ProjectExplorer::RunConfiguration *> toRemove;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations()) {
        if (CMakeRunConfiguration* cmakeRC = qobject_cast<CMakeRunConfiguration *>(rc))
            existingRunConfigurations.insert(cmakeRC->title(), cmakeRC);
        QtSupport::CustomExecutableRunConfiguration *ceRC =
                qobject_cast<QtSupport::CustomExecutableRunConfiguration *>(rc);
        if (ceRC && !ceRC->isConfigured())
            toRemove << rc;
    }

    foreach (const CMakeBuildTarget &ct, buildTargets()) {
        if (ct.library)
            continue;
        if (ct.executable.isEmpty())
            continue;
        QList<CMakeRunConfiguration *> list = existingRunConfigurations.values(ct.title);
        if (!list.isEmpty()) {
            // Already exists, so override the settings...
            foreach (CMakeRunConfiguration *rc, list) {
                rc->setExecutable(ct.executable);
                rc->setBaseWorkingDirectory(ct.workingDirectory);
                rc->setEnabled(true);
            }
            existingRunConfigurations.remove(ct.title);
        } else {
            // Does not exist yet
            Core::Id id = CMakeRunConfigurationFactory::idFromBuildTarget(ct.title);
            CMakeRunConfiguration *rc = new CMakeRunConfiguration(t, id, ct.executable,
                                                                  ct.workingDirectory, ct.title);
            t->addRunConfiguration(rc);
        }
    }
    QMultiMap<QString, CMakeRunConfiguration *>::const_iterator it =
            existingRunConfigurations.constBegin();
    for ( ; it != existingRunConfigurations.constEnd(); ++it) {
        CMakeRunConfiguration *rc = it.value();
        // The executables for those runconfigurations aren't build by the current buildconfiguration
        // We just set a disable flag and show that in the display name
        rc->setEnabled(false);
        // removeRunConfiguration(rc);
    }

    foreach (ProjectExplorer::RunConfiguration *rc, toRemove)
        t->removeRunConfiguration(rc);

    if (t->runConfigurations().isEmpty()) {
        // Oh no, no run configuration,
        // create a custom executable run configuration
        t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    }
}

void CMakeProject::createUiCodeModelSupport()
{
//    qDebug()<<"creatUiCodeModelSupport()";
    CppTools::CppModelManagerInterface *modelManager
            = CppTools::CppModelManagerInterface::instance();

    // First move all to
    QMap<QString, CMakeUiCodeModelSupport *> oldCodeModelSupport;
    oldCodeModelSupport = m_uiCodeModelSupport;
    m_uiCodeModelSupport.clear();

    // Find all ui files
    foreach (const QString &uiFile, m_files) {
        if (uiFile.endsWith(QLatin1String(".ui"))) {
            // UI file, not convert to
            QString uiHeaderFilePath = uiHeaderFile(uiFile);
            QMap<QString, CMakeUiCodeModelSupport *>::iterator it
                    = oldCodeModelSupport.find(uiFile);
            if (it != oldCodeModelSupport.end()) {
                //                qDebug()<<"updated old codemodelsupport";
                CMakeUiCodeModelSupport *cms = it.value();
                cms->setFileName(uiHeaderFilePath);
                m_uiCodeModelSupport.insert(it.key(), cms);
                oldCodeModelSupport.erase(it);
            } else {
                //                qDebug()<<"adding new codemodelsupport";
                CMakeUiCodeModelSupport *cms = new CMakeUiCodeModelSupport(modelManager, this, uiFile, uiHeaderFilePath);
                m_uiCodeModelSupport.insert(uiFile, cms);
                modelManager->addEditorSupport(cms);
            }
        }
    }

    // Remove old
    QMap<QString, CMakeUiCodeModelSupport *>::const_iterator it, end;
    end = oldCodeModelSupport.constEnd();
    for (it = oldCodeModelSupport.constBegin(); it!=end; ++it) {
        modelManager->removeEditorSupport(it.value());
        delete it.value();
    }
}

void CMakeProject::updateCodeModelSupportFromEditor(const QString &uiFileName,
                                                    const QString &contents)
{
    const QMap<QString, CMakeUiCodeModelSupport *>::const_iterator it =
            m_uiCodeModelSupport.constFind(uiFileName);
    if (it != m_uiCodeModelSupport.constEnd())
        it.value()->updateFromEditor(contents);
}

void CMakeProject::editorChanged(Core::IEditor *editor)
{
    // Handle old editor
    if (isFormWindowEditor(m_lastEditor)) {
        disconnect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));
        if (m_dirtyUic) {
            const QString contents =  formWindowEditorContents(m_lastEditor);
            updateCodeModelSupportFromEditor(m_lastEditor->document()->fileName(), contents);
            m_dirtyUic = false;
        }
    }

    m_lastEditor = editor;

    // Handle new editor
    if (isFormWindowEditor(editor))
        connect(editor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));
}

void CMakeProject::editorAboutToClose(Core::IEditor *editor)
{
    if (m_lastEditor == editor) {
        // Oh no our editor is going to be closed
        // get the content first
        if (isFormWindowEditor(m_lastEditor)) {
            disconnect(m_lastEditor, SIGNAL(changed()), this, SLOT(uiEditorContentsChanged()));
            if (m_dirtyUic) {
                const QString contents = formWindowEditorContents(m_lastEditor);
                updateCodeModelSupportFromEditor(m_lastEditor->document()->fileName(), contents);
                m_dirtyUic = false;
            }
        }
        m_lastEditor = 0;
    }
}

void CMakeProject::uiEditorContentsChanged()
{
    // cast sender, get filename
    if (!m_dirtyUic && isFormWindowEditor(sender()))
        m_dirtyUic = true;
}

void CMakeProject::buildStateChanged(ProjectExplorer::Project *project)
{
    if (project == this) {
        if (!ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager()->isBuilding(this)) {
            QMap<QString, CMakeUiCodeModelSupport *>::const_iterator it, end;
            end = m_uiCodeModelSupport.constEnd();
            for (it = m_uiCodeModelSupport.constBegin(); it != end; ++it) {
                it.value()->updateFromBuild();
            }
        }
    }
}

// CMakeFile

CMakeFile::CMakeFile(CMakeProject *parent, QString fileName)
    : Core::IDocument(parent), m_project(parent), m_fileName(fileName)
{

}

bool CMakeFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    // Once we have an texteditor open for this file, we probably do
    // need to implement this, don't we.
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
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
    return QLatin1String(Constants::CMAKEMIMETYPE);
}


bool CMakeFile::isModified() const
{
    return false;
}

bool CMakeFile::isSaveAsAllowed() const
{
    return false;
}

void CMakeFile::rename(const QString &newName)
{
    Q_ASSERT(false);
    Q_UNUSED(newName);
    // Can't happen....
}

Core::IDocument::ReloadBehavior CMakeFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool CMakeFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) : m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    QPushButton *runCmakeButton = new QPushButton(tr("Run cmake"));
    connect(runCmakeButton, SIGNAL(clicked()),
            this, SLOT(runCMake()));
    fl->addRow(tr("Reconfigure project:"), runCmakeButton);

    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_pathLineEdit);

    m_changeButton = new QPushButton(this);
    m_changeButton->setText(tr("&Change"));
    connect(m_changeButton, SIGNAL(clicked()), this, SLOT(openChangeBuildDirectoryDialog()));
    hbox->addWidget(m_changeButton);

    fl->addRow(tr("Build directory:"), hbox);

    m_buildConfiguration = bc;
    m_pathLineEdit->setText(m_buildConfiguration->buildDirectory());
    if (m_buildConfiguration->buildDirectory() == bc->target()->project()->projectDirectory())
        m_changeButton->setEnabled(false);
    else
        m_changeButton->setEnabled(true);

    setDisplayName(tr("CMake"));
}

void CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog()
{
    CMakeProject *project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
    CMakeOpenProjectWizard copw(project->projectManager(), CMakeOpenProjectWizard::ChangeDirectory,
                                CMakeOpenProjectWizard::BuildInfo(m_buildConfiguration));
    if (copw.exec() == QDialog::Accepted) {
        project->changeBuildDirectory(m_buildConfiguration, copw.buildDirectory());
        m_buildConfiguration->setUseNinja(copw.useNinja());
        m_pathLineEdit->setText(m_buildConfiguration->buildDirectory());
    }
}

void CMakeBuildSettingsWidget::runCMake()
{
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->saveModifiedFiles())
        return;
    CMakeProject *project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
    CMakeOpenProjectWizard copw(project->projectManager(),
                                CMakeOpenProjectWizard::WantToUpdate,
                                CMakeOpenProjectWizard::BuildInfo(m_buildConfiguration));
    if (copw.exec() == QDialog::Accepted)
        project->parseCMakeLists();
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
            if (name() == QLatin1String("CodeBlocks_project_file"))
                parseCodeBlocks_project_file();
            else if (isStartElement())
                parseUnknownElement();
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
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Project"))
            parseProject();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseProject()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Option"))
            parseOption();
        else if (name() == QLatin1String("Unit"))
            parseUnit();
        else if (name() == QLatin1String("Build"))
            parseBuild();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuild()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Target"))
            parseBuildTarget();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTarget()
{
    m_buildTarget.clear();

    if (attributes().hasAttribute(QLatin1String("title")))
        m_buildTarget.title = attributes().value(QLatin1String("title")).toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!m_buildTarget.title.endsWith(QLatin1String("/fast")))
                m_buildTargets.append(m_buildTarget);
            return;
        } else if (name() == QLatin1String("Compiler")) {
            parseCompiler();
        } else if (name() == QLatin1String("Option")) {
            parseBuildTargetOption();
        } else if (name() == QLatin1String("MakeCommands")) {
            parseMakeCommands();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTargetOption()
{
    if (attributes().hasAttribute(QLatin1String("output"))) {
        m_buildTarget.executable = attributes().value(QLatin1String("output")).toString();
    } else if (attributes().hasAttribute(QLatin1String("type"))) {
        const QString value = attributes().value(QLatin1String("type")).toString();
        if (value == QLatin1String("2") || value == QLatin1String("3"))
            m_buildTarget.library = true;
    } else if (attributes().hasAttribute(QLatin1String("working_dir"))) {
        m_buildTarget.workingDirectory = attributes().value(QLatin1String("working_dir")).toString();
    }
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

QString CMakeCbpParser::projectName() const
{
    return m_projectName;
}

void CMakeCbpParser::parseOption()
{
    if (attributes().hasAttribute(QLatin1String("title")))
        m_projectName = attributes().value(QLatin1String("title")).toString();

    if (attributes().hasAttribute(QLatin1String("compiler")))
        m_compiler = attributes().value(QLatin1String("compiler")).toString();

    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseMakeCommands()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Build"))
            parseBuildTargetBuild();
        else if (name() == QLatin1String("Clean"))
            parseBuildTargetClean();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTargetBuild()
{
    if (attributes().hasAttribute(QLatin1String("command")))
        m_buildTarget.makeCommand = attributes().value(QLatin1String("command")).toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTargetClean()
{
    if (attributes().hasAttribute(QLatin1String("command")))
        m_buildTarget.makeCleanCommand = attributes().value(QLatin1String("command")).toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseCompiler()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Add"))
            parseAdd();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseAdd()
{
    // CMake only supports <Add option=\> and <Add directory=\>
    const QXmlStreamAttributes addAttributes = attributes();

    const QString includeDirectory = addAttributes.value(QLatin1String("directory")).toString();
    // allow adding multiple times because order happens
    if (!includeDirectory.isEmpty())
        m_includeFiles.append(includeDirectory);

    QString compilerOption = addAttributes.value(QLatin1String("option")).toString();
    // defining multiple times a macro to the same value makes no sense
    if (!compilerOption.isEmpty() && !m_compilerOptions.contains(compilerOption)) {
        m_compilerOptions.append(compilerOption);
        int macroNameIndex = compilerOption.indexOf(QLatin1String("-D")) + 2;
        if (macroNameIndex != 1) {
            int assignIndex = compilerOption.indexOf(QLatin1Char('='), macroNameIndex);
            if (assignIndex != -1)
                compilerOption[assignIndex] = ' ';
            m_defines.append("#define ");
            m_defines.append(compilerOption.mid(macroNameIndex).toUtf8());
            m_defines.append('\n');
        }
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseUnit()
{
    //qDebug()<<stream.attributes().value("filename");
    QString fileName = attributes().value(QLatin1String("filename")).toString();
    m_parsingCmakeUnit = false;
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!fileName.endsWith(QLatin1String(".rule")) && !m_processedUnits.contains(fileName)) {
                // Now check whether we found a virtual element beneath
                if (m_parsingCmakeUnit) {
                    m_cmakeFileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ProjectFileType, false));
                } else {
                    bool generated = false;
                    QString onlyFileName = QFileInfo(fileName).fileName();
                    if (   (onlyFileName.startsWith(QLatin1String("moc_")) && onlyFileName.endsWith(QLatin1String(".cxx")))
                        || (onlyFileName.startsWith(QLatin1String("ui_")) && onlyFileName.endsWith(QLatin1String(".h")))
                        || (onlyFileName.startsWith(QLatin1String("qrc_")) && onlyFileName.endsWith(QLatin1String(".cxx"))))
                        generated = true;

                    if (fileName.endsWith(QLatin1String(".qrc")))
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ResourceType, generated));
                    else
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::SourceType, generated));
                }
                m_processedUnits.insert(fileName);
            }
            return;
        } else if (name() == QLatin1String("Option")) {
            parseUnitOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnitOption()
{
    if (attributes().hasAttribute(QLatin1String("virtualFolder")))
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

QByteArray CMakeCbpParser::defines() const
{
    return m_defines;
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
    library = false;
}
