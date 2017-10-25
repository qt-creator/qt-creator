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

#include "servermodereader.h"

#include "cmakebuildconfiguration.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "servermode.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/asconst.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QVector>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

const char CACHE_TYPE[] = "cache";
const char CODEMODEL_TYPE[] = "codemodel";
const char CONFIGURE_TYPE[] = "configure";
const char CMAKE_INPUTS_TYPE[] = "cmakeInputs";
const char COMPUTE_TYPE[] = "compute";

const char BACKTRACE_KEY[] = "backtrace";
const char LINE_KEY[] = "line";
const char NAME_KEY[] = "name";
const char PATH_KEY[] = "path";
const char SOURCE_DIRECTORY_KEY[] = "sourceDirectory";
const char SOURCES_KEY[] = "sources";

const int MAX_PROGRESS = 1400;

// --------------------------------------------------------------------
// ServerModeReader:
// --------------------------------------------------------------------

ServerModeReader::ServerModeReader()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::aboutToSave,
            this, [this](const Core::IDocument *document) {
        if (m_cmakeFiles.contains(document->filePath()))
            emit dirty();
    });

    connect(&m_parser, &CMakeParser::addOutput,
            this, [](const QString &m) { Core::MessageManager::write(m); });
    connect(&m_parser, &CMakeParser::addTask, this, [this](const Task &t) {
        Task editable(t);
        if (!editable.file.isEmpty()) {
            QDir srcDir(m_parameters.sourceDirectory.toString());
            editable.file = FileName::fromString(srcDir.absoluteFilePath(editable.file.toString()));
        }
        TaskHub::addTask(editable);
    });
}

ServerModeReader::~ServerModeReader()
{
    stop();
}

void ServerModeReader::setParameters(const BuildDirParameters &p)
{
    QTC_ASSERT(p.cmakeTool, return);

    BuildDirReader::setParameters(p);
    if (!m_cmakeServer) {
        m_cmakeServer.reset(new ServerMode(p.environment,
                                           p.sourceDirectory, p.workDirectory,
                                           p.cmakeTool->cmakeExecutable(),
                                           p.generator, p.extraGenerator, p.platform, p.toolset,
                                           true, 1));
        connect(m_cmakeServer.get(), &ServerMode::errorOccured,
                this, &ServerModeReader::errorOccured);
        connect(m_cmakeServer.get(), &ServerMode::cmakeReply,
                this, &ServerModeReader::handleReply);
        connect(m_cmakeServer.get(), &ServerMode::cmakeError,
                this, &ServerModeReader::handleError);
        connect(m_cmakeServer.get(), &ServerMode::cmakeProgress,
                this, &ServerModeReader::handleProgress);
        connect(m_cmakeServer.get(), &ServerMode::cmakeSignal,
                this, &ServerModeReader::handleSignal);
        connect(m_cmakeServer.get(), &ServerMode::cmakeMessage, [this](const QString &m) {
            const QStringList lines = m.split('\n');
            for (const QString &l : lines) {
                m_parser.stdError(l);
                Core::MessageManager::write(l);
            }
        });
        connect(m_cmakeServer.get(), &ServerMode::message,
                this, [](const QString &m) { Core::MessageManager::write(m); });
        connect(m_cmakeServer.get(), &ServerMode::connected,
                this, &ServerModeReader::isReadyNow, Qt::QueuedConnection); // Delay
        connect(m_cmakeServer.get(), &ServerMode::disconnected,
                this, [this]() {
            stop();
            Core::MessageManager::write(tr("Parsing of CMake project failed: Connection to CMake server lost."));
            m_cmakeServer.reset();
        }, Qt::QueuedConnection); // Delay
    }
}

bool ServerModeReader::isCompatible(const BuildDirParameters &p)
{
    if (!p.cmakeTool)
        return false;

    // Server mode connection got lost, reset...
    if (!m_parameters.cmakeTool->cmakeExecutable().isEmpty() && !m_cmakeServer)
        return false;

    return p.cmakeTool->hasServerMode()
            && p.cmakeTool->cmakeExecutable() == m_parameters.cmakeTool->cmakeExecutable()
            && p.environment == m_parameters.environment
            && p.generator == m_parameters.generator
            && p.extraGenerator == m_parameters.extraGenerator
            && p.platform == m_parameters.platform
            && p.toolset == m_parameters.toolset
            && p.sourceDirectory == m_parameters.sourceDirectory
            && p.workDirectory == m_parameters.workDirectory;
}

void ServerModeReader::resetData()
{
    m_cmakeConfiguration.clear();
    // m_cmakeFiles: Keep these!
    m_cmakeInputsFileNodes.clear();
    qDeleteAll(m_projects); // Also deletes targets and filegroups that are its children!
    m_projects.clear();
    m_targets.clear();
    m_fileGroups.clear();
}

void ServerModeReader::parse(bool forceConfiguration)
{
    emit configurationStarted();

    QTC_ASSERT(m_cmakeServer, return);
    QVariantMap extra;
    if (forceConfiguration || !QDir(m_parameters.buildDirectory.toString()).exists("CMakeCache.txt")) {
        QStringList cacheArguments = transform(m_parameters.configuration,
                                               [this](const CMakeConfigItem &i) {
            return i.toArgument(m_parameters.expander);
        });
        Core::MessageManager::write(tr("Starting to parse CMake project, using: \"%1\".")
                                    .arg(cacheArguments.join("\", \"")));
        cacheArguments.prepend(QString()); // Work around a bug in CMake 3.7.0 and 3.7.1 where
                                           // the first argument gets lost!
        extra.insert("cacheArguments", QVariant(cacheArguments));
    } else {
        Core::MessageManager::write(tr("Starting to parse CMake project."));
    }

    m_future.reset(new QFutureInterface<void>());
    m_future->setProgressRange(0, MAX_PROGRESS);
    m_progressStepMinimum = 0;
    m_progressStepMaximum = 1000;
    Core::ProgressManager::addTask(m_future->future(),
                                   tr("Configuring \"%1\"").arg(m_parameters.projectName),
                                   "CMake.Configure");

    m_cmakeServer->sendRequest(CONFIGURE_TYPE, extra);
}

void ServerModeReader::stop()
{
    if (m_future) {
        m_future->reportCanceled();
        m_future->reportFinished();
        m_future.reset();
    }
    m_parser.flush();
}

bool ServerModeReader::isReady() const
{
    return m_cmakeServer->isConnected();
}

bool ServerModeReader::isParsing() const
{
    return static_cast<bool>(m_future);
}

QList<CMakeBuildTarget> ServerModeReader::takeBuildTargets()
{
    const QList<CMakeBuildTarget> result = transform(m_targets, [](const Target *t) -> CMakeBuildTarget {
        CMakeBuildTarget ct;
        ct.title = t->name;
        ct.executable = t->artifacts.isEmpty() ? FileName() : t->artifacts.at(0);
        TargetType type = UtilityType;
        if (t->type == "EXECUTABLE")
            type = ExecutableType;
        else if (t->type == "STATIC_LIBRARY")
            type = StaticLibraryType;
        else if (t->type == "MODULE_LIBRARY"
                 || t->type == "SHARED_LIBRARY"
                 || t->type == "INTERFACE_LIBRARY"
                 || t->type == "OBJECT_LIBRARY")
            type = DynamicLibraryType;
        else
            type = UtilityType;
        ct.targetType = type;
        if (t->artifacts.isEmpty()) {
            ct.workingDirectory = t->buildDirectory;
        } else {
            ct.workingDirectory = Utils::FileName::fromString(t->artifacts.at(0).toFileInfo().absolutePath());
        }
        ct.sourceDirectory = t->sourceDirectory;
        return ct;
    });
    m_targets.clear();
    return result;
}

CMakeConfig ServerModeReader::takeParsedConfiguration()
{
    CMakeConfig config = m_cmakeConfiguration;
    m_cmakeConfiguration.clear();
    return config;
}

static void addCMakeVFolder(FolderNode *base, const Utils::FileName &basePath, int priority,
                     const QString &displayName, const QList<FileNode *> &files)
{
    if (files.isEmpty())
        return;
    FolderNode *folder = base;
    if (!displayName.isEmpty()) {
        folder = new VirtualFolderNode(basePath, priority);
        folder->setDisplayName(displayName);
        base->addNode(folder);
    }
    folder->addNestedNodes(files);
    for (FolderNode *fn : folder->folderNodes())
        fn->compress();
}

static QList<FileNode *> removeKnownNodes(const QSet<Utils::FileName> &knownFiles, const QList<FileNode *> &files)
{
    return Utils::filtered(files, [&knownFiles](const FileNode *n) {
        if (knownFiles.contains(n->filePath())) {
            delete n;
            return false;
        }
        return true;
    });
}

static void addCMakeInputs(FolderNode *root,
                           const Utils::FileName &sourceDir,
                           const Utils::FileName &buildDir,
                           QList<FileNode *> &sourceInputs,
                           QList<FileNode *> &buildInputs,
                           QList<FileNode *> &rootInputs)
{
    ProjectNode *cmakeVFolder = new CMakeInputsNode(root->filePath());
    root->addNode(cmakeVFolder);

    QSet<Utils::FileName> knownFiles;
    root->forEachGenericNode([&knownFiles](const Node *n) {
        if (n->listInProject())
            knownFiles.insert(n->filePath());
    });

    addCMakeVFolder(cmakeVFolder, sourceDir, 1000, QString(), removeKnownNodes(knownFiles, sourceInputs));
    addCMakeVFolder(cmakeVFolder, buildDir, 100,
                    QCoreApplication::translate("CMakeProjectManager::Internal::ServerModeReader", "<Build Directory>"),
                    removeKnownNodes(knownFiles, buildInputs));
    addCMakeVFolder(cmakeVFolder, Utils::FileName(), 10,
                    QCoreApplication::translate("CMakeProjectManager::Internal::ServerModeReader", "<Other Locations>"),
                    removeKnownNodes(knownFiles, rootInputs));
}

void ServerModeReader::generateProjectTree(CMakeProjectNode *root,
                                           const QList<const FileNode *> &allFiles)
{
    // Split up cmake inputs into useful chunks:
    QList<FileNode *> cmakeFilesSource;
    QList<FileNode *> cmakeFilesBuild;
    QList<FileNode *> cmakeFilesOther;
    QList<FileNode *> cmakeLists;

    foreach (FileNode *fn, m_cmakeInputsFileNodes) {
        const FileName path = fn->filePath();
        if (path.fileName().compare("CMakeLists.txt", HostOsInfo::fileNameCaseSensitivity()) == 0)
            cmakeLists.append(fn);
        else if (path.isChildOf(m_parameters.buildDirectory))
            cmakeFilesBuild.append(fn);
        else if (path.isChildOf(m_parameters.sourceDirectory))
            cmakeFilesSource.append(fn);
        else
            cmakeFilesOther.append(fn);
    }
    m_cmakeInputsFileNodes.clear(); // Clean out, they are not going to be used anymore!

    const Project *topLevel = Utils::findOrDefault(m_projects, [this](const Project *p) {
        return m_parameters.sourceDirectory == p->sourceDirectory;
    });
    if (topLevel)
        root->setDisplayName(topLevel->name);

    QHash<Utils::FileName, ProjectNode *> cmakeListsNodes = addCMakeLists(root, cmakeLists);
    QList<FileNode *> knownHeaders;
    addProjects(cmakeListsNodes, m_projects, knownHeaders);

    addHeaderNodes(root, knownHeaders, allFiles);

    if (!cmakeFilesSource.isEmpty() || !cmakeFilesBuild.isEmpty() || !cmakeFilesOther.isEmpty())
        addCMakeInputs(root, m_parameters.sourceDirectory, m_parameters.buildDirectory,
                       cmakeFilesSource, cmakeFilesBuild, cmakeFilesOther);
}

void ServerModeReader::updateCodeModel(CppTools::RawProjectParts &rpps)
{
    int counter = 0;
    for (const FileGroup *fg : Utils::asConst(m_fileGroups)) {
        ++counter;
        const QStringList flags = QtcProcess::splitArgs(fg->compileFlags);
        const QStringList includes = transform(fg->includePaths, [](const IncludePath *ip)  { return ip->path.toString(); });

        CppTools::RawProjectPart rpp;
        rpp.setProjectFileLocation(fg->target->sourceDirectory.toString() + "/CMakeLists.txt");
        rpp.setBuildSystemTarget(fg->target->name);
        rpp.setDisplayName(fg->target->name + QString::number(counter));
        rpp.setMacros(fg->macros);
        rpp.setIncludePaths(includes);

        CppTools::RawProjectPartFlags cProjectFlags;
        cProjectFlags.commandLineFlags = flags;
        rpp.setFlagsForC(cProjectFlags);

        CppTools::RawProjectPartFlags cxxProjectFlags;
        cxxProjectFlags.commandLineFlags = flags;
        rpp.setFlagsForCxx(cxxProjectFlags);

        rpp.setFiles(transform(fg->sources, &FileName::toString));

        const bool isExecutable = fg->target->type == "EXECUTABLE";
        rpp.setBuildTargetType(isExecutable ? CppTools::ProjectPart::Executable
                                            : CppTools::ProjectPart::Library);
        rpps.append(rpp);
    }
}

void ServerModeReader::handleReply(const QVariantMap &data, const QString &inReplyTo)
{
    Q_UNUSED(data);
    if (inReplyTo == CONFIGURE_TYPE) {
        m_cmakeServer->sendRequest(COMPUTE_TYPE);
        if (m_future)
            m_future->setProgressValue(1000);
        m_progressStepMinimum = m_progressStepMaximum;
        m_progressStepMaximum = 1100;
    } else if (inReplyTo == COMPUTE_TYPE) {
        m_cmakeServer->sendRequest(CODEMODEL_TYPE);
        if (m_future)
            m_future->setProgressValue(1100);
        m_progressStepMinimum = m_progressStepMaximum;
        m_progressStepMaximum = 1200;
    } else if (inReplyTo == CODEMODEL_TYPE) {
        extractCodeModelData(data);
        m_cmakeServer->sendRequest(CMAKE_INPUTS_TYPE);
        if (m_future)
            m_future->setProgressValue(1200);
        m_progressStepMinimum = m_progressStepMaximum;
        m_progressStepMaximum = 1300;
    } else if (inReplyTo == CMAKE_INPUTS_TYPE) {
        extractCMakeInputsData(data);
        m_cmakeServer->sendRequest(CACHE_TYPE);
        if (m_future)
            m_future->setProgressValue(1300);
        m_progressStepMinimum = m_progressStepMaximum;
        m_progressStepMaximum = 1400;
    } else if (inReplyTo == CACHE_TYPE) {
        extractCacheData(data);
        if (m_future) {
            m_future->setProgressValue(MAX_PROGRESS);
            m_future->reportFinished();
            m_future.reset();
        }
        Core::MessageManager::write(tr("CMake Project was parsed successfully."));
        emit dataAvailable();
    }
}

void ServerModeReader::handleError(const QString &message)
{
    TaskHub::addTask(Task::Error, message, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                     Utils::FileName(), -1);
    stop();
    Core::MessageManager::write(tr("CMake Project parsing failed."));
    emit errorOccured(message);
}

void ServerModeReader::handleProgress(int min, int cur, int max, const QString &inReplyTo)
{
    Q_UNUSED(inReplyTo);

    if (!m_future)
        return;
    const int progress = calculateProgress(m_progressStepMinimum, min, cur, max, m_progressStepMaximum);
    m_future->setProgressValue(progress);
}

void ServerModeReader::handleSignal(const QString &signal, const QVariantMap &data)
{
    Q_UNUSED(data);
    // CMake on Windows sends false dirty signals on each edit (QTCREATORBUG-17944)
    if (!HostOsInfo::isWindowsHost() && signal == "dirty")
        emit dirty();
}

int ServerModeReader::calculateProgress(const int minRange, const int min, const int cur, const int max, const int maxRange)
{
    if (minRange == maxRange || min == max)
        return minRange;
    const int clampedCur = std::min(std::max(cur, min), max);
    return minRange + ((clampedCur - min) / (max - min)) * (maxRange - minRange);
}

void ServerModeReader::extractCodeModelData(const QVariantMap &data)
{
    const QVariantList configs = data.value("configurations").toList();
    QTC_CHECK(configs.count() == 1); // FIXME: Support several configurations!
    for (const QVariant &c : configs) {
        const QVariantMap &cData = c.toMap();
        extractConfigurationData(cData);
    }
}

void ServerModeReader::extractConfigurationData(const QVariantMap &data)
{
    const QString name = data.value(NAME_KEY).toString();
    Q_UNUSED(name);
    QSet<QString> knownTargets; // To filter duplicate target names:-/
    const QVariantList projects = data.value("projects").toList();
    for (const QVariant &p : projects) {
        const QVariantMap pData = p.toMap();
        m_projects.append(extractProjectData(pData, knownTargets));
    }
}

ServerModeReader::Project *ServerModeReader::extractProjectData(const QVariantMap &data,
                                                                QSet<QString> &knownTargets)
{
    auto project = new Project;
    project->name = data.value(NAME_KEY).toString();
    project->sourceDirectory = FileName::fromString(data.value(SOURCE_DIRECTORY_KEY).toString());

    const QVariantList targets = data.value("targets").toList();
    for (const QVariant &t : targets) {
        const QVariantMap tData = t.toMap();
        Target *tp = extractTargetData(tData, project, knownTargets);
        if (tp)
            project->targets.append(tp);
    }
    return project;
}

ServerModeReader::Target *ServerModeReader::extractTargetData(const QVariantMap &data, Project *p,
                                                              QSet<QString> &knownTargets)
{
    const QString targetName = data.value(NAME_KEY).toString();

    // Remove duplicate targets: CMake unfortunately does duplicate targets for all projects that
    // contain them. Keep at least till cmake 3.9 is deprecated.
    const int count = knownTargets.count();
    knownTargets.insert(targetName);
    if (knownTargets.count() == count)
        return nullptr;

    auto target = new Target;
    target->project = p;
    target->name = targetName;
    target->sourceDirectory = FileName::fromString(data.value(SOURCE_DIRECTORY_KEY).toString());
    target->buildDirectory = FileName::fromString(data.value("buildDirectory").toString());

    target->crossReferences = extractCrossReferences(data.value("crossReferences").toMap());

    QDir srcDir(target->sourceDirectory.toString());

    target->type = data.value("type").toString();
    const QStringList artifacts = data.value("artifacts").toStringList();
    target->artifacts = transform(artifacts, [&srcDir](const QString &a) { return FileName::fromString(srcDir.absoluteFilePath(a)); });

    const QVariantList fileGroups = data.value("fileGroups").toList();
    for (const QVariant &fg : fileGroups) {
        const QVariantMap fgData = fg.toMap();
        target->fileGroups.append(extractFileGroupData(fgData, srcDir, target));
    }

    fixTarget(target);

    m_targets.append(target);
    return target;
}

ServerModeReader::FileGroup *ServerModeReader::extractFileGroupData(const QVariantMap &data,
                                                                    const QDir &srcDir,
                                                                    Target *t)
{
    auto fileGroup = new FileGroup;
    fileGroup->target = t;
    fileGroup->compileFlags = data.value("compileFlags").toString();
    fileGroup->macros = Utils::transform<QVector>(data.value("defines").toStringList(), [](const QString &s) {
        return ProjectExplorer::Macro::fromKeyValue(s);
    });
    fileGroup->includePaths = transform(data.value("includePath").toList(),
                                        [](const QVariant &i) -> IncludePath* {
        const QVariantMap iData = i.toMap();
        auto result = new IncludePath;
        result->path = FileName::fromString(iData.value("path").toString());
        result->isSystem = iData.value("isSystem", false).toBool();
        return result;
    });
    fileGroup->isGenerated = data.value("isGenerated", false).toBool();
    fileGroup->sources = transform(data.value(SOURCES_KEY).toStringList(),
                                   [&srcDir](const QString &s) {
        return FileName::fromString(QDir::cleanPath(srcDir.absoluteFilePath(s)));
    });

    m_fileGroups.append(fileGroup);
    return fileGroup;
}

QList<ServerModeReader::CrossReference *> ServerModeReader::extractCrossReferences(const QVariantMap &data)
{
    QList<CrossReference *> crossReferences;

    if (data.isEmpty())
        return crossReferences;

    auto cr = std::make_unique<CrossReference>();
    cr->backtrace = extractBacktrace(data.value(BACKTRACE_KEY, QVariantList()).toList());
    QTC_ASSERT(!cr->backtrace.isEmpty(), return {});
    crossReferences.append(cr.release());

    const QVariantList related = data.value("relatedStatements", QVariantList()).toList();
    for (const QVariant &relatedData : related) {
        auto cr = std::make_unique<CrossReference>();

        // extract information:
        const QVariantMap map = relatedData.toMap();
        const QString typeString = map.value("type", QString()).toString();
        if (typeString.isEmpty())
            cr->type = CrossReference::TARGET;
        else if (typeString == "target_link_libraries")
            cr->type = CrossReference::LIBRARIES;
        else if (typeString == "target_compile_defines")
            cr->type = CrossReference::DEFINES;
        else if (typeString == "target_include_directories")
            cr->type = CrossReference::INCLUDES;
        else
            cr->type = CrossReference::UNKNOWN;
        cr->backtrace = extractBacktrace(map.value(BACKTRACE_KEY, QVariantList()).toList());

        // sanity check:
        if (cr->backtrace.isEmpty())
            continue;

        // store information:
        crossReferences.append(cr.release());
    }
    return crossReferences;
}

ServerModeReader::BacktraceItem *ServerModeReader::extractBacktraceItem(const QVariantMap &data)
{
    QTC_ASSERT(!data.isEmpty(), return nullptr);
    auto item = std::make_unique<BacktraceItem>();

    item->line = data.value(LINE_KEY, -1).toInt();
    item->name = data.value(NAME_KEY, QString()).toString();
    item->path = data.value(PATH_KEY, QString()).toString();

    QTC_ASSERT(!item->path.isEmpty(), return nullptr);
    return item.release();
}

QList<ServerModeReader::BacktraceItem *> ServerModeReader::extractBacktrace(const QVariantList &data)
{
    QList<BacktraceItem *> btResult;
    for (const QVariant &bt : data) {
        BacktraceItem *btItem = extractBacktraceItem(bt.toMap());
        QTC_ASSERT(btItem, continue);

        btResult.append(btItem);
    }
    return btResult;
}

void ServerModeReader::extractCMakeInputsData(const QVariantMap &data)
{
    const FileName src = FileName::fromString(data.value(SOURCE_DIRECTORY_KEY).toString());
    QTC_ASSERT(src == m_parameters.sourceDirectory, return);
    QDir srcDir(src.toString());

    m_cmakeFiles.clear();

    const QVariantList buildFiles = data.value("buildFiles").toList();
    for (const QVariant &bf : buildFiles) {
        const QVariantMap &section = bf.toMap();
        const QStringList sources = section.value(SOURCES_KEY).toStringList();

        const bool isTemporary = section.value("isTemporary").toBool();
        const bool isCMake = section.value("isCMake").toBool();

        for (const QString &s : sources) {
            const FileName sfn = FileName::fromString(QDir::cleanPath(srcDir.absoluteFilePath(s)));
            const int oldCount = m_cmakeFiles.count();
            m_cmakeFiles.insert(sfn);
            if (oldCount < m_cmakeFiles.count() && (!isCMake || sfn.toString().endsWith("/CMakeLists.txt"))) {
                // Always include CMakeLists.txt files, even when cmake things these are part of its
                // stuff. This unbreaks cmake binaries running from their own build directory.
                m_cmakeInputsFileNodes.append(new FileNode(sfn, FileType::Project, isTemporary));
            }
        }
    }
}

void ServerModeReader::extractCacheData(const QVariantMap &data)
{
    CMakeConfig config;
    const QVariantList entries = data.value("cache").toList();
    for (const QVariant &e : entries) {
        const QVariantMap eData = e.toMap();
        CMakeConfigItem item;
        item.key = eData.value("key").toByteArray();
        item.value = eData.value("value").toByteArray();
        item.type = CMakeConfigItem::typeStringToType(eData.value("type").toByteArray());
        const QVariantMap properties = eData.value("properties").toMap();
        item.isAdvanced = properties.value("ADVANCED", false).toBool();
        item.documentation = properties.value("HELPSTRING").toByteArray();
        item.values = CMakeConfigItem::cmakeSplitValue(properties.value("STRINGS").toString(), true);
        config.append(item);
    }
    m_cmakeConfiguration = config;
}

void ServerModeReader::fixTarget(ServerModeReader::Target *target) const
{
    QHash<QString, const FileGroup *> languageFallbacks;

    for (const FileGroup *group : Utils::asConst(target->fileGroups)) {
        if (group->includePaths.isEmpty() && group->compileFlags.isEmpty()
                && group->macros.isEmpty())
            continue;

        const FileGroup *fallback = languageFallbacks.value(group->language);
        if (!fallback || fallback->sources.count() < group->sources.count())
            languageFallbacks.insert(group->language, group);
    }

    if (!languageFallbacks.value(""))
        return; // No empty language groups found, no need to proceed.

    const FileGroup *fallback = languageFallbacks.value("CXX");
    if (!fallback)
        fallback = languageFallbacks.value("C");
    if (!fallback)
        fallback = languageFallbacks.value("");

    if (!fallback)
        return;

    for (auto it = target->fileGroups.begin(); it != target->fileGroups.end(); ++it) {
        if (!(*it)->language.isEmpty())
            continue;
        (*it)->language = fallback->language.isEmpty() ? "CXX" : fallback->language;

        if (*it == fallback
                || !(*it)->includePaths.isEmpty() || !(*it)->macros.isEmpty()
                || !(*it)->compileFlags.isEmpty())
            continue;

        for (const IncludePath *ip : fallback->includePaths)
            (*it)->includePaths.append(new IncludePath(*ip));
        (*it)->macros = fallback->macros;
        (*it)->compileFlags = fallback->compileFlags;
    }
}

QHash<Utils::FileName, ProjectNode *>
ServerModeReader::addCMakeLists(CMakeProjectNode *root, const QList<FileNode *> &cmakeLists)
{
    QHash<Utils::FileName, ProjectNode *> cmakeListsNodes;
    cmakeListsNodes.insert(root->filePath(), root);

    const QSet<Utils::FileName> cmakeDirs
            = Utils::transform<QSet>(cmakeLists, [](const Node *n) { return n->filePath().parentDir(); });
    root->addNestedNodes(cmakeLists, Utils::FileName(),
                         [&cmakeDirs, &cmakeListsNodes](const Utils::FileName &fp)
                         -> ProjectExplorer::FolderNode * {
        FolderNode *fn = nullptr;
        if (cmakeDirs.contains(fp)) {
            CMakeListsNode *n = new CMakeListsNode(fp);
            cmakeListsNodes.insert(fp, n);
            fn = n;
        } else {
            fn = new FolderNode(fp);
        }
        return fn;
    });
    root->compress();
    return cmakeListsNodes;
}

static ProjectNode *createProjectNode(const QHash<Utils::FileName, ProjectNode *> &cmakeListsNodes,
                                      const Utils::FileName &dir, const QString &displayName)
{
    ProjectNode *cmln = cmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, qDebug() << dir.toUserOutput() ; return nullptr);

    Utils::FileName projectName = dir;
    projectName.appendPath(".project::" + displayName);

    CMakeProjectNode *pn = static_cast<CMakeProjectNode *>(cmln->projectNode(projectName));
    if (!pn) {
        pn = new CMakeProjectNode(projectName);
        cmln->addNode(pn);
    }
    pn->setDisplayName(displayName);
    return pn;
}

void ServerModeReader::addProjects(const QHash<Utils::FileName, ProjectNode *> &cmakeListsNodes,
                                   const QList<Project *> &projects,
                                   QList<FileNode *> &knownHeaderNodes)
{
    for (const Project *p : projects) {
        ProjectNode *pNode = createProjectNode(cmakeListsNodes, p->sourceDirectory, p->name);
        QTC_ASSERT(pNode, qDebug() << p->sourceDirectory.toUserOutput() ; continue);
        addTargets(cmakeListsNodes, p->targets, knownHeaderNodes);
    }
}

static CMakeTargetNode *createTargetNode(const QHash<Utils::FileName, ProjectNode *> &cmakeListsNodes,
                                         const Utils::FileName &dir, const QString &displayName)
{
    ProjectNode *cmln = cmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, return nullptr);

    QByteArray targetId = CMakeTargetNode::generateId(dir, displayName);

    CMakeTargetNode *tn = static_cast<CMakeTargetNode *>(cmln->findNode([&targetId](const Node *n) {
        return n->id() == targetId;
    }));
    if (!tn) {
        tn = new CMakeTargetNode(dir, displayName);
        cmln->addNode(tn);
    }
    tn->setDisplayName(displayName);
    return tn;
}

void ServerModeReader::addTargets(const QHash<Utils::FileName, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
                                  const QList<Target *> &targets,
                                  QList<ProjectExplorer::FileNode *> &knownHeaderNodes)
{
    for (const Target *t : targets) {
        CMakeTargetNode *tNode = createTargetNode(cmakeListsNodes, t->sourceDirectory, t->name);
        QTC_ASSERT(tNode, qDebug() << "No target node for" << t->sourceDirectory << t->name; continue);
        tNode->setTargetInformation(t->artifacts, t->type);
        QList<FolderNode::LocationInfo> info;
        // Set up a default target path:
        FileName targetPath = t->sourceDirectory;
        targetPath.appendPath("CMakeLists.txt");
        for (CrossReference *cr : Utils::asConst(t->crossReferences)) {
            BacktraceItem *bt = cr->backtrace.isEmpty() ? nullptr : cr->backtrace.at(0);
            if (bt) {
                const QString btName = bt->name.toLower();
                const FileName path = Utils::FileName::fromUserInput(bt->path);
                QString dn;
                if (cr->type != CrossReference::TARGET) {
                    if (path == targetPath) {
                        if (bt->line >= 0)
                            dn = tr("%1 in line %3").arg(btName).arg(bt->line);
                        else
                            dn = tr("%1").arg(btName);
                    } else {
                        if (bt->line >= 0)
                            dn = tr("%1 in %2:%3").arg(btName, path.toUserOutput()).arg(bt->line);
                        else
                            dn = tr("%1 in %2").arg(btName, path.toUserOutput());
                    }
                } else {
                    dn = tr("Target Definition");
                    targetPath = path;
                }
                info.append(FolderNode::LocationInfo(dn, path, bt->line));
            }
        }
        tNode->setLocationInfo(info);
        addFileGroups(tNode, t->sourceDirectory, t->buildDirectory, t->fileGroups, knownHeaderNodes);
    }
}

void ServerModeReader::addFileGroups(ProjectNode *targetRoot,
                                     const Utils::FileName &sourceDirectory,
                                     const Utils::FileName &buildDirectory,
                                     const QList<ServerModeReader::FileGroup *> &fileGroups,
                                     QList<FileNode *> &knownHeaderNodes)
{
    QList<FileNode *> toList;
    QSet<Utils::FileName> alreadyListed;
    for (const FileGroup *f : fileGroups) {
        const QList<FileName> newSources = Utils::filtered(f->sources, [&alreadyListed](const Utils::FileName &fn) {
            const int count = alreadyListed.count();
            alreadyListed.insert(fn);
            return count != alreadyListed.count();
        });
        const QList<FileNode *> newFileNodes
                = Utils::transform(newSources, [f, &knownHeaderNodes](const Utils::FileName &fn) {
            auto node = new FileNode(fn, Node::fileTypeForFileName(fn), f->isGenerated);
            if (node->fileType() == FileType::Header)
                knownHeaderNodes.append(node);
            return node;
        });
        toList.append(newFileNodes);
    }

    // Split up files in groups (based on location):
    const bool inSourceBuild = (m_parameters.buildDirectory == m_parameters.sourceDirectory);
    QList<FileNode *> sourceFileNodes;
    QList<FileNode *> buildFileNodes;
    QList<FileNode *> otherFileNodes;
    foreach (FileNode *fn, toList) {
        if (fn->filePath().isChildOf(m_parameters.buildDirectory) && !inSourceBuild)
            buildFileNodes.append(fn);
        else if (fn->filePath().isChildOf(m_parameters.sourceDirectory))
            sourceFileNodes.append(fn);
        else
            otherFileNodes.append(fn);
    }

    addCMakeVFolder(targetRoot, sourceDirectory, 1000,  QString(), sourceFileNodes);
    addCMakeVFolder(targetRoot, buildDirectory, 100, tr("<Build Directory>"), buildFileNodes);
    addCMakeVFolder(targetRoot, Utils::FileName(), 10, tr("<Other Locations>"), otherFileNodes);
}

void ServerModeReader::addHeaderNodes(ProjectNode *root, const QList<FileNode *> knownHeaders,
                                      const QList<const FileNode *> &allFiles)
{
    if (root->isEmpty())
        return;

    static QIcon headerNodeIcon = Core::FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_H);
    auto headerNode = new VirtualFolderNode(root->filePath(), Node::DefaultPriority - 5);
    headerNode->setDisplayName(tr("<Headers>"));
    headerNode->setIcon(headerNodeIcon);

    // knownHeaders are already listed in their targets:
    QSet<Utils::FileName> seenHeaders = Utils::transform<QSet>(knownHeaders, &FileNode::filePath);

    // Add scanned headers:
    for (const FileNode *fn : allFiles) {
        if (fn->fileType() != FileType::Header || !fn->filePath().isChildOf(root->filePath()))
            continue;
        const int count = seenHeaders.count();
        seenHeaders.insert(fn->filePath());
        if (seenHeaders.count() != count) {
            auto node = fn->clone();
            node->setEnabled(false);
            headerNode->addNestedNode(node);
        }
    }

    if (headerNode->nodes().isEmpty())
        delete headerNode; // No Headers, do not show this Folder.
    else
        root->addNode(headerNode);
}

} // namespace Internal
} // namespace CMakeProjectManager

#if defined(WITH_TESTS)

#include "cmakeprojectplugin.h"
#include <QTest>

namespace CMakeProjectManager {
namespace Internal {

void CMakeProjectPlugin::testServerModeReaderProgress_data()
{
    QTest::addColumn<int>("minRange");
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("cur");
    QTest::addColumn<int>("max");
    QTest::addColumn<int>("maxRange");
    QTest::addColumn<int>("expected");

    QTest::newRow("empty range") << 100 << 10 << 11 << 20 << 100 << 100;
    QTest::newRow("one range (low)") << 0 << 10 << 11 << 20 << 1 << 0;
    QTest::newRow("one range (high)") << 20 << 10 << 19 << 20 << 20 << 20;
    QTest::newRow("large range") << 30 << 10 << 11 << 20 << 100000 << 30;

    QTest::newRow("empty progress") << -5 << 10 << 10 << 10 << 99995 << -5;
    QTest::newRow("one progress (low)") << 42 << 10 << 10 << 11 << 100042 << 42;
    QTest::newRow("one progress (high)") << 0 << 10 << 11 << 11 << 100000 << 100000;
    QTest::newRow("large progress") << 0 << 10 << 10 << 11 << 100000 << 0;

    QTest::newRow("cur too low") << 0 << 10 << 9 << 100 << 100000 << 0;
    QTest::newRow("cur too high") << 0 << 10 << 101 << 100 << 100000 << 100000;
    QTest::newRow("cur much too low") << 0 << 10 << -1000 << 100 << 100000 << 0;
    QTest::newRow("cur much too high") << 0 << 10 << 1110000 << 100 << 100000 << 100000;
}

void CMakeProjectPlugin::testServerModeReaderProgress()
{
    QFETCH(int, minRange);
    QFETCH(int, min);
    QFETCH(int, cur);
    QFETCH(int, max);
    QFETCH(int, maxRange);
    QFETCH(int, expected);

    ServerModeReader reader;
    const int r = reader.calculateProgress(minRange, min, cur, max, maxRange);

    QCOMPARE(r, expected);

    QVERIFY(r <= maxRange);
    QVERIFY(r >= minRange);
}

} // namespace Internal
} // namespace CMakeProjectManager

#endif
