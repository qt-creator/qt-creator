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

#include "genericproject.h"

#include "genericbuildconfiguration.h"
#include "genericmakestep.h"
#include "genericprojectconstants.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cpptools/cppprojectupdaterinterface.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/selectablefilesmodel.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QSet>
#include <QStringList>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager {
namespace Internal {

enum RefreshOptions {
    Files         = 0x01,
    Configuration = 0x02,
    Everything    = Files | Configuration
};

////////////////////////////////////////////////////////////////////////////////////
//
// GenericProjectFile
//
////////////////////////////////////////////////////////////////////////////////////

class GenericProjectFile : public Core::IDocument
{
public:
    GenericProjectFile(GenericProject *parent, const FilePath &fileName, RefreshOptions options)
        : m_project(parent), m_options(options)
    {
        setId("Generic.ProjectFile");
        setMimeType(Constants::GENERICMIMETYPE);
        setFilePath(fileName);
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const final
    {
        return BehaviorSilent;
    }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    GenericProject *m_project = nullptr;
    RefreshOptions m_options;
};


////////////////////////////////////////////////////////////////////////////////////
//
// GenericProjectNode
//
////////////////////////////////////////////////////////////////////////////////////

class GenericBuildSystem : public BuildSystem
{
public:
    explicit GenericBuildSystem(Target *target);
    ~GenericBuildSystem();

    void triggerParsing() final;

    bool supportsAction(Node *, ProjectAction action, const Node *) const final
    {
        return action == AddNewFile
                || action == AddExistingFile
                || action == AddExistingDirectory
                || action == RemoveFile
                || action == Rename;
    }

    RemovedFilesFromProject removeFiles(Node *, const QStringList &filePaths, QStringList *) final;
    bool renameFile(Node *, const QString &filePath, const QString &newFilePath) final;
    bool addFiles(Node *, const QStringList &filePaths, QStringList *) final;

    FilePath filesFilePath() const { return ::FilePath::fromString(m_filesFileName); }

    void refresh(RefreshOptions options);

    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parse(RefreshOptions options);

    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = nullptr) const;

    Utils::FilePath findCommonSourceRoot();
    void refreshCppCodeModel();
    void updateDeploymentData();

    bool setFiles(const QStringList &filePaths);
    void removeFiles(const QStringList &filesToRemove);

private:
    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    QString m_cxxflagsFileName;
    QString m_cflagsFileName;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_rawProjectIncludePaths;
    ProjectExplorer::HeaderPaths m_projectIncludePaths;
    QStringList m_cxxflags;
    QStringList m_cflags;

    CppTools::CppProjectUpdaterInterface *m_cppCodeModelUpdater = nullptr;

    Utils::FileSystemWatcher m_deployFileWatcher;
};


////////////////////////////////////////////////////////////////////////////////////
//
// GenericProject
//
////////////////////////////////////////////////////////////////////////////////////

static bool writeFile(const QString &filePath, const QString &contents)
{
    Utils::FileSaver saver(filePath, QIODevice::Text | QIODevice::WriteOnly);
    return saver.write(contents.toUtf8()) && saver.finalize();
}

GenericProject::GenericProject(const Utils::FilePath &fileName)
    : Project(Constants::GENERICMIMETYPE, fileName)
{
    setId(Constants::GENERICPROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());
    setBuildSystemCreator([](Target *t) { return new GenericBuildSystem(t); });
}

GenericBuildSystem::GenericBuildSystem(Target *target)
    : BuildSystem(target)
{
    QObject *projectUpdaterFactory = ExtensionSystem::PluginManager::getObjectByName(
        "CppProjectUpdaterFactory");
    if (projectUpdaterFactory) {
        const bool successFullyCreatedProjectUpdater
            = QMetaObject::invokeMethod(projectUpdaterFactory,
                                        "create",
                                        Q_RETURN_ARG(CppTools::CppProjectUpdaterInterface *,
                                                     m_cppCodeModelUpdater));
        QTC_CHECK(successFullyCreatedProjectUpdater);
    }

    connect(target->project(), &Project::projectFileIsDirty, this, [this](const FilePath &p) {
        if (p.endsWith(".files"))
            refresh(Files);
        else if (p.endsWith(".includes") || p.endsWith(".config") || p.endsWith(".cxxflags")
                 || p.endsWith(".cflags"))
            refresh(Configuration);
        else
            refresh(Everything);
    });

    const QFileInfo fileInfo = projectFilePath().toFileInfo();
    const QDir dir = fileInfo.dir();

    const QString projectName = fileInfo.completeBaseName();

    m_filesFileName    = QFileInfo(dir, projectName + ".files").absoluteFilePath();
    m_includesFileName = QFileInfo(dir, projectName + ".includes").absoluteFilePath();
    m_configFileName = QFileInfo(dir, projectName + ".config").absoluteFilePath();

    const QFileInfo cxxflagsFileInfo(dir, projectName + ".cxxflags");
    m_cxxflagsFileName = cxxflagsFileInfo.absoluteFilePath();
    if (!cxxflagsFileInfo.exists()) {
        QTC_CHECK(writeFile(m_cxxflagsFileName, Constants::GENERICPROJECT_CXXFLAGS_FILE_TEMPLATE));
    }

    const QFileInfo cflagsFileInfo(dir, projectName + ".cflags");
    m_cflagsFileName = cflagsFileInfo.absoluteFilePath();
    if (!cflagsFileInfo.exists()) {
        QTC_CHECK(writeFile(m_cflagsFileName, Constants::GENERICPROJECT_CFLAGS_FILE_TEMPLATE));
    }

    project()->setExtraProjectFiles({FilePath::fromString(m_filesFileName),
                                     FilePath::fromString(m_includesFileName),
                                     FilePath::fromString(m_configFileName),
                                     FilePath::fromString(m_cxxflagsFileName),
                                     FilePath::fromString(m_cflagsFileName)});

    connect(&m_deployFileWatcher, &FileSystemWatcher::fileChanged,
            this, &GenericBuildSystem::updateDeploymentData);

    connect(target, &Target::activeBuildConfigurationChanged, this, [this] { refresh(Everything); });
}

GenericBuildSystem::~GenericBuildSystem()
{
    delete m_cppCodeModelUpdater;
}

void GenericBuildSystem::triggerParsing()
{
    refresh(Everything);
}

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        for (;;) {
            const QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool GenericBuildSystem::saveRawFileList(const QStringList &rawFileList)
{
    bool result = saveRawList(rawFileList, m_filesFileName);
    refresh(Files);
    return result;
}

bool GenericBuildSystem::saveRawList(const QStringList &rawList, const QString &fileName)
{
    FileChangeBlocker changeGuard(fileName);
    // Make sure we can open the file for writing
    Utils::FileSaver saver(fileName, QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        for (const QString &filePath : rawList)
            stream << filePath << '\n';
        saver.setResult(&stream);
    }
    bool result = saver.finalize(ICore::dialogParent());
    return result;
}

static void insertSorted(QStringList *list, const QString &value)
{
    const auto it = std::lower_bound(list->begin(), list->end(), value);
    if (it == list->end())
        list->append(value);
    else if (*it > value)
        list->insert(it, value);
}

bool GenericBuildSystem::addFiles(Node *, const QStringList &filePaths, QStringList *)
{
    const QDir baseDir(projectDirectory().toString());
    QStringList newList = m_rawFileList;
    if (filePaths.size() > m_rawFileList.size()) {
        newList += transform(filePaths, [&baseDir](const QString &p) {
            return baseDir.relativeFilePath(p);
        });
        sort(newList);
        newList.erase(std::unique(newList.begin(), newList.end()), newList.end());
    } else {
        for (const QString &filePath : filePaths)
            insertSorted(&newList, baseDir.relativeFilePath(filePath));
    }

    const auto includes = transform<QSet<QString>>(m_projectIncludePaths,
                                                   [](const HeaderPath &hp) { return hp.path; });
    QSet<QString> toAdd;

    for (const QString &filePath : filePaths) {
        const QString directory = QFileInfo(filePath).absolutePath();
        if (!includes.contains(directory))
            toAdd << directory;
    }

    const QDir dir(projectDirectory().toString());
    const auto candidates = toAdd;
    for (const QString &path : candidates) {
        QString relative = dir.relativeFilePath(path);
        if (relative.isEmpty())
            relative = '.';
        m_rawProjectIncludePaths.append(relative);
    }

    bool result = saveRawList(newList, m_filesFileName);
    result &= saveRawList(m_rawProjectIncludePaths, m_includesFileName);
    refresh(Everything);

    return result;
}

RemovedFilesFromProject GenericBuildSystem::removeFiles(Node *, const QStringList &filePaths, QStringList *)
{
    QStringList newList = m_rawFileList;

    for (const QString &filePath : filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList) ? RemovedFilesFromProject::Ok
                                    : RemovedFilesFromProject::Error;
}

bool GenericBuildSystem::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(projectDirectory().toString());
    for (const QString &filePath : filePaths)
        newList.append(baseDir.relativeFilePath(filePath));
    Utils::sort(newList);

    return saveRawFileList(newList);
}

bool GenericBuildSystem::renameFile(Node *, const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        int index = newList.indexOf(i.value());
        if (index != -1) {
            QDir baseDir(projectDirectory().toString());
            newList.removeAt(index);
            insertSorted(&newList, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

static QStringList readFlags(const QString &filePath)
{
    const QStringList lines = readLines(filePath);
    if (lines.isEmpty())
        return QStringList();
    QStringList flags;
    for (const auto &line : lines)
        flags.append(QtcProcess::splitArgs(line));
    return flags;
}

void GenericBuildSystem::parse(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(m_filesFileName);
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_rawProjectIncludePaths = readLines(m_includesFileName);
        QStringList normalPaths;
        QStringList frameworkPaths;
        for (const QString &rawPath : qAsConst(m_rawProjectIncludePaths)) {
            if (rawPath.startsWith("-F"))
                frameworkPaths << rawPath.mid(2);
            else
                normalPaths << rawPath;
        }
        const auto stringsToHeaderPaths = [this](const QStringList &paths, HeaderPathType type) {
            return transform<HeaderPaths>(processEntries(paths),
                                          [type](const QString &p) { return HeaderPath(p, type);
            });
        };
        m_projectIncludePaths = stringsToHeaderPaths(normalPaths, HeaderPathType::User);
        m_projectIncludePaths << stringsToHeaderPaths(frameworkPaths, HeaderPathType::Framework);
        m_cxxflags = readFlags(m_cxxflagsFileName);
        m_cflags = readFlags(m_cflagsFileName);
    }
}

FilePath GenericBuildSystem::findCommonSourceRoot()
{
    if (m_files.isEmpty())
        return FilePath::fromFileInfo(QFileInfo(m_filesFileName).absolutePath());

    QString root = m_files.front();
    for (const QString &item : qAsConst(m_files)) {
        if (root.length() > item.length())
            root.truncate(item.length());

        for (int i = 0; i < root.length(); ++i) {
            if (root[i] != item[i]) {
                root.truncate(i);
                break;
            }
        }
    }
    return FilePath::fromString(QFileInfo(root).absolutePath());
}

void GenericBuildSystem::refresh(RefreshOptions options)
{
    ParseGuard guard = guardParsingRun();
    parse(options);

    if (options & Files) {
        auto newRoot = std::make_unique<ProjectNode>(projectDirectory());
        newRoot->setDisplayName(projectFilePath().toFileInfo().completeBaseName());

        // find the common base directory of all source files
        FilePath baseDir = findCommonSourceRoot();

        std::vector<std::unique_ptr<FileNode>> fileNodes;
        for (const QString &f : qAsConst(m_files)) {
            FileType fileType = FileType::Source; // ### FIXME
            if (f.endsWith(".qrc"))
                fileType = FileType::Resource;
            fileNodes.emplace_back(std::make_unique<FileNode>(FilePath::fromString(f), fileType));
        }
        newRoot->addNestedNodes(std::move(fileNodes), baseDir);

        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_filesFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_includesFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_configFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_cxxflagsFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_cflagsFileName),
                                                          FileType::Project));

        newRoot->compress();
        setRootProjectNode(std::move(newRoot));
    }

    refreshCppCodeModel();
    updateDeploymentData();
    guard.markAsSuccess();

    emitBuildSystemUpdated();
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList GenericBuildSystem::processEntries(const QStringList &paths,
                                              QHash<QString, QString> *map) const
{
    const BuildConfiguration *const buildConfig = target()->activeBuildConfiguration();

    const Utils::Environment buildEnv = buildConfig ? buildConfig->environment()
                                                    : Utils::Environment::systemEnvironment();

    const Utils::MacroExpander *expander = project()->macroExpander();
    if (buildConfig)
        expander = buildConfig->macroExpander();
    else
        expander = target()->macroExpander();

    const QDir projectDir(projectDirectory().toString());

    QFileInfo fileInfo;
    QStringList absolutePaths;
    for (const QString &path : paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        trimmedPath = buildEnv.expandVariables(trimmedPath);
        trimmedPath = expander->expand(trimmedPath);

        trimmedPath = Utils::FilePath::fromUserInput(trimmedPath).toString();

        fileInfo.setFile(projectDir, trimmedPath);
        if (fileInfo.exists()) {
            const QString absPath = fileInfo.absoluteFilePath();
            absolutePaths.append(absPath);
            if (map)
                map->insert(absPath, trimmedPath);
        }
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

void GenericBuildSystem::refreshCppCodeModel()
{
    if (!m_cppCodeModelUpdater)
        return;
    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return);

    RawProjectPart rpp;
    rpp.setDisplayName(project()->displayName());
    rpp.setProjectFileLocation(projectFilePath().toString());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    rpp.setHeaderPaths(m_projectIncludePaths);
    rpp.setConfigFileName(m_configFileName);
    rpp.setFlagsForCxx({nullptr, m_cxxflags});
    rpp.setFlagsForC({nullptr, m_cflags});
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), {rpp}});
}

void GenericBuildSystem::updateDeploymentData()
{
    static const QString fileName("QtCreatorDeployment.txt");
    Utils::FilePath deploymentFilePath;
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (bc)
        deploymentFilePath = bc->buildDirectory().pathAppended(fileName);

    bool hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    if (!hasDeploymentData) {
        deploymentFilePath = projectDirectory().pathAppended(fileName);
        hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    }
    if (hasDeploymentData) {
        DeploymentData deploymentData;
        deploymentData.addFilesFromDeploymentFile(deploymentFilePath.toString(),
                                                  projectDirectory().toString());
        setDeploymentData(deploymentData);
        if (m_deployFileWatcher.files() != QStringList(deploymentFilePath.toString())) {
            m_deployFileWatcher.removeFiles(m_deployFileWatcher.files());
            m_deployFileWatcher.addFile(deploymentFilePath.toString(),
                                        FileSystemWatcher::WatchModifiedDate);
        }
    }
}

void GenericBuildSystem::removeFiles(const QStringList &filesToRemove)
{
    if (removeFiles(nullptr, filesToRemove, nullptr) == RemovedFilesFromProject::Error) {
        TaskHub::addTask(BuildSystemTask(Task::Error,
                                         GenericProject::tr("Project files list update failed."),
                                         filesFilePath()));
    }
}

Project::RestoreResult GenericProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    const RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    if (!activeTarget())
        addTargetForDefaultKit();

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    const QList<Target *> targetList = targets();
    if (targetList.isEmpty())
        return RestoreResult::Error;

    for (Target *t : targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new CustomExecutableRunConfiguration(t));
    }

    if (Target *t = activeTarget())
        static_cast<GenericBuildSystem *>(t->buildSystem())->refresh(Everything);

    return RestoreResult::Ok;
}

ProjectExplorer::DeploymentKnowledge GenericProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Approximative;
}

bool GenericProjectFile::reload(QString *errorString, IDocument::ReloadFlag flag, IDocument::ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;

    if (Target *t = m_project->activeTarget())
        static_cast<GenericBuildSystem *>(t->buildSystem())->refresh(m_options);

    return true;
}

void GenericProject::editFilesTriggered()
{
    SelectableFilesDialogEditFiles sfd(projectDirectory(),
                                       files(Project::AllFiles),
                                       ICore::dialogParent());
    if (sfd.exec() == QDialog::Accepted) {
        if (Target *t = activeTarget()) {
            auto bs = static_cast<GenericBuildSystem *>(t->buildSystem());
            bs->setFiles(transform(sfd.selectedFiles(), &FilePath::toString));
        }
    }
}

void GenericProject::removeFilesTriggered(const QStringList &filesToRemove)
{
    if (Target *t = activeTarget())
        static_cast<GenericBuildSystem *>(t->buildSystem())->removeFiles(filesToRemove);
}

} // namespace Internal
} // namespace GenericProjectManager
