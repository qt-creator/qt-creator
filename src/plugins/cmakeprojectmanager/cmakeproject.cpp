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
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "cmakeprojectmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpartbuilder.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <texteditor/textdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QSet>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

using namespace Internal;

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the actual compiler executable
// DEFINES

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(CMakeManager *manager, const FileName &fileName)
{
    setId(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setProjectManager(manager);
    setDocument(new TextEditor::TextDocument);
    document()->setFilePath(fileName);

    setRootProjectNode(new CMakeProjectNode(FileName::fromString(fileName.toFileInfo().absolutePath())));
    setProjectContext(Core::Context(CMakeProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    rootProjectNode()->setDisplayName(fileName.parentDir().fileName());

    connect(this, &CMakeProject::activeTargetChanged, this, &CMakeProject::handleActiveTargetChanged);
}

CMakeProject::~CMakeProject()
{
    setRootProjectNode(nullptr);
    m_codeModelFuture.cancel();
    qDeleteAll(m_extraCompilers);
}

QStringList CMakeProject::getCXXFlagsFor(const CMakeBuildTarget &buildTarget,
                                         QHash<QString, QStringList> &cache)
{
    // check cache:
    auto it = cache.constFind(buildTarget.title);
    if (it != cache.constEnd())
        return *it;

    if (extractCXXFlagsFromMake(buildTarget, cache))
        return cache.value(buildTarget.title);

    if (extractCXXFlagsFromNinja(buildTarget, cache))
        return cache.value(buildTarget.title);

    cache.insert(buildTarget.title, QStringList());
    return QStringList();
}

bool CMakeProject::extractCXXFlagsFromMake(const CMakeBuildTarget &buildTarget,
                                           QHash<QString, QStringList> &cache)
{
    QString makeCommand = QDir::fromNativeSeparators(buildTarget.makeCommand);
    int startIndex = makeCommand.indexOf('\"');
    int endIndex = makeCommand.indexOf('\"', startIndex + 1);
    if (startIndex != -1 && endIndex != -1) {
        startIndex += 1;
        QString makefile = makeCommand.mid(startIndex, endIndex - startIndex);
        int slashIndex = makefile.lastIndexOf('/');
        makefile.truncate(slashIndex);
        makefile.append("/CMakeFiles/" + buildTarget.title + ".dir/flags.make");
        QFile file(makefile);
        if (file.exists()) {
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.startsWith("CXX_FLAGS =")) {
                    // Skip past =
                    cache.insert(buildTarget.title,
                                 line.mid(11).trimmed().split(' ', QString::SkipEmptyParts));
                    return true;
                }
            }
        }
    }
    return false;
}

bool CMakeProject::extractCXXFlagsFromNinja(const CMakeBuildTarget &buildTarget,
                                            QHash<QString, QStringList> &cache)
{
    Q_UNUSED(buildTarget)
    if (!cache.isEmpty()) // We fill the cache in one go!
        return false;

    // Attempt to find build.ninja file and obtain FLAGS (CXX_FLAGS) from there if no suitable flags.make were
    // found
    // Get "all" target's working directory
    QByteArray ninjaFile;
    QString buildNinjaFile = QDir::fromNativeSeparators(buildTargets().at(0).workingDirectory);
    buildNinjaFile += "/build.ninja";
    QFile buildNinja(buildNinjaFile);
    if (buildNinja.exists()) {
        buildNinja.open(QIODevice::ReadOnly | QIODevice::Text);
        ninjaFile = buildNinja.readAll();
        buildNinja.close();
    }

    if (ninjaFile.isEmpty())
        return false;

    QTextStream stream(ninjaFile);
    bool cxxFound = false;
    const QString targetSignature = "# Object build statements for ";
    QString currentTarget;

    while (!stream.atEnd()) {
        // 1. Look for a block that refers to the current target
        // 2. Look for a build rule which invokes CXX_COMPILER
        // 3. Return the FLAGS definition
        QString line = stream.readLine().trimmed();
        if (line.startsWith('#')) {
            if (line.startsWith(targetSignature)) {
                int pos = line.lastIndexOf(' ');
                currentTarget = line.mid(pos + 1);
            }
        } else if (!currentTarget.isEmpty() && line.startsWith("build")) {
            cxxFound = line.indexOf("CXX_COMPILER") != -1;
        } else if (cxxFound && line.startsWith("FLAGS =")) {
            // Skip past =
            cache.insert(currentTarget, line.mid(7).trimmed().split(' ', QString::SkipEmptyParts));
        }
    }
    return !cache.isEmpty();
}

void CMakeProject::updateProjectData()
{
    auto cmakeBc = qobject_cast<CMakeBuildConfiguration *>(sender());
    QTC_ASSERT(cmakeBc, return);

    Target *t = activeTarget();
    if (!t || t->activeBuildConfiguration() != cmakeBc)
        return;
    Kit *k = t->kit();

    BuildDirManager *bdm = cmakeBc->buildDirManager();
    QTC_ASSERT(bdm, return);

    bdm->generateProjectTree(static_cast<CMakeProjectNode *>(rootProjectNode()));

    updateApplicationAndDeploymentTargets();
    updateTargetRunConfigurations(t);

    createGeneratedCodeModelSupport();

    ToolChain *tc = ToolChainKitInformation::toolChain(k, ToolChain::Language::Cxx);
    if (!tc) {
        emit fileListChanged();
        return;
    }

    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CppTools::ProjectInfo pinfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pinfo);

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k)) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    const FileName sysroot = SysRootKitInformation::sysRoot(k);

    ppBuilder.setQtVersion(activeQtVersion);

    QHash<QString, QStringList> targetDataCache;
    foreach (const CMakeBuildTarget &cbt, buildTargets()) {
        if (cbt.targetType == UtilityType)
            continue;

        // CMake shuffles the include paths that it reports via the CodeBlocks generator
        // So remove the toolchain include paths, so that at least those end up in the correct
        // place.
        QStringList cxxflags = getCXXFlagsFor(cbt, targetDataCache);
        QSet<QString> tcIncludes;
        foreach (const HeaderPath &hp, tc->systemHeaderPaths(cxxflags, sysroot)) {
            tcIncludes.insert(hp.path());
        }
        QStringList includePaths;
        foreach (const QString &i, cbt.includeFiles) {
            if (!tcIncludes.contains(i))
                includePaths.append(i);
        }
        includePaths += projectDirectory().toString();
        ppBuilder.setIncludePaths(includePaths);
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

    updateQmlJSCodeModel();

    emit displayNameChanged();
    emit fileListChanged();

    emit cmakeBc->emitBuildTypeChanged();
}

void CMakeProject::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    QTC_ASSERT(modelManager, return);

    if (!activeTarget() || !activeTarget()->activeBuildConfiguration())
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);

    projectInfo.importPaths.clear();

    QString cmakeImports;
    CMakeBuildConfiguration *bc = qobject_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    const QList<ConfigModel::DataItem> &cm = bc->completeCMakeConfiguration();
    foreach (const ConfigModel::DataItem &di, cm) {
        if (di.key.contains(QStringLiteral("QML_IMPORT_PATH"))) {
            cmakeImports = di.value;
            break;
        }
    }

    foreach (const QString &cmakeImport, cmakeImports.split(QLatin1Char(';'))) {
        projectInfo.importPaths.maybeInsert(FileName::fromString(cmakeImport),QmlJS::Dialect::Qml);
    }

    modelManager->updateProjectInfo(projectInfo, this);
}

bool CMakeProject::needsConfiguration() const
{
    return targets().isEmpty();
}

bool CMakeProject::requiresTargetPanel() const
{
    return !targets().isEmpty();
}

bool CMakeProject::knowsAllBuildExecutables() const
{
    return false;
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
    CMakeBuildConfiguration *bc = nullptr;
    if (activeTarget())
        bc = qobject_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());

    if (!bc)
        return;

    BuildDirManager *bdm = bc->buildDirManager();
    if (bdm && !bdm->isParsing()) {
        bdm->checkConfiguration();
        bdm->forceReparse();
    }
}

QList<CMakeBuildTarget> CMakeProject::buildTargets() const
{
    BuildDirManager *bdm = nullptr;
    if (activeTarget() && activeTarget()->activeBuildConfiguration())
        bdm = static_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration())->buildDirManager();
    if (!bdm)
        return QList<CMakeBuildTarget>();
    return bdm->buildTargets();
}

QStringList CMakeProject::buildTargetTitles(bool runnable) const
{
    const QList<CMakeBuildTarget> targets
            = runnable ? filtered(buildTargets(),
                                  [](const CMakeBuildTarget &ct) {
                                      return !ct.executable.isEmpty() && ct.targetType == ExecutableType;
                                  })
                       : buildTargets();
    return transform(targets, [](const CMakeBuildTarget &ct) { return ct.title; });
}

bool CMakeProject::hasBuildTarget(const QString &title) const
{
    return anyOf(buildTargets(), [title](const CMakeBuildTarget &ct) { return ct.title == title; });
}

QString CMakeProject::displayName() const
{
    return rootProjectNode()->displayName();
}

QStringList CMakeProject::files(FilesMode fileMode) const
{
    const QList<FileNode *> nodes = filtered(rootProjectNode()->recursiveFileNodes(),
                                             [fileMode](const FileNode *fn) {
        const bool isGenerated = fn->isGenerated();
        switch (fileMode)
        {
        case Project::SourceFiles:
            return !isGenerated;
        case Project::GeneratedFiles:
            return isGenerated;
        case Project::AllFiles:
        default:
            return true;
        }
    });
    return transform(nodes, [fileMode](const FileNode* fn) { return fn->filePath().toString(); });
}

Project::RestoreResult CMakeProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;
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

void CMakeProject::handleActiveTargetChanged()
{
    if (m_connectedTarget) {
        disconnect(m_connectedTarget, &Target::activeBuildConfigurationChanged,
                   this, &CMakeProject::handleActiveBuildConfigurationChanged);
        disconnect(m_connectedTarget, &Target::kitChanged,
                   this, &CMakeProject::handleActiveBuildConfigurationChanged);
    }

    m_connectedTarget = activeTarget();

    if (m_connectedTarget) {
        connect(m_connectedTarget, &Target::activeBuildConfigurationChanged,
                this, &CMakeProject::handleActiveBuildConfigurationChanged);
        connect(m_connectedTarget, &Target::kitChanged,
                this, &CMakeProject::handleActiveBuildConfigurationChanged);
    }

    handleActiveBuildConfigurationChanged();
}

void CMakeProject::handleActiveBuildConfigurationChanged()
{
    if (!activeTarget() || !activeTarget()->activeBuildConfiguration())
        return;
    auto activeBc = qobject_cast<CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());

    foreach (Target *t, targets()) {
        foreach (BuildConfiguration *bc, t->buildConfigurations()) {
            auto i = qobject_cast<CMakeBuildConfiguration *>(bc);
            QTC_ASSERT(i, continue);
            if (i == activeBc)
                i->maybeForceReparse();
            else
                i->resetData();
        }
    }
}

void CMakeProject::handleParsingStarted()
{
    if (activeTarget() && activeTarget()->activeBuildConfiguration() == sender())
        emit parsingStarted();
}

CMakeBuildTarget CMakeProject::buildTargetForTitle(const QString &title)
{
    foreach (const CMakeBuildTarget &ct, buildTargets())
        if (ct.title == title)
            return ct;
    return CMakeBuildTarget();
}

QStringList CMakeProject::filesGeneratedFrom(const QString &sourceFile) const
{
    if (!activeTarget())
        return QStringList();
    QFileInfo fi(sourceFile);
    FileName project = projectDirectory();
    FileName baseDirectory = FileName::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        FileName cmakeListsTxt = baseDirectory;
        cmakeListsTxt.appendPath("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        QDir dir(baseDirectory.toString());
        dir.cdUp();
        baseDirectory = FileName::fromString(dir.absolutePath());
    }

    QDir srcDirRoot = QDir(project.toString());
    QString relativePath = srcDirRoot.relativeFilePath(baseDirectory.toString());
    QDir buildDir = QDir(activeTarget()->activeBuildConfiguration()->buildDirectory().toString());
    QString generatedFilePath = buildDir.absoluteFilePath(relativePath);

    if (fi.suffix() == "ui") {
        generatedFilePath += "/ui_";
        generatedFilePath += fi.completeBaseName();
        generatedFilePath += ".h";
        return QStringList(QDir::cleanPath(generatedFilePath));
    } else if (fi.suffix() == "scxml") {
        generatedFilePath += "/";
        generatedFilePath += QDir::cleanPath(fi.completeBaseName());
        return QStringList({ generatedFilePath + ".h",
                             generatedFilePath + ".cpp" });
    } else {
        // TODO: Other types will be added when adapters for their compilers become available.
        return QStringList();
    }
}

void CMakeProject::updateTargetRunConfigurations(Target *t)
{
    // *Update* existing runconfigurations (no need to update new ones!):
    QHash<QString, const CMakeBuildTarget *> buildTargetHash;
    const QList<CMakeBuildTarget> buildTargetList = buildTargets();
    foreach (const CMakeBuildTarget &bt, buildTargetList) {
        if (bt.targetType != ExecutableType || bt.executable.isEmpty())
            continue;

        buildTargetHash.insert(bt.title, &bt);
    }

    foreach (RunConfiguration *rc, t->runConfigurations()) {
        auto cmakeRc = qobject_cast<CMakeRunConfiguration *>(rc);
        if (!cmakeRc)
            continue;

        auto btIt = buildTargetHash.constFind(cmakeRc->title());
        cmakeRc->setEnabled(btIt != buildTargetHash.constEnd());
        if (btIt != buildTargetHash.constEnd()) {
            cmakeRc->setExecutable(btIt.value()->executable);
            cmakeRc->setBaseWorkingDirectory(btIt.value()->workingDirectory);
        }
    }

    // create new and remove obsolete RCs using the factories
    t->updateDefaultRunConfigurations();
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

    deploymentFile.setFileName(sourceDir.filePath("QtCreatorDeployment.txt"));
    // If we don't have a global QtCreatorDeployment.txt check for one created by the active build configuration
    if (!deploymentFile.exists())
        deploymentFile.setFileName(buildDir.filePath("QtCreatorDeployment.txt"));
    if (deploymentFile.open(QFile::ReadOnly | QFile::Text)) {
        deploymentStream.setDevice(&deploymentFile);
        deploymentPrefix = deploymentStream.readLine();
        if (!deploymentPrefix.endsWith('/'))
            deploymentPrefix.append('/');
    }

    BuildTargetInfoList appTargetList;
    DeploymentData deploymentData;

    foreach (const CMakeBuildTarget &ct, buildTargets()) {
        if (ct.targetType == UtilityType)
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
    if (!absoluteSourcePath.endsWith('/'))
        absoluteSourcePath.append('/');
    if (deploymentStream.device()) {
        while (!deploymentStream.atEnd()) {
            QString line = deploymentStream.readLine();
            if (!line.contains(':'))
                continue;
            QStringList file = line.split(':');
            deploymentData.addFile(absoluteSourcePath + file.at(0), deploymentPrefix + file.at(1));
        }
    }

    t->setApplicationTargets(appTargetList);
    t->setDeploymentData(deploymentData);
}

void CMakeProject::createGeneratedCodeModelSupport()
{
    qDeleteAll(m_extraCompilers);
    m_extraCompilers.clear();
    QList<ExtraCompilerFactory *> factories =
            ExtraCompilerFactory::extraCompilerFactories();

    // Find all files generated by any of the extra compilers, in a rather crude way.
    foreach (const QString &file, files(SourceFiles)) {
        foreach (ExtraCompilerFactory *factory, factories) {
            if (file.endsWith('.' + factory->sourceTag())) {
                QStringList generated = filesGeneratedFrom(file);
                if (!generated.isEmpty()) {
                    const FileNameList fileNames = transform(generated,
                                                             [](const QString &s) {
                        return FileName::fromString(s);
                    });
                    m_extraCompilers.append(factory->create(this, FileName::fromString(file),
                                                            fileNames));
                }
            }
        }
    }

    CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
}

void CMakeBuildTarget::clear()
{
    executable.clear();
    makeCommand.clear();
    workingDirectory.clear();
    sourceDirectory.clear();
    title.clear();
    targetType = UtilityType;
    includeFiles.clear();
    compilerOptions.clear();
    defines.clear();
}

} // namespace CMakeProjectManager
