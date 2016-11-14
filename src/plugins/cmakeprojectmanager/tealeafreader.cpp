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

#include "tealeafreader.h"

#include "cmakebuildconfiguration.h"
#include "cmakecbpparser.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/reaper.h>
#include <cpptools/projectpartbuilder.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDateTime>
#include <QFileInfo>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

namespace CMakeProjectManager {
namespace Internal {

class CMakeFile : public IDocument
{
public:
    CMakeFile(TeaLeafReader *r, const FileName &fileName);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    TeaLeafReader *m_reader;
};

CMakeFile::CMakeFile(TeaLeafReader *r, const FileName &fileName) : m_reader(r)
{
    setId("Cmake.ProjectFile");
    setMimeType(Constants::CMAKEPROJECTMIMETYPE);
    setFilePath(fileName);
}

IDocument::ReloadBehavior CMakeFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool CMakeFile::reload(QString *errorString, IDocument::ReloadFlag flag, IDocument::ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);

    if (type != TypePermissions)
        emit m_reader->dirty();
    return true;
}

static QString lineSplit(const QString &rest, const QByteArray &array, std::function<void(const QString &)> f)
{
    QString tmp = rest + SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(array));
    int start = 0;
    int end = tmp.indexOf(QLatin1Char('\n'), start);
    while (end >= 0) {
        f(tmp.mid(start, end - start));
        start = end + 1;
        end = tmp.indexOf(QLatin1Char('\n'), start);
    }
    return tmp.mid(start);
}

static QStringList toArguments(const CMakeConfig &config, const MacroExpander *expander) {
    return transform(config, [expander](const CMakeConfigItem &i) -> QString {
        return i.toArgument(expander);
    });
}

static QByteArray trimCMakeCacheLine(const QByteArray &in) {
    int start = 0;
    while (start < in.count() && (in.at(start) == ' ' || in.at(start) == '\t'))
        ++start;

    return in.mid(start, in.count() - start - 1);
}

static QByteArrayList splitCMakeCacheLine(const QByteArray &line) {
    const int colonPos = line.indexOf(':');
    if (colonPos < 0)
        return QByteArrayList();

    const int equalPos = line.indexOf('=', colonPos + 1);
    if (equalPos < colonPos)
        return QByteArrayList();

    return QByteArrayList() << line.mid(0, colonPos)
                            << line.mid(colonPos + 1, equalPos - colonPos - 1)
                            << line.mid(equalPos + 1);
}

// --------------------------------------------------------------------
// TeaLeafReader:
// --------------------------------------------------------------------

TeaLeafReader::TeaLeafReader()
{
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, [this](const IDocument *document) {
        if (m_cmakeFiles.contains(document->filePath()) || !m_parameters.isAutorun)
            emit dirty();
    });
}

TeaLeafReader::~TeaLeafReader()
{
    stop();
    resetData();
}

bool TeaLeafReader::isCompatible(const BuildDirReader::Parameters &p)
{
    return !p.cmakeHasServerMode;
}

void TeaLeafReader::resetData()
{
    m_hasData = false;

    qDeleteAll(m_watchedFiles);
    m_watchedFiles.clear();

    m_cmakeCache.clear();
    m_projectName.clear();
    m_buildTargets.clear();
    qDeleteAll(m_files);
    m_files.clear();
}

void TeaLeafReader::parse(bool force)
{
    const QString cbpFile = CMakeManager::findCbpFile(QDir(m_parameters.buildDirectory.toString()));
    const QFileInfo cbpFileFi = cbpFile.isEmpty() ? QFileInfo() : QFileInfo(cbpFile);
    if (!cbpFileFi.exists()) {
        // Initial create:
        startCMake(toArguments(m_parameters.configuration, m_parameters.expander));
        return;
    }

    const bool mustUpdate = force
            || m_cmakeFiles.isEmpty()
            || anyOf(m_cmakeFiles, [&cbpFileFi](const FileName &f) {
                   return f.toFileInfo().lastModified() > cbpFileFi.lastModified();
               });
    if (mustUpdate) {
        startCMake(QStringList());
    } else {
        extractData();
        m_hasData = true;
        emit dataAvailable();
    }
}

void TeaLeafReader::stop()
{
    cleanUpProcess();

    if (m_future) {
        m_future->reportCanceled();
        m_future->reportFinished();
        delete m_future;
        m_future = nullptr;
    }
}

bool TeaLeafReader::isParsing() const
{
    return m_cmakeProcess && m_cmakeProcess->state() != QProcess::NotRunning;
}

bool TeaLeafReader::hasData() const
{
    return m_hasData;
}

QList<CMakeBuildTarget> TeaLeafReader::buildTargets() const
{
    return m_buildTargets;
}

CMakeConfig TeaLeafReader::parsedConfiguration() const
{
    CMakeConfig result;
    FileName cacheFile = m_parameters.buildDirectory;
    cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
    if (!cacheFile.exists())
        return result;
    QString errorMessage;
    m_cmakeCache = parseConfiguration(cacheFile, &errorMessage);
    if (!errorMessage.isEmpty())
        emit errorOccured(errorMessage);
    const FileName sourceOfBuildDir
            = FileName::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", m_cmakeCache));
    const FileName canonicalSourceOfBuildDir = FileUtils::canonicalPath(sourceOfBuildDir);
    const FileName canonicalSourceDirectory = FileUtils::canonicalPath(m_parameters.sourceDirectory);
    if (canonicalSourceOfBuildDir != canonicalSourceDirectory) { // Uses case-insensitive compare where appropriate
        emit errorOccured(tr("The build directory is not for %1 but for %2")
                          .arg(canonicalSourceOfBuildDir.toUserOutput(),
                               canonicalSourceDirectory.toUserOutput()));
    }
    return result;
}

CMakeConfig TeaLeafReader::parseConfiguration(const FileName &cacheFile, QString *errorMessage) const
{
    CMakeConfig result;
    QFile cache(cacheFile.toString());
    if (!cache.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = tr("Failed to open %1 for reading.").arg(cacheFile.toUserOutput());
        return CMakeConfig();
    }

    QSet<QByteArray> advancedSet;
    QMap<QByteArray, QByteArray> valuesMap;
    QByteArray documentation;
    while (!cache.atEnd()) {
        const QByteArray line = trimCMakeCacheLine(cache.readLine());

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith("//")) {
            documentation = line.mid(2);
            continue;
        }

        const QByteArrayList pieces = splitCMakeCacheLine(line);
        if (pieces.isEmpty())
            continue;

        QTC_ASSERT(pieces.count() == 3, continue);
        const QByteArray key = pieces.at(0);
        const QByteArray type = pieces.at(1);
        const QByteArray value = pieces.at(2);

        if (key.endsWith("-ADVANCED") && value == "1") {
            advancedSet.insert(key.left(key.count() - 9 /* "-ADVANCED" */));
        } else if (key.endsWith("-STRINGS") && CMakeConfigItem::typeStringToType(type) == CMakeConfigItem::INTERNAL) {
            valuesMap[key.left(key.count() - 8) /* "-STRINGS" */] = value;
        } else {
            CMakeConfigItem::Type t = CMakeConfigItem::typeStringToType(type);
            result << CMakeConfigItem(key, t, documentation, value);
        }
    }

    // Set advanced flags:
    for (int i = 0; i < result.count(); ++i) {
        CMakeConfigItem &item = result[i];
        item.isAdvanced = advancedSet.contains(item.key);

        if (valuesMap.contains(item.key)) {
            item.values = CMakeConfigItem::cmakeSplitValue(QString::fromUtf8(valuesMap[item.key]));
        } else if (item.key  == "CMAKE_BUILD_TYPE") {
            // WA for known options
            item.values << "" << "Debug" << "Release" << "MinSizeRel" << "RelWithDebInfo";
        }
    }

    sort(result, CMakeConfigItem::sortOperator());

    return result;
}

void TeaLeafReader::generateProjectTree(CMakeListsNode *root, const QList<FileNode *> &allFiles)
{
    root->setDisplayName(m_projectName);

    // Delete no longer necessary file watcher based on m_cmakeFiles:
    const QSet<FileName> currentWatched
            = transform(m_watchedFiles, [](CMakeFile *cmf) { return cmf->filePath(); });
    const QSet<FileName> toWatch = m_cmakeFiles;
    QSet<FileName> toDelete = currentWatched;
    toDelete.subtract(toWatch);
    m_watchedFiles = filtered(m_watchedFiles, [&toDelete](Internal::CMakeFile *cmf) {
            if (toDelete.contains(cmf->filePath())) {
                delete cmf;
                return false;
            }
            return true;
        });

    // Add new file watchers:
    QSet<FileName> toAdd = toWatch;
    toAdd.subtract(currentWatched);
    foreach (const FileName &fn, toAdd) {
        CMakeFile *cm = new CMakeFile(this, fn);
        DocumentManager::addDocument(cm);
        m_watchedFiles.insert(cm);
    }

    QList<FileNode *> added;
    QList<FileNode *> deleted;

    ProjectExplorer::compareSortedLists(m_files, allFiles, deleted, added, Node::sortByPath);

    QSet<FileName> allIncludePathSet;
    for (const CMakeBuildTarget &bt : m_buildTargets) {
        const QList<Utils::FileName> targetIncludePaths
                = Utils::filtered(bt.includeFiles, [this](const Utils::FileName &fn) {
            return fn.isChildOf(m_parameters.sourceDirectory);
        });
        allIncludePathSet.unite(QSet<FileName>::fromList(targetIncludePaths));
    }
    const QList<FileName> allIncludePaths = allIncludePathSet.toList();

    QList<FileNode *> includedHeaderFiles;
    QList<FileNode *> unusedFileNodes;
    std::tie(includedHeaderFiles, unusedFileNodes)
            = Utils::partition(allFiles, [&allIncludePaths](const FileNode *fn) -> bool {
        if (fn->fileType() != FileType::Header)
            return false;

        for (const FileName &inc : allIncludePaths) {
            if (fn->filePath().isChildOf(inc))
                return true;
        }
        return false;
    });

    const auto knownFiles = QSet<FileName>::fromList(Utils::transform(m_files, [](const FileNode *fn) { return fn->filePath(); }));
    QList<FileNode *> uniqueHeaders;
    foreach (FileNode *ifn, includedHeaderFiles) {
        if (!knownFiles.contains(ifn->filePath())) {
            uniqueHeaders.append(ifn);
            ifn->setEnabled(false);
        }
    }

    QList<FileNode *> fileNodes = m_files + uniqueHeaders;

    // Filter out duplicate nodes that e.g. the servermode reader introduces:
    QSet<FileName> uniqueFileNames;
    QSet<Node *> uniqueNodes;
    foreach (FileNode *fn, root->recursiveFileNodes()) {
        const int count = uniqueFileNames.count();
        uniqueFileNames.insert(fn->filePath());
        if (count != uniqueFileNames.count())
            uniqueNodes.insert(static_cast<Node *>(fn));
    }
    root->trim(uniqueNodes);
    root->removeProjectNodes(root->projectNodes()); // Remove all project nodes

    root->buildTree(fileNodes, m_parameters.sourceDirectory);
    m_files.clear(); // Some of the FileNodes in files() were deleted!
}

QSet<Id> TeaLeafReader::updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder)
{
    QSet<Id> languages;
    const ToolChain *tc = ToolChainManager::findToolChain(m_parameters.toolChainId);
    const FileName sysroot = m_parameters.sysRoot;

    QHash<QString, QStringList> targetDataCache;
    foreach (const CMakeBuildTarget &cbt, m_buildTargets) {
        if (cbt.targetType == UtilityType)
            continue;

        // CMake shuffles the include paths that it reports via the CodeBlocks generator
        // So remove the toolchain include paths, so that at least those end up in the correct
        // place.
        const QStringList cxxflags = getCXXFlagsFor(cbt, targetDataCache);
        QSet<FileName> tcIncludes;
        QStringList includePaths;
        if (tc) {
            foreach (const HeaderPath &hp, tc->systemHeaderPaths(cxxflags, sysroot))
                tcIncludes.insert(FileName::fromString(hp.path()));
            foreach (const FileName &i, cbt.includeFiles) {
                if (!tcIncludes.contains(i))
                    includePaths.append(i.toString());
            }
        } else {
            includePaths = transform(cbt.includeFiles, &FileName::toString);
        }
        includePaths += m_parameters.buildDirectory.toString();
        ppBuilder.setIncludePaths(includePaths);
        ppBuilder.setCFlags(cxxflags);
        ppBuilder.setCxxFlags(cxxflags);
        ppBuilder.setDefines(cbt.defines);
        ppBuilder.setDisplayName(cbt.title);

        const QSet<Id> partLanguages
                = QSet<Id>::fromList(ppBuilder.createProjectPartsForFiles(
                                               transform(cbt.files, [](const FileName &fn) { return fn.toString(); })));

        languages.unite(partLanguages);
    }
    return languages;

}

void TeaLeafReader::cleanUpProcess()
{
    if (m_cmakeProcess) {
        m_cmakeProcess->disconnect();
        Reaper::reap(m_cmakeProcess);
        m_cmakeProcess = nullptr;
    }

    // Delete issue parser:
    if (m_parser)
        m_parser->flush();
    delete m_parser;
    m_parser = nullptr;
}

void TeaLeafReader::extractData()
{
    const FileName srcDir = m_parameters.sourceDirectory;
    const FileName bldDir = m_parameters.buildDirectory;
    const FileName topCMake = Utils::FileName(srcDir).appendPath("CMakeLists.txt");

    resetData();

    m_projectName = m_parameters.projectName;
    m_files.append(new FileNode(topCMake, FileType::Project, false));
    // Do not insert topCMake into m_cmakeFiles: The project already watches that!

    // Find cbp file
    FileName cbpFile = FileName::fromString(CMakeManager::findCbpFile(bldDir.toString()));
    if (cbpFile.isEmpty())
        return;
    m_cmakeFiles.insert(cbpFile);

    // Add CMakeCache.txt file:
    FileName cacheFile = m_parameters.buildDirectory;
    cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
    if (cacheFile.toFileInfo().exists())
        m_cmakeFiles.insert(cacheFile);

    // setFolderName
    CMakeCbpParser cbpparser;
    // Parsing
    if (!cbpparser.parseCbpFile(m_parameters.pathMapper, cbpFile, srcDir))
        return;

    m_projectName = cbpparser.projectName();

    m_files = cbpparser.fileList();
    if (cbpparser.hasCMakeFiles()) {
        m_files.append(cbpparser.cmakeFileList());
        foreach (const FileNode *node, cbpparser.cmakeFileList())
            m_cmakeFiles.insert(node->filePath());
    }

    // Make sure the top cmakelists.txt file is always listed:
    if (!contains(m_files, [topCMake](FileNode *fn) { return fn->filePath() == topCMake; }))
        m_files.append(new FileNode(topCMake, FileType::Project, false));

    Utils::sort(m_files, &Node::sortByPath);

    m_buildTargets = cbpparser.buildTargets();
}

void TeaLeafReader::startCMake(const QStringList &configurationArguments)
{
    const FileName buildDirectory = m_parameters.buildDirectory;
    QTC_ASSERT(!m_cmakeProcess, return);
    QTC_ASSERT(!m_parser, return);
    QTC_ASSERT(!m_future, return);
    QTC_ASSERT(buildDirectory.exists(), return);

    const QString srcDir = m_parameters.sourceDirectory.toString();

    m_parser = new CMakeParser;
    QDir source = QDir(srcDir);
    connect(m_parser, &IOutputParser::addTask, m_parser,
            [source](const Task &task) {
                if (task.file.isEmpty() || task.file.toFileInfo().isAbsolute()) {
                    TaskHub::addTask(task);
                } else {
                    Task t = task;
                    t.file = FileName::fromString(source.absoluteFilePath(task.file.toString()));
                    TaskHub::addTask(t);
                }
            });

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.

    m_cmakeProcess = new QtcProcess;
    m_cmakeProcess->setWorkingDirectory(buildDirectory.toString());
    m_cmakeProcess->setEnvironment(m_parameters.environment);

    connect(m_cmakeProcess, &QProcess::readyReadStandardOutput,
            this, &TeaLeafReader::processCMakeOutput);
    connect(m_cmakeProcess, &QProcess::readyReadStandardError,
            this, &TeaLeafReader::processCMakeError);
    connect(m_cmakeProcess, static_cast<void(QProcess::*)(int,  QProcess::ExitStatus)>(&QProcess::finished),
            this, &TeaLeafReader::cmakeFinished);

    QString args;
    QtcProcess::addArg(&args, srcDir);
    QtcProcess::addArgs(&args, m_parameters.generatorArguments);
    QtcProcess::addArgs(&args, configurationArguments);

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    MessageManager::write(tr("Running \"%1 %2\" in %3.")
                          .arg(m_parameters.cmakeExecutable.toUserOutput())
                          .arg(args)
                          .arg(buildDirectory.toUserOutput()));

    m_future = new QFutureInterface<void>();
    m_future->setProgressRange(0, 1);
    ProgressManager::addTask(m_future->future(),
                             tr("Configuring \"%1\"").arg(m_parameters.projectName),
                             "CMake.Configure");

    m_cmakeProcess->setCommand(m_parameters.cmakeExecutable.toString(), args);
    m_cmakeProcess->start();
    emit configurationStarted();
}

void TeaLeafReader::cmakeFinished(int code, QProcess::ExitStatus status)
{
    QTC_ASSERT(m_cmakeProcess, return);

    // process rest of the output:
    processCMakeOutput();
    processCMakeError();

    m_cmakeProcess->disconnect();
    cleanUpProcess();

    extractData(); // try even if cmake failed...

    QString msg;
    if (status != QProcess::NormalExit)
        msg = tr("*** cmake process crashed.");
    else if (code != 0)
        msg = tr("*** cmake process exited with exit code %1.").arg(code);

    if (!msg.isEmpty()) {
        MessageManager::write(msg);
        TaskHub::addTask(Task::Error, msg, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        m_future->reportCanceled();
    } else {
        m_future->setProgressValue(1);
    }

    m_future->reportFinished();
    delete m_future;
    m_future = nullptr;

    m_hasData = true;
    emit dataAvailable();
}

void TeaLeafReader::processCMakeOutput()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardOutput(),
                     [this](const QString &s) { MessageManager::write(s); });
}

void TeaLeafReader::processCMakeError()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardError(), [this](const QString &s) {
        m_parser->stdError(s);
        MessageManager::write(s);
    });
}

QStringList TeaLeafReader::getCXXFlagsFor(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache)
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

bool TeaLeafReader::extractCXXFlagsFromMake(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache)
{
    QString makeCommand = buildTarget.makeCommand.toString();
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

bool TeaLeafReader::extractCXXFlagsFromNinja(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache)
{
    Q_UNUSED(buildTarget)
    if (!cache.isEmpty()) // We fill the cache in one go!
        return false;

    // Attempt to find build.ninja file and obtain FLAGS (CXX_FLAGS) from there if no suitable flags.make were
    // found
    // Get "all" target's working directory
    QByteArray ninjaFile;
    QString buildNinjaFile = buildTargets().at(0).workingDirectory.toString();
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

} // namespace Internal
} // namespace CMakeProjectManager
