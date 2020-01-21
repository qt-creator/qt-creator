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
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

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

////////////////////////////////////////////////////////////////////////////////////
//
// GenericProjectFile
//
////////////////////////////////////////////////////////////////////////////////////

class GenericProjectFile : public Core::IDocument
{
public:
    GenericProjectFile(GenericProject *parent, const FilePath &fileName,
                       GenericProject::RefreshOptions options) :
        m_project(parent),
        m_options(options)
    {
        setId("Generic.ProjectFile");
        setMimeType(Constants::GENERICMIMETYPE);
        setFilePath(fileName);
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const final
    {
        return BehaviorSilent;
    }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override
    {
        Q_UNUSED(errorString)
        Q_UNUSED(flag)
        if (type == TypePermissions)
            return true;
        m_project->refresh(m_options);
        return true;
    }

private:
    GenericProject *m_project = nullptr;
    GenericProject::RefreshOptions m_options;
};


////////////////////////////////////////////////////////////////////////////////////
//
// GenericProjectNode
//
////////////////////////////////////////////////////////////////////////////////////

class GenericProjectNode : public ProjectNode
{
public:
    explicit GenericProjectNode(GenericProject *project) :
        ProjectNode(project->projectDirectory()),
        m_project(project)
    {
        setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());
    }

    bool supportsAction(ProjectAction action, const Node *) const override
    {
        return action == AddNewFile
                || action == AddExistingFile
                || action == AddExistingDirectory
                || action == RemoveFile
                || action == Rename;
    }

    bool addFiles(const QStringList &filePaths, QStringList * = nullptr) override
    {
        return m_project->addFiles(filePaths);
    }

    RemovedFilesFromProject removeFiles(const QStringList &filePaths,
                                        QStringList * = nullptr) override
    {
        return m_project->removeFiles(filePaths) ? RemovedFilesFromProject::Ok
                                                 : RemovedFilesFromProject::Error;
    }

    bool renameFile(const QString &filePath, const QString &newFilePath) override
    {
        return m_project->renameFile(filePath, newFilePath);
    }

private:
    GenericProject *m_project = nullptr;
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
    , m_deployFileWatcher(new FileSystemWatcher(this))
{
    setId(Constants::GENERICPROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());

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

    connect(this, &GenericProject::projectFileIsDirty, this, [this](const FilePath &p) {
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

    setExtraProjectFiles({FilePath::fromString(m_filesFileName),
                          FilePath::fromString(m_includesFileName),
                          FilePath::fromString(m_configFileName),
                          FilePath::fromString(m_cxxflagsFileName),
                          FilePath::fromString(m_cflagsFileName)});

    connect(m_deployFileWatcher,
            &FileSystemWatcher::fileChanged,
            this,
            &GenericProject::updateDeploymentData);

    connect(this, &Project::activeTargetChanged, this, [this] { refresh(Everything); });

    connect(this, &Project::activeBuildConfigurationChanged, this, [this] { refresh(Everything); });
}

GenericProject::~GenericProject()
{
    delete m_cppCodeModelUpdater;
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

bool GenericProject::saveRawFileList(const QStringList &rawFileList)
{
    bool result = saveRawList(rawFileList, m_filesFileName);
    refresh(GenericProject::Files);
    return result;
}

bool GenericProject::saveRawList(const QStringList &rawList, const QString &fileName)
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
    bool result = saver.finalize(ICore::mainWindow());
    return result;
}

static void insertSorted(QStringList *list, const QString &value)
{
    int pos = Utils::indexOf(*list, [value](const QString &s) { return s > value; });
    if (pos == -1)
        list->append(value);
    else
        list->insert(pos, value);
}

bool GenericProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    const QDir baseDir(projectDirectory().toString());
    for (const QString &filePath : filePaths)
        insertSorted(&newList, baseDir.relativeFilePath(filePath));

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
    refresh(GenericProject::Everything);

    return result;
}

bool GenericProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    for (const QString &filePath : filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

bool GenericProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(projectDirectory().toString());
    for (const QString &filePath : filePaths)
        newList.append(baseDir.relativeFilePath(filePath));
    Utils::sort(newList);

    return saveRawFileList(newList);
}

bool GenericProject::renameFile(const QString &filePath, const QString &newFilePath)
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
    return QtcProcess::splitArgs(lines.first());
}

void GenericProject::parseProject(RefreshOptions options)
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

FilePath GenericProject::findCommonSourceRoot()
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

void GenericProject::refresh(RefreshOptions options)
{
    ParseGuard guard = guardParsingRun();
    parseProject(options);

    if (options & Files) {
        auto newRoot = std::make_unique<GenericProjectNode>(this);

        // find the common base directory of all source files
        Utils::FilePath baseDir = findCommonSourceRoot();

        for (const QString &f : qAsConst(m_files)) {
            FileType fileType = FileType::Source; // ### FIXME
            if (f.endsWith(".qrc"))
                fileType = FileType::Resource;
            newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(f), fileType), baseDir);
        }

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
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList GenericProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const BuildConfiguration *const buildConfig = activeTarget() ? activeTarget()->activeBuildConfiguration()
                                                                 : nullptr;

    const Utils::Environment buildEnv = buildConfig ? buildConfig->environment()
                                                    : Utils::Environment::systemEnvironment();

    const Utils::MacroExpander *expander = macroExpander();
    if (buildConfig)
        expander = buildConfig->macroExpander();
    else if (activeTarget())
        expander = activeTarget()->macroExpander();

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

void GenericProject::refreshCppCodeModel()
{
    if (!m_cppCodeModelUpdater)
        return;
    QtSupport::CppKitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);

    RawProjectPart rpp;
    rpp.setDisplayName(displayName());
    rpp.setProjectFileLocation(projectFilePath().toString());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    rpp.setHeaderPaths(m_projectIncludePaths);
    rpp.setConfigFileName(m_configFileName);
    rpp.setFlagsForCxx({nullptr, m_cxxflags});
    rpp.setFlagsForC({nullptr, m_cflags});
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({this, kitInfo, activeParseEnvironment(), {rpp}});
}

void GenericProject::updateDeploymentData()
{
    static const QString fileName("QtCreatorDeployment.txt");
    Utils::FilePath deploymentFilePath;
    if (activeTarget() && activeTarget()->activeBuildConfiguration()) {
        deploymentFilePath = activeTarget()->activeBuildConfiguration()->buildDirectory()
                .pathAppended(fileName);
    }
    bool hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    if (!hasDeploymentData) {
        deploymentFilePath = projectDirectory().pathAppended(fileName);
        hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    }
    if (hasDeploymentData) {
        if (activeTarget()) {
            DeploymentData deploymentData;
            deploymentData.addFilesFromDeploymentFile(deploymentFilePath.toString(),
                                                      projectDirectory().toString());
            activeTarget()->setDeploymentData(deploymentData);
        }
        if (m_deployFileWatcher->files() != QStringList(deploymentFilePath.toString())) {
            m_deployFileWatcher->removeFiles(m_deployFileWatcher->files());
            m_deployFileWatcher->addFile(deploymentFilePath.toString(),
                                         FileSystemWatcher::WatchModifiedDate);
        }
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

    refresh(Everything);
    return RestoreResult::Ok;
}

ProjectExplorer::DeploymentKnowledge GenericProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Approximative;
}

} // namespace Internal
} // namespace GenericProjectManager
