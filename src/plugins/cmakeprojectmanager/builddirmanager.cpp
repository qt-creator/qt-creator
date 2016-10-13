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

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmaketool.h"

#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/projectpartbuilder.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>

using namespace ProjectExplorer;

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

namespace CMakeProjectManager {
namespace Internal {

static QStringList toArguments(const CMakeConfig &config, const Kit *k) {
    return Utils::transform(config, [k](const CMakeConfigItem &i) -> QString {
        QString a = QString::fromLatin1("-D");
        a.append(QString::fromUtf8(i.key));
        switch (i.type) {
        case CMakeConfigItem::FILEPATH:
            a.append(QLatin1String(":FILEPATH="));
            break;
        case CMakeConfigItem::PATH:
            a.append(QLatin1String(":PATH="));
            break;
        case CMakeConfigItem::BOOL:
            a.append(QLatin1String(":BOOL="));
            break;
        case CMakeConfigItem::STRING:
            a.append(QLatin1String(":STRING="));
            break;
        case CMakeConfigItem::INTERNAL:
            a.append(QLatin1String(":INTERNAL="));
            break;
        case CMakeConfigItem::STATIC:
            a.append(QLatin1String(":STATIC="));
            break;
        }
        a.append(i.expandedValue(k));

        return a;
    });
}

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc)
{
    QTC_ASSERT(bc, return);
    m_projectName = sourceDirectory().fileName();

    m_reparseTimer.setSingleShot(true);

    connect(&m_reparseTimer, &QTimer::timeout, this, &BuildDirManager::parse);
    connect(Core::EditorManager::instance(), &Core::EditorManager::aboutToSave,
            this, &BuildDirManager::handleDocumentSaves);
}

BuildDirManager::~BuildDirManager()
{
    stopProcess();
    resetData();
    qDeleteAll(m_watchedFiles);
    delete m_tempDir;
}

const Kit *BuildDirManager::kit() const
{
    return m_buildConfiguration->target()->kit();
}

const Utils::FileName BuildDirManager::buildDirectory() const
{
    return m_buildConfiguration->buildDirectory();
}

const Utils::FileName BuildDirManager::workDirectory() const
{
    const Utils::FileName bdir = buildDirectory();
    if (bdir.exists())
        return bdir;
    if (m_tempDir)
        return Utils::FileName::fromString(m_tempDir->path());
    return bdir;
}

const Utils::FileName BuildDirManager::sourceDirectory() const
{
    return m_buildConfiguration->target()->project()->projectDirectory();
}

const CMakeConfig BuildDirManager::intendedConfiguration() const
{
    return m_buildConfiguration->cmakeConfiguration();
}

bool BuildDirManager::isParsing() const
{
    if (m_cmakeProcess)
        return m_cmakeProcess->state() != QProcess::NotRunning;
    return false;
}

void BuildDirManager::cmakeFilesChanged()
{
    if (isParsing())
        return;

    const CMakeTool *tool = CMakeKitInformation::cmakeTool(m_buildConfiguration->target()->kit());
    if (!tool->isAutoRun())
        return;

    m_reparseTimer.start(1000);
}

void BuildDirManager::forceReparse()
{
    if (m_buildConfiguration->target()->activeBuildConfiguration() != m_buildConfiguration)
        return;

    stopProcess();

    CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    QTC_ASSERT(tool, return);

    startCMake(tool, CMakeGeneratorKitInformation::generatorArguments(kit()), intendedConfiguration());
}

void BuildDirManager::resetData()
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

bool BuildDirManager::updateCMakeStateBeforeBuild()
{
    return m_reparseTimer.isActive();
}

bool BuildDirManager::persistCMakeState()
{
    if (!m_tempDir)
        return false;

    QDir dir(buildDirectory().toString());
    dir.mkpath(buildDirectory().toString());

    delete m_tempDir;
    m_tempDir = nullptr;

    resetData();
    QTimer::singleShot(0, this, &BuildDirManager::parse); // make sure signals only happen afterwards!
    return true;
}

void BuildDirManager::generateProjectTree(CMakeProjectNode *root)
{
    root->setDisplayName(m_projectName);

    // Delete no longer necessary file watcher:
    const QSet<Utils::FileName> currentWatched
            = Utils::transform(m_watchedFiles, [](CMakeFile *cmf) { return cmf->filePath(); });
    const QSet<Utils::FileName> toWatch = m_cmakeFiles;
    QSet<Utils::FileName> toDelete = currentWatched;
    toDelete.subtract(toWatch);
    m_watchedFiles = Utils::filtered(m_watchedFiles, [&toDelete](Internal::CMakeFile *cmf) {
            if (toDelete.contains(cmf->filePath())) {
                delete cmf;
                return false;
            }
            return true;
        });

    // Add new file watchers:
    QSet<Utils::FileName> toAdd = toWatch;
    toAdd.subtract(currentWatched);
    foreach (const Utils::FileName &fn, toAdd) {
        CMakeFile *cm = new CMakeFile(this, fn);
        Core::DocumentManager::addDocument(cm);
        m_watchedFiles.insert(cm);
    }

    QList<FileNode *> fileNodes = m_files;
    root->buildTree(fileNodes);
    m_files.clear(); // Some of the FileNodes in files() were deleted!
}

QSet<Core::Id> BuildDirManager::updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder)
{
    QSet<Core::Id> languages;
    ToolChain *tc = ToolChainKitInformation::toolChain(kit(), ToolChain::Language::Cxx);
    const Utils::FileName sysroot = SysRootKitInformation::sysRoot(kit());

    QHash<QString, QStringList> targetDataCache;
    foreach (const CMakeBuildTarget &cbt, buildTargets()) {
        if (cbt.targetType == UtilityType)
            continue;

        // CMake shuffles the include paths that it reports via the CodeBlocks generator
        // So remove the toolchain include paths, so that at least those end up in the correct
        // place.
        QStringList cxxflags = getCXXFlagsFor(cbt, targetDataCache);
        QSet<QString> tcIncludes;
        foreach (const HeaderPath &hp, tc->systemHeaderPaths(cxxflags, sysroot))
            tcIncludes.insert(hp.path());
        QStringList includePaths;
        foreach (const QString &i, cbt.includeFiles) {
            if (!tcIncludes.contains(i))
                includePaths.append(i);
        }
        includePaths += buildDirectory().toString();
        ppBuilder.setIncludePaths(includePaths);
        ppBuilder.setCFlags(cxxflags);
        ppBuilder.setCxxFlags(cxxflags);
        ppBuilder.setDefines(cbt.defines);
        ppBuilder.setDisplayName(cbt.title);

        const QSet<Core::Id> partLanguages
                = QSet<Core::Id>::fromList(ppBuilder.createProjectPartsForFiles(cbt.files));

        languages.unite(partLanguages);
    }
    return languages;
}

void BuildDirManager::parse()
{
    checkConfiguration();

    CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    const QStringList generatorArgs = CMakeGeneratorKitInformation::generatorArguments(kit());

    QTC_ASSERT(tool, return);

    const QString cbpFile = CMakeManager::findCbpFile(QDir(workDirectory().toString()));
    const QFileInfo cbpFileFi = cbpFile.isEmpty() ? QFileInfo() : QFileInfo(cbpFile);
    if (!cbpFileFi.exists()) {
        // Initial create:
        startCMake(tool, generatorArgs, intendedConfiguration());
        return;
    }

    const bool mustUpdate = m_cmakeFiles.isEmpty()
            || Utils::anyOf(m_cmakeFiles, [&cbpFileFi](const Utils::FileName &f) {
                   return f.toFileInfo().lastModified() > cbpFileFi.lastModified();
               });
    if (mustUpdate) {
        startCMake(tool, generatorArgs, CMakeConfig());
    } else {
        extractData();
        m_hasData = true;
        emit dataAvailable();
    }
}

void BuildDirManager::clearCache()
{
    auto cmakeCache = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeCache.txt"));
    auto cmakeFiles = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeFiles"));

    const bool mustCleanUp = cmakeCache.exists() || cmakeFiles.exists();
    if (!mustCleanUp)
        return;

    Utils::FileUtils::removeRecursively(cmakeCache);
    Utils::FileUtils::removeRecursively(cmakeFiles);

    forceReparse();
}

QList<CMakeBuildTarget> BuildDirManager::buildTargets() const
{
    return m_buildTargets;
}

CMakeConfig BuildDirManager::parsedConfiguration() const
{
    if (m_cmakeCache.isEmpty()) {
        Utils::FileName cacheFile = workDirectory();
        cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
        if (!cacheFile.exists())
            return m_cmakeCache;
        QString errorMessage;
        m_cmakeCache = parseConfiguration(cacheFile, &errorMessage);
        if (!errorMessage.isEmpty())
            emit errorOccured(errorMessage);
        const Utils::FileName sourceOfBuildDir
                = Utils::FileName::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", m_cmakeCache));
        const Utils::FileName canonicalSourceOfBuildDir = Utils::FileUtils::canonicalPath(sourceOfBuildDir);
        const Utils::FileName canonicalSourceDirectory = Utils::FileUtils::canonicalPath(sourceDirectory());
        if (canonicalSourceOfBuildDir != canonicalSourceDirectory) // Uses case-insensitive compare where appropriate
            emit errorOccured(tr("The build directory is not for %1 but for %2")
                    .arg(canonicalSourceOfBuildDir.toUserOutput(),
                         canonicalSourceDirectory.toUserOutput()));
    }
    return m_cmakeCache;
}

void BuildDirManager::stopProcess()
{
    if (!m_cmakeProcess)
        return;

    m_cmakeProcess->disconnect();

    if (m_cmakeProcess->state() == QProcess::Running) {
        m_cmakeProcess->terminate();
        if (!m_cmakeProcess->waitForFinished(500) && m_cmakeProcess->state() == QProcess::Running)
            m_cmakeProcess->kill();
    }

    cleanUpProcess();

    if (!m_future)
      return;
    m_future->reportCanceled();
    m_future->reportFinished();
    delete m_future;
    m_future = nullptr;
}

void BuildDirManager::cleanUpProcess()
{
    if (!m_cmakeProcess)
        return;

    QTC_ASSERT(m_cmakeProcess->state() == QProcess::NotRunning, return);

    m_cmakeProcess->disconnect();

    if (m_cmakeProcess->state() == QProcess::Running) {
        m_cmakeProcess->terminate();
        if (!m_cmakeProcess->waitForFinished(500) && m_cmakeProcess->state() == QProcess::Running)
            m_cmakeProcess->kill();
    }
    m_cmakeProcess->waitForFinished();
    delete m_cmakeProcess;
    m_cmakeProcess = nullptr;

    // Delete issue parser:
    m_parser->flush();
    delete m_parser;
    m_parser = nullptr;
}

void BuildDirManager::extractData()
{
    const Utils::FileName topCMake
            = Utils::FileName::fromString(sourceDirectory().toString() + QLatin1String("/CMakeLists.txt"));

    resetData();

    m_projectName = sourceDirectory().fileName();
    m_files.append(new FileNode(topCMake, ProjectFileType, false));
    // Do not insert topCMake into m_cmakeFiles: The project already watches that!

    // Find cbp file
    QString cbpFile = CMakeManager::findCbpFile(workDirectory().toString());
    if (cbpFile.isEmpty())
        return;
    m_cmakeFiles.insert(Utils::FileName::fromString(cbpFile));

    // Add CMakeCache.txt file:
    Utils::FileName cacheFile = workDirectory();
    cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
    if (cacheFile.toFileInfo().exists())
        m_cmakeFiles.insert(cacheFile);

    // setFolderName
    CMakeCbpParser cbpparser;
    CMakeTool *cmake = CMakeKitInformation::cmakeTool(kit());
    // Parsing
    if (!cbpparser.parseCbpFile(cmake->pathMapper(), cbpFile, sourceDirectory().toString()))
        return;

    m_projectName = cbpparser.projectName();

    m_files = cbpparser.fileList();
    if (cbpparser.hasCMakeFiles()) {
        m_files.append(cbpparser.cmakeFileList());
        foreach (const FileNode *node, cbpparser.cmakeFileList())
            m_cmakeFiles.insert(node->filePath());
    }

    // Make sure the top cmakelists.txt file is always listed:
    if (!Utils::contains(m_files, [topCMake](FileNode *fn) { return fn->filePath() == topCMake; })) {
        m_files.append(new FileNode(topCMake, ProjectFileType, false));
    }

    m_buildTargets = cbpparser.buildTargets();
}

void BuildDirManager::startCMake(CMakeTool *tool, const QStringList &generatorArgs,
                                 const CMakeConfig &config)
{
    QTC_ASSERT(tool && tool->isValid(), return);

    QTC_ASSERT(!m_cmakeProcess, return);
    QTC_ASSERT(!m_parser, return);
    QTC_ASSERT(!m_future, return);

    // Find a directory to set up into:
    if (!buildDirectory().exists()) {
        if (!m_tempDir)
            m_tempDir = new QTemporaryDir(QDir::tempPath() + QLatin1String("/qtc-cmake-XXXXXX"));
        QTC_ASSERT(m_tempDir->isValid(), return);
    }

    // Make sure work directory exists:
    QTC_ASSERT(workDirectory().exists(), return);

    m_parser = new CMakeParser;
    QDir source = QDir(sourceDirectory().toString());
    connect(m_parser, &IOutputParser::addTask, m_parser,
            [source](const Task &task) {
                if (task.file.isEmpty() || task.file.toFileInfo().isAbsolute()) {
                    TaskHub::addTask(task);
                } else {
                    Task t = task;
                    t.file = Utils::FileName::fromString(source.absoluteFilePath(task.file.toString()));
                    TaskHub::addTask(t);
                }
            });

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.
    const QString srcDir = sourceDirectory().toString();

    m_cmakeProcess = new Utils::QtcProcess(this);
    m_cmakeProcess->setWorkingDirectory(workDirectory().toString());
    m_cmakeProcess->setEnvironment(m_buildConfiguration->environment());

    connect(m_cmakeProcess, &QProcess::readyReadStandardOutput,
            this, &BuildDirManager::processCMakeOutput);
    connect(m_cmakeProcess, &QProcess::readyReadStandardError,
            this, &BuildDirManager::processCMakeError);
    connect(m_cmakeProcess, static_cast<void(QProcess::*)(int,  QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildDirManager::cmakeFinished);

    QString args;
    Utils::QtcProcess::addArg(&args, srcDir);
    Utils::QtcProcess::addArgs(&args, generatorArgs);
    Utils::QtcProcess::addArgs(&args, toArguments(config, kit()));

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    Core::MessageManager::write(tr("Running \"%1 %2\" in %3.")
                                .arg(tool->cmakeExecutable().toUserOutput())
                                .arg(args)
                                .arg(workDirectory().toUserOutput()));

    m_future = new QFutureInterface<void>();
    m_future->setProgressRange(0, 1);
    Core::ProgressManager::addTask(m_future->future(),
                                   tr("Configuring \"%1\"").arg(m_buildConfiguration->target()->project()->displayName()),
                                   "CMake.Configure");

    m_cmakeProcess->setCommand(tool->cmakeExecutable().toString(), args);
    m_cmakeProcess->start();
    emit configurationStarted();
}

void BuildDirManager::cmakeFinished(int code, QProcess::ExitStatus status)
{
    QTC_ASSERT(m_cmakeProcess, return);

    // process rest of the output:
    processCMakeOutput();
    processCMakeError();

    cleanUpProcess();

    extractData(); // try even if cmake failed...

    QString msg;
    if (status != QProcess::NormalExit)
        msg = tr("*** cmake process crashed!");
    else if (code != 0)
        msg = tr("*** cmake process exited with exit code %1.").arg(code);

    if (!msg.isEmpty()) {
        Core::MessageManager::write(msg);
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

static QString lineSplit(const QString &rest, const QByteArray &array, std::function<void(const QString &)> f)
{
    QString tmp = rest + Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(array));
    int start = 0;
    int end = tmp.indexOf(QLatin1Char('\n'), start);
    while (end >= 0) {
        f(tmp.mid(start, end - start));
        start = end + 1;
        end = tmp.indexOf(QLatin1Char('\n'), start);
    }
    return tmp.mid(start);
}

void BuildDirManager::processCMakeOutput()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardOutput(), [this](const QString &s) { Core::MessageManager::write(s); });
}

void BuildDirManager::processCMakeError()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardError(), [this](const QString &s) {
        m_parser->stdError(s);
        Core::MessageManager::write(s);
    });
}

QStringList BuildDirManager::getCXXFlagsFor(const CMakeBuildTarget &buildTarget,
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

bool BuildDirManager::extractCXXFlagsFromMake(const CMakeBuildTarget &buildTarget,
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

bool BuildDirManager::extractCXXFlagsFromNinja(const CMakeBuildTarget &buildTarget,
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

void BuildDirManager::checkConfiguration()
{
    if (m_tempDir) // always throw away changes in the tmpdir!
        return;

    Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig cache = parsedConfiguration();
    if (cache.isEmpty())
        return; // No cache file yet.

    CMakeConfig newConfig;
    QSet<QString> changedKeys;
    QSet<QString> removedKeys;
    foreach (const CMakeConfigItem &iBc, intendedConfiguration()) {
        const CMakeConfigItem &iCache
                = Utils::findOrDefault(cache, [&iBc](const CMakeConfigItem &i) { return i.key == iBc.key; });
        if (iCache.isNull()) {
            removedKeys << QString::fromUtf8(iBc.key);
        } else if (QString::fromUtf8(iCache.value) != iBc.expandedValue(k)) {
            changedKeys << QString::fromUtf8(iBc.key);
            newConfig.append(iCache);
        } else {
            newConfig.append(iBc);
        }
    }

    if (!changedKeys.isEmpty() || !removedKeys.isEmpty()) {
        QSet<QString> total = removedKeys + changedKeys;
        QStringList keyList = total.toList();
        Utils::sort(keyList);
        QString table = QLatin1String("<table>");
        foreach (const QString &k, keyList) {
            QString change;
            if (removedKeys.contains(k))
                change = tr("<removed>");
            else
                change = QString::fromUtf8(CMakeConfigItem::valueOf(k.toUtf8(), cache)).trimmed();
            if (change.isEmpty())
                change = tr("<empty>");
            table += QString::fromLatin1("\n<tr><td>%1</td><td>%2</td></tr>").arg(k).arg(change.toHtmlEscaped());
        }
        table += QLatin1String("\n</table>");

        QPointer<QMessageBox> box = new QMessageBox(Core::ICore::mainWindow());
        box->setText(tr("CMake configuration has changed on disk."));
        box->setInformativeText(tr("The CMakeCache.txt file has changed: %1").arg(table));
        box->setStandardButtons(QMessageBox::Discard | QMessageBox::Apply);
        box->setDefaultButton(QMessageBox::Discard);

        int ret = box->exec();
        if (ret == QMessageBox::Apply)
            m_buildConfiguration->setCMakeConfiguration(newConfig);
    }
}

void BuildDirManager::handleDocumentSaves(Core::IDocument *document)
{
    if (!m_cmakeFiles.contains(document->filePath()))
        return;

    m_reparseTimer.start(100);
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

static CMakeConfigItem::Type fromByteArray(const QByteArray &type) {
    if (type == "BOOL")
        return CMakeConfigItem::BOOL;
    if (type == "STRING")
        return CMakeConfigItem::STRING;
    if (type == "FILEPATH")
        return CMakeConfigItem::FILEPATH;
    if (type == "PATH")
        return CMakeConfigItem::PATH;
    QTC_CHECK(type == "INTERNAL" || type == "STATIC");

    return CMakeConfigItem::INTERNAL;
}

CMakeConfig BuildDirManager::parseConfiguration(const Utils::FileName &cacheFile,
                                                QString *errorMessage)
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
        } else if (key.endsWith("-STRINGS") && fromByteArray(type) == CMakeConfigItem::INTERNAL) {
            valuesMap[key.left(key.count() - 8) /* "-STRINGS" */] = value;
        } else {
            CMakeConfigItem::Type t = fromByteArray(type);
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

    Utils::sort(result, CMakeConfigItem::sortOperator());

    return result;
}

void BuildDirManager::handleCmakeFileChange()
{
    Target *t = m_buildConfiguration->target()->project()->activeTarget();
    BuildConfiguration *bc = t ? t->activeBuildConfiguration() : nullptr;

    if (m_buildConfiguration->target() == t && m_buildConfiguration == bc)
        cmakeFilesChanged();
}

void BuildDirManager::maybeForceReparse()
{
    checkConfiguration();

    const QByteArray GENERATOR_KEY = "CMAKE_GENERATOR";
    const QByteArray EXTRA_GENERATOR_KEY = "CMAKE_EXTRA_GENERATOR";
    const QByteArray CMAKE_COMMAND_KEY = "CMAKE_COMMAND";

    const QByteArrayList criticalKeys
            = QByteArrayList() << GENERATOR_KEY << CMAKE_COMMAND_KEY;

    if (!m_hasData) {
        forceReparse();
        return;
    }

    const CMakeConfig currentConfig = parsedConfiguration();

    const CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    QTC_ASSERT(tool, return); // No cmake... we should not have ended up here in the first place
    const QString extraKitGenerator = CMakeGeneratorKitInformation::extraGenerator(kit());
    const QString mainKitGenerator = CMakeGeneratorKitInformation::generator(kit());
    CMakeConfig targetConfig = m_buildConfiguration->cmakeConfiguration();
    targetConfig.append(CMakeConfigItem(GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), mainKitGenerator.toUtf8()));
    if (!extraKitGenerator.isEmpty())
        targetConfig.append(CMakeConfigItem(EXTRA_GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                            QByteArray(), extraKitGenerator.toUtf8()));
    targetConfig.append(CMakeConfigItem(CMAKE_COMMAND_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), tool->cmakeExecutable().toUserOutput().toUtf8()));
    Utils::sort(targetConfig, CMakeConfigItem::sortOperator());

    bool mustReparse = false;
    auto ccit = currentConfig.constBegin();
    auto kcit = targetConfig.constBegin();

    while (ccit != currentConfig.constEnd() && kcit != targetConfig.constEnd()) {
        if (ccit->key == kcit->key) {
            if (ccit->value != kcit->value) {
                if (criticalKeys.contains(kcit->key)) {
                        clearCache();
                        return;
                }
                mustReparse = true;
            }
            ++ccit;
            ++kcit;
        } else {
            if (ccit->key < kcit->key) {
                ++ccit;
            } else {
                ++kcit;
                mustReparse = true;
            }
        }
    }

    // If we have keys that do not exist yet, then reparse.
    //
    // The critical keys *must* be set in cmake configuration, so those were already
    // handled above.
    if (mustReparse || kcit != targetConfig.constEnd())
        forceReparse();
}

} // namespace Internal
} // namespace CMakeProjectManager
