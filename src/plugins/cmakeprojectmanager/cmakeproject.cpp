/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakeproject.h"

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "cmakefile.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/icore.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpartbuilder.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/uicodemodelsupport.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileSystemWatcher>
#include <QTemporaryDir>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the actual compiler executable
// DEFINES

// Open Questions
// Who sets up the environment for cl.exe ? INCLUDEPATH and so on

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(CMakeManager *manager, const FileName &fileName)
{
    setId(Constants::CMAKEPROJECT_ID);
    setProjectManager(manager);
    setDocument(new CMakeFile(fileName));
    setRootProjectNode(new CMakeProjectNode(fileName));
    setProjectContext(Core::Context(CMakeProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    rootProjectNode()->setDisplayName(fileName.parentDir().fileName());

    connect(this, &CMakeProject::buildDirectoryDataAvailable, this, &CMakeProject::updateRunConfigurations);
    connect(this, &Project::activeTargetChanged, this, &CMakeProject::activeTargetHasChanged);
    connect(this, &CMakeProject::environmentChanged, this, [this]() {
        BuildConfiguration *bc = nullptr;
        if (activeTarget())
            bc = activeTarget()->activeBuildConfiguration();
        changeActiveBuildConfiguration(bc); // Does a clean reset of the builddirmanager
    });

    connect(this, &Project::addedTarget, this, [this](Target *t) {
        connect(t, &Target::kitChanged, this, &CMakeProject::handleKitChanges);
    });
}

CMakeProject::~CMakeProject()
{
    setRootProjectNode(nullptr);
    m_codeModelFuture.cancel();
    delete m_buildDirManager;
}

void CMakeProject::changeActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    if (m_buildDirManager) {
        m_buildDirManager->disconnect();
        m_buildDirManager->deleteLater();
    }
    m_buildDirManager = nullptr;

    Kit *k = nullptr;
    CMakeConfig config;
    Utils::FileName buildDir;

    CMakeBuildConfiguration *cmakebc = qobject_cast<CMakeBuildConfiguration *>(bc);
    if (!cmakebc) {
        k = KitManager::defaultKit();
        config = CMakeConfigurationKitInformation::configuration(k);
    } else {
        k = cmakebc->target()->kit();
        // FIXME: Fill config with data from cmakebc!
        buildDir = cmakebc->buildDirectory();
    }
    if (k) {
        m_buildDirManager = new Internal::BuildDirManager(projectDirectory(), k, config,
                                                          cmakebc->environment(), buildDir);
        connect(m_buildDirManager, &BuildDirManager::parsingStarted,
                this, &CMakeProject::parsingStarted);
        connect(m_buildDirManager, &BuildDirManager::dataAvailable,
                this, &CMakeProject::parseCMakeOutput);
    }
}

void CMakeProject::activeTargetHasChanged(Target *target)
{
    if (m_activeTarget) {
        disconnect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                   this, &CMakeProject::changeActiveBuildConfiguration);
    }

    m_activeTarget = target;

    if (!m_activeTarget)
        return;

    connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
            this, &CMakeProject::changeActiveBuildConfiguration);
    changeActiveBuildConfiguration(m_activeTarget->activeBuildConfiguration());
}

void CMakeProject::changeBuildDirectory(CMakeBuildConfiguration *bc, const QString &newBuildDirectory)
{
    bc->setBuildDirectory(FileName::fromString(newBuildDirectory));
    if (activeTarget() && activeTarget()->activeBuildConfiguration() == bc)
        changeActiveBuildConfiguration(bc);
}

void CMakeProject::handleKitChanges()
{
    const Target *t = qobject_cast<Target *>(sender());
    if (t && t != activeTarget())
        return;
    changeActiveBuildConfiguration(t->activeBuildConfiguration()); // force proper refresh
}

QStringList CMakeProject::getCXXFlagsFor(const CMakeBuildTarget &buildTarget, QByteArray *cachedBuildNinja)
{
    QString makeCommand = QDir::fromNativeSeparators(buildTarget.makeCommand);
    int startIndex = makeCommand.indexOf(QLatin1Char('\"'));
    int endIndex = makeCommand.indexOf(QLatin1Char('\"'), startIndex + 1);
    if (startIndex != -1 && endIndex != -1) {
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
                    return line.mid(11).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
                }
            }
        }
    }

    // Attempt to find build.ninja file and obtain FLAGS (CXX_FLAGS) from there if no suitable flags.make were
    // found
    // Get "all" target's working directory
    if (!buildTargets().empty()) {
        if (cachedBuildNinja->isNull()) {
            QString buildNinjaFile = QDir::fromNativeSeparators(buildTargets().at(0).workingDirectory);
            buildNinjaFile += QLatin1String("/build.ninja");
            QFile buildNinja(buildNinjaFile);
            if (buildNinja.exists()) {
                buildNinja.open(QIODevice::ReadOnly | QIODevice::Text);
                *cachedBuildNinja = buildNinja.readAll();
                buildNinja.close();
            } else {
                *cachedBuildNinja = QByteArray();
            }
        }

        if (cachedBuildNinja->isEmpty())
            return QStringList();

        QTextStream stream(cachedBuildNinja);
        bool targetFound = false;
        bool cxxFound = false;
        QString targetSearchPattern = QString::fromLatin1("target %1").arg(buildTarget.title);

        while (!stream.atEnd()) {
            // 1. Look for a block that refers to the current target
            // 2. Look for a build rule which invokes CXX_COMPILER
            // 3. Return the FLAGS definition
            QString line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("#"))) {
                if (!line.startsWith(QLatin1String("# Object build statements for"))) continue;
                targetFound = line.endsWith(targetSearchPattern);
            } else if (targetFound && line.startsWith(QLatin1String("build"))) {
                cxxFound = line.indexOf(QLatin1String("CXX_COMPILER")) != -1;
            } else if (cxxFound && line.startsWith(QLatin1String("FLAGS ="))) {
                // Skip past =
                return line.mid(7).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
            }
        }

    }
    return QStringList();
}

void CMakeProject::parseCMakeOutput()
{
    QTC_ASSERT(m_buildDirManager, return);
    QTC_ASSERT(activeTarget() && activeTarget()->activeBuildConfiguration(), return);

    auto activeBC = static_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());

    rootProjectNode()->setDisplayName(m_buildDirManager->projectName());

    buildTree(static_cast<CMakeProjectNode *>(rootProjectNode()), m_buildDirManager->files());

    updateApplicationAndDeploymentTargets();

    createUiCodeModelSupport();

    ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_buildDirManager->kit());
    if (!tc) {
        emit buildDirectoryDataAvailable(activeBC);
        emit fileListChanged();
        return;
    }

    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CppTools::ProjectInfo pinfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pinfo);

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(m_buildDirManager->kit())) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    ppBuilder.setQtVersion(activeQtVersion);

    QByteArray cachedBuildNinja;
    foreach (const CMakeBuildTarget &cbt, buildTargets()) {
        // This explicitly adds -I. to the include paths
        QStringList includePaths = cbt.includeFiles;
        includePaths += projectDirectory().toString();
        ppBuilder.setIncludePaths(includePaths);
        QStringList cxxflags = getCXXFlagsFor(cbt, &cachedBuildNinja);
        ppBuilder.setCFlags(cxxflags);
        ppBuilder.setCxxFlags(cxxflags);
        ppBuilder.setDefines(cbt.defines);
        ppBuilder.setDisplayName(cbt.title);

        const QList<Core::Id> languages = ppBuilder.createProjectPartsForFiles(cbt.files);
        foreach (Core::Id language, languages)
            setProjectLanguage(language, true);
    }

    m_codeModelFuture.cancel();
    pinfo.finish();
    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);

    emit displayNameChanged();
    emit buildDirectoryDataAvailable(activeBC);
    emit fileListChanged();

    emit activeBC->emitBuildTypeChanged();
}

bool CMakeProject::needsConfiguration() const
{
    return targets().isEmpty();
}

bool CMakeProject::requiresTargetPanel() const
{
    return !targets().isEmpty();
}

bool CMakeProject::supportsKit(Kit *k, QString *errorMessage) const
{
    if (!CMakeKitInformation::cmakeTool(k)) {
        if (errorMessage)
            *errorMessage = tr("No cmake tool set.");
        return false;
    }
    return true;
}

void CMakeProject::runCMake()
{
    if (m_buildDirManager && !m_buildDirManager->isBusy())
        m_buildDirManager->forceReparse();
}

bool CMakeProject::isParsing() const
{
    return m_buildDirManager && m_buildDirManager->isBusy();
}

bool CMakeProject::isProjectFile(const FileName &fileName)
{
    if (!m_buildDirManager)
        return false;
    return m_buildDirManager->isProjectFile(fileName);
}

QList<CMakeBuildTarget> CMakeProject::buildTargets() const
{
    if (!m_buildDirManager)
        return QList<CMakeBuildTarget>();
    return m_buildDirManager->buildTargets();
}

QStringList CMakeProject::buildTargetTitles(bool runnable) const
{
    const QList<CMakeBuildTarget> targets
            = runnable ? Utils::filtered(buildTargets(),
                                         [](const CMakeBuildTarget &ct) {
                                             return !ct.executable.isEmpty() && ct.targetType == ExecutableType;
                                         })
                       : buildTargets();
    return Utils::transform(targets, [](const CMakeBuildTarget &ct) { return ct.title; });
}

bool CMakeProject::hasBuildTarget(const QString &title) const
{
    return Utils::anyOf(buildTargets(), [title](const CMakeBuildTarget &ct) { return ct.title == title; });
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
    return a->filePath() < b->filePath();
}

void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> newList)
{
    // Gather old list
    QList<ProjectExplorer::FileNode *> oldList;
    gatherFileNodes(rootNode, oldList);
    Utils::sort(oldList, sortNodesByPath);
    Utils::sort(newList, sortNodesByPath);

    QList<ProjectExplorer::FileNode *> added;
    QList<ProjectExplorer::FileNode *> deleted;

    ProjectExplorer::compareSortedLists(oldList, newList, deleted, added, sortNodesByPath);

    qDeleteAll(ProjectExplorer::subtractSortedList(newList, added, sortNodesByPath));

    // add added nodes
    foreach (ProjectExplorer::FileNode *fn, added) {
        // Get relative path to rootNode
        QString parentDir = fn->filePath().toFileInfo().absolutePath();
        ProjectExplorer::FolderNode *folder = findOrCreateFolder(rootNode, parentDir);
        folder->addFileNodes(QList<ProjectExplorer::FileNode *>()<< fn);
    }

    // remove old file nodes and check whether folder nodes can be removed
    foreach (ProjectExplorer::FileNode *fn, deleted) {
        ProjectExplorer::FolderNode *parent = fn->parentFolderNode();
        parent->removeFileNodes(QList<ProjectExplorer::FileNode *>() << fn);
        // Check for empty parent
        while (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty()) {
            ProjectExplorer::FolderNode *grandparent = parent->parentFolderNode();
            grandparent->removeFolderNodes(QList<ProjectExplorer::FolderNode *>() << parent);
            parent = grandparent;
            if (parent == rootNode)
                break;
        }
    }
}

ProjectExplorer::FolderNode *CMakeProject::findOrCreateFolder(CMakeProjectNode *rootNode, QString directory)
{
    FileName path = rootNode->filePath().parentDir();
    QDir rootParentDir(path.toString());
    QString relativePath = rootParentDir.relativeFilePath(directory);
    if (relativePath == QLatin1String("."))
        relativePath.clear();
    QStringList parts = relativePath.split(QLatin1Char('/'), QString::SkipEmptyParts);
    ProjectExplorer::FolderNode *parent = rootNode;
    foreach (const QString &part, parts) {
        path.appendPath(part);
        // Find folder in subFolders
        bool found = false;
        foreach (ProjectExplorer::FolderNode *folder, parent->subFolderNodes()) {
            if (folder->filePath() == path) {
                // yeah found something :)
                parent = folder;
                found = true;
                break;
            }
        }
        if (!found) {
            // No FolderNode yet, so create it
            auto tmp = new ProjectExplorer::FolderNode(path);
            tmp->setDisplayName(part);
            parent->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << tmp);
            parent = tmp;
        }
    }
    return parent;
}

QString CMakeProject::displayName() const
{
    return rootProjectNode()->displayName();
}

QStringList CMakeProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files;
}

Project::RestoreResult CMakeProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    m_activeTarget = activeTarget();
    if (m_activeTarget) {
        connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                this, &CMakeProject::changeActiveBuildConfiguration);
        if (BuildConfiguration *bc = m_activeTarget->activeBuildConfiguration())
            changeActiveBuildConfiguration(bc);
    }

    return RestoreResult::Ok;
}

bool CMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();

    return true;
}

CMakeBuildTarget CMakeProject::buildTargetForTitle(const QString &title)
{
    foreach (const CMakeBuildTarget &ct, buildTargets())
        if (ct.title == title)
            return ct;
    return CMakeBuildTarget();
}

QString CMakeProject::uiHeaderFile(const QString &uiFile)
{
    if (!activeTarget())
        return QString();
    QFileInfo fi(uiFile);
    FileName project = projectDirectory();
    FileName baseDirectory = FileName::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        FileName cmakeListsTxt = baseDirectory;
        cmakeListsTxt.appendPath(QLatin1String("CMakeLists.txt"));
        if (cmakeListsTxt.exists())
            break;
        QDir dir(baseDirectory.toString());
        dir.cdUp();
        baseDirectory = FileName::fromString(dir.absolutePath());
    }

    QDir srcDirRoot = QDir(project.toString());
    QString relativePath = srcDirRoot.relativeFilePath(baseDirectory.toString());
    QDir buildDir = QDir(activeTarget()->activeBuildConfiguration()->buildDirectory().toString());
    QString uiHeaderFilePath = buildDir.absoluteFilePath(relativePath);
    uiHeaderFilePath += QLatin1String("/ui_");
    uiHeaderFilePath += fi.completeBaseName();
    uiHeaderFilePath += QLatin1String(".h");

    return QDir::cleanPath(uiHeaderFilePath);
}

void CMakeProject::updateRunConfigurations()
{
    foreach (Target *t, targets())
        updateTargetRunConfigurations(t);
}

// TODO Compare with updateDefaultRunConfigurations();
void CMakeProject::updateTargetRunConfigurations(Target *t)
{
    // create new and remove obsolete RCs using the factories
    t->updateDefaultRunConfigurations();

    // *Update* runconfigurations:
    QMultiMap<QString, CMakeRunConfiguration*> existingRunConfigurations;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations()) {
        if (CMakeRunConfiguration* cmakeRC = qobject_cast<CMakeRunConfiguration *>(rc))
            existingRunConfigurations.insert(cmakeRC->title(), cmakeRC);
    }

    foreach (const CMakeBuildTarget &ct, buildTargets()) {
        if (ct.targetType != ExecutableType)
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
        }
    }

    if (t->runConfigurations().isEmpty()) {
        // Oh no, no run configuration,
        // create a custom executable run configuration
        t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    }
}

void CMakeProject::updateApplicationAndDeploymentTargets()
{
    Target *t = activeTarget();
    if (!t)
        return;

    QFile deploymentFile;
    QTextStream deploymentStream;
    QString deploymentPrefix;

    QDir sourceDir(t->project()->projectDirectory().toString());
    QDir buildDir(t->activeBuildConfiguration()->buildDirectory().toString());

    deploymentFile.setFileName(sourceDir.filePath(QLatin1String("QtCreatorDeployment.txt")));
    // If we don't have a global QtCreatorDeployment.txt check for one created by the active build configuration
    if (!deploymentFile.exists())
        deploymentFile.setFileName(buildDir.filePath(QLatin1String("QtCreatorDeployment.txt")));
    if (deploymentFile.open(QFile::ReadOnly | QFile::Text)) {
        deploymentStream.setDevice(&deploymentFile);
        deploymentPrefix = deploymentStream.readLine();
        if (!deploymentPrefix.endsWith(QLatin1Char('/')))
            deploymentPrefix.append(QLatin1Char('/'));
    }

    BuildTargetInfoList appTargetList;
    DeploymentData deploymentData;

    foreach (const CMakeBuildTarget &ct, buildTargets()) {
        if (ct.executable.isEmpty())
            continue;

        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType)
            deploymentData.addFile(ct.executable, deploymentPrefix + buildDir.relativeFilePath(QFileInfo(ct.executable).dir().path()), DeployableFile::TypeExecutable);
        if (ct.targetType == ExecutableType) {
            // TODO: Put a path to corresponding .cbp file into projectFilePath?
            appTargetList.list << BuildTargetInfo(ct.title,
                                                  FileName::fromString(ct.executable),
                                                  FileName::fromString(ct.executable));
        }
    }

    QString absoluteSourcePath = sourceDir.absolutePath();
    if (!absoluteSourcePath.endsWith(QLatin1Char('/')))
        absoluteSourcePath.append(QLatin1Char('/'));
    if (deploymentStream.device()) {
        while (!deploymentStream.atEnd()) {
            QString line = deploymentStream.readLine();
            if (!line.contains(QLatin1Char(':')))
                continue;
            QStringList file = line.split(QLatin1Char(':'));
            deploymentData.addFile(absoluteSourcePath + file.at(0), deploymentPrefix + file.at(1));
        }
    }

    t->setApplicationTargets(appTargetList);
    t->setDeploymentData(deploymentData);
}

void CMakeProject::createUiCodeModelSupport()
{
    QHash<QString, QString> uiFileHash;

    // Find all ui files
    foreach (const QString &uiFile, m_files) {
        if (uiFile.endsWith(QLatin1String(".ui")))
            uiFileHash.insert(uiFile, uiHeaderFile(uiFile));
    }

    QtSupport::UiCodeModelManager::update(this, uiFileHash);
}

void CMakeBuildTarget::clear()
{
    executable.clear();
    makeCommand.clear();
    makeCleanCommand.clear();
    workingDirectory.clear();
    sourceDirectory.clear();
    title.clear();
    targetType = ExecutableType;
    includeFiles.clear();
    compilerOptions.clear();
    defines.clear();
}
