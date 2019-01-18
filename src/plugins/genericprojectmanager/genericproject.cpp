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

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cppprojectupdater.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
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
    GenericProjectFile(GenericProject *parent, const FileName &fileName,
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
        Q_UNUSED(errorString);
        Q_UNUSED(flag);
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

    bool showInSimpleTree() const override { return true; }

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

    bool removeFiles(const QStringList &filePaths, QStringList * = nullptr) override
    {
        return m_project->removeFiles(filePaths);
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

GenericProject::GenericProject(const Utils::FileName &fileName) :
    Project(Constants::GENERICMIMETYPE, fileName, [this]() { refresh(Everything); }),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater),
    m_deployFileWatcher(new FileSystemWatcher(this))
{
    setId(Constants::GENERICPROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());

    const QFileInfo fileInfo = projectFilePath().toFileInfo();
    const QDir dir = fileInfo.dir();

    const QString projectName = fileInfo.completeBaseName();

    m_filesFileName    = QFileInfo(dir, projectName + ".files").absoluteFilePath();
    m_includesFileName = QFileInfo(dir, projectName + ".includes").absoluteFilePath();
    m_configFileName   = QFileInfo(dir, projectName + ".config").absoluteFilePath();

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

    m_filesIDocument
            = new ProjectDocument(Constants::GENERICMIMETYPE, FileName::fromString(m_filesFileName),
                                  [this]() { refresh(Files); });
    m_includesIDocument
            = new ProjectDocument(Constants::GENERICMIMETYPE, FileName::fromString(m_includesFileName),
                                  [this]() { refresh(Configuration); });
    m_configIDocument
            = new ProjectDocument(Constants::GENERICMIMETYPE, FileName::fromString(m_configFileName),
                                  [this]() { refresh(Configuration); });
    m_cxxFlagsIDocument
            = new ProjectDocument(Constants::GENERICMIMETYPE, FileName::fromString(m_cxxflagsFileName),
                                  [this]() { refresh(Configuration); });
    m_cFlagsIDocument
            = new ProjectDocument(Constants::GENERICMIMETYPE, FileName::fromString(m_cflagsFileName),
                                  [this]() { refresh(Configuration); });

    connect(m_deployFileWatcher, &FileSystemWatcher::fileChanged,
            this, &GenericProject::updateDeploymentData);
}

GenericProject::~GenericProject()
{
    delete m_cppCodeModelUpdater;
    m_filesIDocument->deleteLater();
    m_includesIDocument->deleteLater();
    m_configIDocument->deleteLater();
    m_cxxFlagsIDocument->deleteLater();
    m_cFlagsIDocument->deleteLater();
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

    const QSet<QString> includes = m_projectIncludePaths.toSet();
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
        m_projectIncludePaths = processEntries(m_rawProjectIncludePaths);
        m_cxxflags = readFlags(m_cxxflagsFileName);
        m_cflags = readFlags(m_cflagsFileName);

        // TODO: Possibly load some configuration from the project file
        //QSettings projectInfo(m_fileName, QSettings::IniFormat);
    }
}

void GenericProject::refresh(RefreshOptions options)
{
    emitParsingStarted();
    parseProject(options);

    if (options & Files) {
        auto newRoot = std::make_unique<GenericProjectNode>(this);

        for (const QString &f : m_files) {
            FileType fileType = FileType::Source; // ### FIXME
            if (f.endsWith(".qrc"))
                fileType = FileType::Resource;
            newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(f), fileType,
                                                              false));
        }

        newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(m_filesFileName),
                                                          FileType::Project,
                                                          /* generated = */ false));
        newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(m_includesFileName),
                                                          FileType::Project,
                                                          /* generated = */ false));
        newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(m_configFileName),
                                                          FileType::Project,
                                                          /* generated = */ false));
        newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(m_cxxflagsFileName),
                                                          FileType::Project,
                                                          /* generated = */ false));
        newRoot->addNestedNode(std::make_unique<FileNode>(FileName::fromString(m_cflagsFileName),
                                                          FileType::Project,
                                                          /* generated = */ false));

        setRootProjectNode(std::move(newRoot));
    }

    refreshCppCodeModel();
    updateDeploymentData();
    emitParsingFinished(true);
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

        trimmedPath = Utils::FileName::fromUserInput(trimmedPath).toString();

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
    QtSupport::CppKitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);

    CppTools::RawProjectPart rpp;
    rpp.setDisplayName(displayName());
    rpp.setProjectFileLocation(projectFilePath().toString());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    rpp.setIncludePaths(m_projectIncludePaths);
    rpp.setConfigFileName(m_configFileName);
    rpp.setFlagsForCxx({nullptr, m_cxxflags});
    rpp.setFlagsForC({nullptr, m_cflags});
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({this, kitInfo, {rpp}});
}

void GenericProject::updateDeploymentData()
{
    static const QString fileName("QtCreatorDeployment.txt");
    Utils::FileName deploymentFilePath;
    if (activeTarget() && activeTarget()->activeBuildConfiguration()) {
        deploymentFilePath = activeTarget()->activeBuildConfiguration()->buildDirectory()
                .appendPath(fileName);
    }
    bool hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    if (!hasDeploymentData) {
        deploymentFilePath = projectDirectory().appendPath(fileName);
        hasDeploymentData = QFileInfo::exists(deploymentFilePath.toString());
    }
    if (hasDeploymentData) {
        DeploymentData deploymentData;
        deploymentData.addFilesFromDeploymentFile(deploymentFilePath.toString(),
                                                  projectDirectory().toString());
        activeTarget()->setDeploymentData(deploymentData);
        if (m_deployFileWatcher->files() != QStringList(deploymentFilePath.toString())) {
            m_deployFileWatcher->removeFiles(m_deployFileWatcher->files());
            m_deployFileWatcher->addFile(deploymentFilePath.toString(),
                                         FileSystemWatcher::WatchModifiedDate);
        }
    }
}

void GenericProject::activeTargetWasChanged()
{
    if (m_activeTarget) {
        disconnect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                   this, &GenericProject::activeBuildConfigurationWasChanged);
    }

    m_activeTarget = activeTarget();

    if (!m_activeTarget)
        return;

    connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
            this, &GenericProject::activeBuildConfigurationWasChanged);
    refresh(Everything);
}

void GenericProject::activeBuildConfigurationWasChanged()
{
    refresh(Everything);
}

Project::RestoreResult GenericProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    const RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

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

    m_activeTarget = activeTarget();
    if (m_activeTarget) {
        connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                this, &GenericProject::activeBuildConfigurationWasChanged);
    }

    connect(this, &Project::activeTargetChanged,
            this, &GenericProject::activeTargetWasChanged);
    refresh(Everything);
    return RestoreResult::Ok;
}

} // namespace Internal
} // namespace GenericProjectManager
