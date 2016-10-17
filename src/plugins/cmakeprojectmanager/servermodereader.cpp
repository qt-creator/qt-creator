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
#include <cpptools/projectpartbuilder.h>
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

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

QString socketName(const BuildDirReader::Parameters &p)
{
    return p.buildDirectory.toString() + "/socket";
}

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
        connect(m_cmakeServer.get(), &ServerMode::cmakeMessage,
                this, [this](const QString &m) { Core::MessageManager::write(m); });
        connect(m_cmakeServer.get(), &ServerMode::message,
                this, [](const QString &m) { Core::MessageManager::write(m); });
        connect(m_cmakeServer.get(), &ServerMode::connected,
                this, &ServerModeReader::isReadyNow);
        connect(m_cmakeServer.get(), &ServerMode::disconnected,
                this, [this]() { m_cmakeServer.reset(); });
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
    QTC_ASSERT(m_cmakeServer, return);
    QVariantMap extra;
    if (force)
        extra.insert("cacheArguments", QVariant(transform(m_parameters.configuration,
                                                 [this](const CMakeConfigItem &i) {
            return i.toArgument(m_parameters.expander);
        })));

    m_future = new QFutureInterface<void>();
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
        delete m_future;
        m_future = nullptr;
    }
}

bool ServerModeReader::isReady() const
{
    return m_cmakeServer->isConnected();
}

bool ServerModeReader::isParsing() const
{
    return false;
}

bool ServerModeReader::hasData() const
{
    return m_hasData;
}

QList<CMakeBuildTarget> ServerModeReader::buildTargets() const
{
    return transform(m_targets, [](const Target *t) -> CMakeBuildTarget {
        CMakeBuildTarget ct;
        ct.title = t->name;
        ct.executable = t->artifacts.isEmpty() ? FileName() : t->artifacts.at(0);
        TargetType type = UtilityType;
        if (t->type == "STATIC_LIBRARY")
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
}

CMakeConfig ServerModeReader::parsedConfiguration() const
{
    return m_cmakeCache;
}

void ServerModeReader::generateProjectTree(CMakeProjectNode *root)
{
    QSet<Utils::FileName> knownFiles;
    for (auto it = m_cmakeInputsFileNodes.constBegin(); it != m_cmakeInputsFileNodes.constEnd(); ++it)
        knownFiles.insert((*it)->filePath());

    QList<FileNode *> fileGroupNodes = m_cmakeInputsFileNodes;
    for (const FileGroup *fg : qAsConst(m_fileGroups)) {
        for (const FileName &s : fg->sources) {
            const int oldCount = knownFiles.count();
            knownFiles.insert(s);
            if (oldCount != knownFiles.count())
                fileGroupNodes.append(new FileNode(s, SourceType, fg->isGenerated));
        }
    }
    root->buildTree(fileGroupNodes);
}

QSet<Core::Id> ServerModeReader::updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder)
{
    QSet<Core::Id> languages;
    int counter = 0;
    for (const FileGroup *fg : qAsConst(m_fileGroups)) {
        ++counter;
        const QString defineArg
                = transform(fg->defines, [](const QString &s) -> QString { return QString::fromLatin1("#define ") + s; }).join('\n');
        const QStringList flags = QtcProcess::splitArgs(fg->compileFlags);
        const QStringList includes = transform(fg->includePaths, [](const IncludePath *ip)  { return ip->path.toString(); });

        ppBuilder.setProjectFile(fg->target->sourceDirectory.toString());
        ppBuilder.setDisplayName(fg->target->name + QString::number(counter));
        ppBuilder.setDefines(defineArg.toUtf8());
        ppBuilder.setIncludePaths(includes);
        ppBuilder.setCFlags(flags);
        ppBuilder.setCxxFlags(flags);


        languages.unite(QSet<Core::Id>::fromList(ppBuilder.createProjectPartsForFiles(transform(fg->sources, &FileName::toString))));
    }
    return languages;
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
            delete m_future;
            m_future = nullptr;
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
    const QVariantList projects = data.value("projects").toList();
    for (const QVariant &p : projects) {
        const QVariantMap pData = p.toMap();
        m_projects.append(extractProjectData(pData));
    }
}

ServerModeReader::Project *ServerModeReader::extractProjectData(const QVariantMap &data)
{
    auto project = new Project;
    project->name = data.value(NAME_KEY).toString();
    project->sourceDirectory = FileName::fromString(data.value(SOURCE_DIRECTORY_KEY).toString());

    const QVariantList targets = data.value("targets").toList();
    for (const QVariant &t : targets) {
        const QVariantMap tData = t.toMap();
        project->targets.append(extractTargetData(tData, project));
    }
    return project;
}

ServerModeReader::Target *ServerModeReader::extractTargetData(const QVariantMap &data, Project *p)
{
    auto target = new Target;
    target->project = p;
    target->name = data.value(NAME_KEY).toString();
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
        return FileName::fromString(srcDir.absoluteFilePath(s));
    });

    m_fileGroups.append(fileGroup);
    return fileGroup;
}

void ServerModeReader::extractCMakeInputsData(const QVariantMap &data)
{
    const FileName src = FileName::fromString(data.value(SOURCE_DIRECTORY_KEY).toString());
    QTC_ASSERT(src == m_parameters.sourceDirectory, return);
    QDir srcDir(src.toString());

    const QVariantList buildFiles = data.value("buildFiles").toList();
    for (const QVariant &bf : buildFiles) {
        const QVariantMap &section = bf.toMap();
        const QStringList sources = section.value(SOURCES_KEY).toStringList();

        const bool isTemporary = section.value("isTemporary").toBool();
        const bool isCMake = section.value("isCMake").toBool();

        for (const QString &s : sources) {
            const FileName sfn = FileName::fromString(srcDir.absoluteFilePath(s));
            const int oldCount = m_cmakeFiles.count();
            m_cmakeFiles.insert(sfn);
            if (!isCMake && oldCount < m_cmakeFiles.count())
                m_cmakeInputsFileNodes.append(new FileNode(sfn, ProjectFileType, isTemporary));
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

} // namespace Internal
} // namespace CMakeProjectManager
