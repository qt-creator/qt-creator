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
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

const char CACHE_TYPE[] = "cache";
const char CODEMODEL_TYPE[] = "codemodel";
const char CONFIGURE_TYPE[] = "configure";
const char CMAKE_INPUTS_TYPE[] = "cmakeInputs";
const char COMPUTE_TYPE[] = "compute";

const char NAME_KEY[] = "name";
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
}

ServerModeReader::~ServerModeReader()
{
    stop();
}

void ServerModeReader::setParameters(const BuildDirReader::Parameters &p)
{
    BuildDirReader::setParameters(p);
    if (!m_cmakeServer) {
        m_cmakeServer.reset(new ServerMode(p.environment,
                                           p.sourceDirectory, p.buildDirectory, p.cmakeExecutable,
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
        connect(m_cmakeServer.get(), &ServerMode::cmakeMessage,
                this, [this](const QString &m) { Core::MessageManager::write(m); });
        connect(m_cmakeServer.get(), &ServerMode::message,
                this, [](const QString &m) { Core::MessageManager::write(m); });
        connect(m_cmakeServer.get(), &ServerMode::connected,
                this, &ServerModeReader::isReadyNow, Qt::QueuedConnection); // Delay
        connect(m_cmakeServer.get(), &ServerMode::disconnected,
                this, [this]() {
            stop();
            m_cmakeServer.reset();
        }, Qt::QueuedConnection); // Delay
    }
}

bool ServerModeReader::isCompatible(const BuildDirReader::Parameters &p)
{
    // Server mode connection got lost, reset...
    if (!m_parameters.cmakeExecutable.isEmpty() && !m_cmakeServer)
        return false;

    return p.cmakeHasServerMode
            && p.cmakeExecutable == m_parameters.cmakeExecutable
            && p.environment == m_parameters.environment
            && p.generator == m_parameters.generator
            && p.extraGenerator == m_parameters.extraGenerator
            && p.platform == m_parameters.platform
            && p.toolset == m_parameters.toolset
            && p.sourceDirectory == m_parameters.sourceDirectory
            && p.buildDirectory == m_parameters.buildDirectory;
}

void ServerModeReader::resetData()
{
    m_hasData = false;
}

void ServerModeReader::parse(bool force)
{
    emit configurationStarted();

    QTC_ASSERT(m_cmakeServer, return);
    QVariantMap extra;
    if (force || !QDir(m_parameters.buildDirectory.toString()).exists("CMakeCache.txt")) {
        QStringList cacheArguments = transform(m_parameters.configuration,
                                               [this](const CMakeConfigItem &i) {
            return i.toArgument(m_parameters.expander);
        });
        cacheArguments.prepend(QString()); // Work around a bug in CMake 3.7.0 and 3.7.1 where
                                           // the first argument gets lost!
        extra.insert("cacheArguments", QVariant(cacheArguments));
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
}

bool ServerModeReader::isReady() const
{
    return m_cmakeServer->isConnected();
}

bool ServerModeReader::isParsing() const
{
    return static_cast<bool>(m_future);
}

bool ServerModeReader::hasData() const
{
    return m_hasData;
}

QList<CMakeBuildTarget> ServerModeReader::buildTargets() const
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
        ct.workingDirectory = t->buildDirectory;
        ct.sourceDirectory = t->sourceDirectory;
        return ct;
    });
    m_targets.clear();
    return result;
}

CMakeConfig ServerModeReader::takeParsedConfiguration()
{
    CMakeConfig config = m_cmakeCache;
    m_cmakeCache.clear();
    return config;
}

static void addCMakeVFolder(FolderNode *base, const Utils::FileName &basePath, int priority,
                     const QString &displayName, QList<FileNode *> &files)
{
    if (files.isEmpty())
        return;
    auto folder = new VirtualFolderNode(basePath, priority);
    folder->setDisplayName(displayName);
    base->addNode(folder);
    folder->addNestedNodes(files);
    for (FolderNode *fn : folder->folderNodes())
        fn->compress();
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

    addCMakeVFolder(cmakeVFolder, sourceDir, 1000,
                    QCoreApplication::translate("CMakeProjectManager::Internal::ServerModeReader", "<Source Directory>"),
                    sourceInputs);
    addCMakeVFolder(cmakeVFolder, buildDir, 100,
                    QCoreApplication::translate("CMakeProjectManager::Internal::ServerModeReader", "<Build Directory>"),
                    buildInputs);
    addCMakeVFolder(cmakeVFolder, Utils::FileName(), 10,
                    QCoreApplication::translate("CMakeProjectManager::Internal::ServerModeReader", "<Other Locations>"),
                    rootInputs);
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
        else if (path.isChildOf(m_parameters.sourceDirectory))
            cmakeFilesSource.append(fn);
        else if (path.isChildOf(m_parameters.buildDirectory))
            cmakeFilesBuild.append(fn);
        else
            cmakeFilesOther.append(fn);
    }
    m_cmakeInputsFileNodes.clear(); // Clean out, they are not going to be used anymore!

    const Project *topLevel = Utils::findOrDefault(m_projects, [this](const Project *p) {
        return m_parameters.sourceDirectory == p->sourceDirectory;
    });
    if (topLevel)
        root->setDisplayName(topLevel->name);

    if (!cmakeFilesSource.isEmpty() || !cmakeFilesBuild.isEmpty() || !cmakeFilesOther.isEmpty())
        addCMakeInputs(root, m_parameters.sourceDirectory, m_parameters.buildDirectory,
                       cmakeFilesSource, cmakeFilesBuild, cmakeFilesOther);

    QHash<Utils::FileName, ProjectNode *> cmakeListsNodes = addCMakeLists(root, cmakeLists);
    QList<FileNode *> knownHeaders;
    addProjects(cmakeListsNodes, m_projects, knownHeaders);

    addHeaderNodes(root, knownHeaders, allFiles);
}

void ServerModeReader::updateCodeModel(CppTools::RawProjectParts &rpps)
{
    int counter = 0;
    foreach (const FileGroup *fg, m_fileGroups) {
        ++counter;
        const QString defineArg
                = transform(fg->defines, [](const QString &s) -> QString {
                    QString result = QString::fromLatin1("#define ") + s;
                    int assignIndex = result.indexOf('=');
                    if (assignIndex != -1)
                        result[assignIndex] = ' ';
                    return result;
                }).join('\n');
        const QStringList flags = QtcProcess::splitArgs(fg->compileFlags);
        const QStringList includes = transform(fg->includePaths, [](const IncludePath *ip)  { return ip->path.toString(); });

        CppTools::RawProjectPart rpp;
        rpp.setProjectFileLocation(fg->target->sourceDirectory.toString() + "/CMakeLists.txt");
        rpp.setBuildSystemTarget(fg->target->name);
        rpp.setDisplayName(fg->target->name + QString::number(counter));
        rpp.setDefines(defineArg.toUtf8());
        rpp.setIncludePaths(includes);

        CppTools::RawProjectPartFlags cProjectFlags;
        cProjectFlags.commandLineFlags = flags;
        rpp.setFlagsForC(cProjectFlags);

        CppTools::RawProjectPartFlags cxxProjectFlags;
        cxxProjectFlags.commandLineFlags = flags;
        rpp.setFlagsForCxx(cxxProjectFlags);

        rpp.setFiles(transform(fg->sources, &FileName::toString));

        rpps.append(rpp);
    }

    qDeleteAll(m_projects); // Not used anymore!
    m_projects.clear();
    m_targets.clear();
    m_fileGroups.clear();
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
        m_hasData = true;
        emit dataAvailable();
    }
}

void ServerModeReader::handleError(const QString &message)
{
    TaskHub::addTask(Task::Error, message, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                     Utils::FileName(), -1);
    stop();
    emit errorOccured(message);
}

void ServerModeReader::handleProgress(int min, int cur, int max, const QString &inReplyTo)
{
    Q_UNUSED(inReplyTo);

    if (!m_future)
        return;
    int progress = m_progressStepMinimum
            + (((max - min) / (cur - min)) * (m_progressStepMaximum  - m_progressStepMinimum));
    m_future->setProgressValue(progress);
}

void ServerModeReader::handleSignal(const QString &signal, const QVariantMap &data)
{
    Q_UNUSED(data);
    if (signal == "dirty")
        emit dirty();
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

    QDir srcDir(target->sourceDirectory.toString());

    target->type = data.value("type").toString();
    const QStringList artifacts = data.value("artifacts").toStringList();
    target->artifacts = transform(artifacts, [&srcDir](const QString &a) { return FileName::fromString(srcDir.absoluteFilePath(a)); });

    const QVariantList fileGroups = data.value("fileGroups").toList();
    for (const QVariant &fg : fileGroups) {
        const QVariantMap fgData = fg.toMap();
        target->fileGroups.append(extractFileGroupData(fgData, srcDir, target));
    }

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
    fileGroup->defines = data.value("defines").toStringList();
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
    m_cmakeCache = config;
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

    Utils::FileName targetName = dir;
    targetName.appendPath(".target::" + displayName);

    CMakeTargetNode *tn = static_cast<CMakeTargetNode *>(cmln->projectNode(targetName));
    if (!tn) {
        tn = new CMakeTargetNode(targetName);
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
    QList<FileNode *> sourceFileNodes;
    QList<FileNode *> buildFileNodes;
    QList<FileNode *> otherFileNodes;
    foreach (FileNode *fn, toList) {
        if (fn->filePath().isChildOf(m_parameters.sourceDirectory))
            sourceFileNodes.append(fn);
        else if (fn->filePath().isChildOf(m_parameters.buildDirectory))
            buildFileNodes.append(fn);
        else
            otherFileNodes.append(fn);
    }

    addCMakeVFolder(targetRoot, sourceDirectory, 1000, tr("<Source Directory>"), sourceFileNodes);
    addCMakeVFolder(targetRoot, buildDirectory, 100, tr("<Build Directory>"), buildFileNodes);
    addCMakeVFolder(targetRoot, Utils::FileName(), 10, tr("<Other Locations>"), otherFileNodes);
}

void ServerModeReader::addHeaderNodes(ProjectNode *root, const QList<FileNode *> knownHeaders,
                                      const QList<const FileNode *> &allFiles)
{
    auto headerNode = new VirtualFolderNode(root->filePath(), Node::DefaultPriority - 5);
    headerNode->setDisplayName(tr("<Headers>"));

    // knownHeaders are already listed in their targets:
    QSet<Utils::FileName> seenHeaders = Utils::transform<QSet>(knownHeaders, &FileNode::filePath);

    // Add scanned headers:
    for (const FileNode *fn : allFiles) {
        if (fn->fileType() != FileType::Header || !fn->filePath().isChildOf(root->filePath()))
            continue;
        const int count = seenHeaders.count();
        seenHeaders.insert(fn->filePath());
        if (seenHeaders.count() != count) {
            auto node = new FileNode(*fn);
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
