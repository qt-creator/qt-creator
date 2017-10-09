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

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakecbpparser.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/reaper.h>
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

// --------------------------------------------------------------------
// TeaLeafReader:
// --------------------------------------------------------------------

TeaLeafReader::TeaLeafReader()
{
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, [this](const IDocument *document) {
        if (m_cmakeFiles.contains(document->filePath())
                || !m_parameters.cmakeTool
                || !m_parameters.cmakeTool->isAutoRun())
            emit dirty();
    });

    // Remove \' (quote) for function-style macrosses:
    //  -D'MACRO()'=xxx
    //  -D'MACRO()=xxx'
    //  -D'MACRO()'
    // otherwise, compiler will fails
    m_macroFixupRe1.setPattern("^-D(\\s*)'([0-9a-zA-Z_\\(\\)]+)'=");
    m_macroFixupRe2.setPattern("^-D(\\s*)'([0-9a-zA-Z_\\(\\)]+)=(.+)'$");
    m_macroFixupRe3.setPattern("^-D(\\s*)'([0-9a-zA-Z_\\(\\)]+)'$");
}

TeaLeafReader::~TeaLeafReader()
{
    stop();
    resetData();
}

bool TeaLeafReader::isCompatible(const BuildDirParameters &p)
{
    if (!p.cmakeTool)
        return false;
    return !p.cmakeTool->hasServerMode();
}

void TeaLeafReader::resetData()
{
    qDeleteAll(m_watchedFiles);
    m_watchedFiles.clear();

    m_projectName.clear();
    m_buildTargets.clear();
    qDeleteAll(m_files);
    m_files.clear();
}

static QString findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   the cbp file is named like the project() command in the CMakeList.txt file
    //   so this function below could find the wrong cbp file, if the user changes the project()
    //   2name
    QDateTime t;
    QString file;
    foreach (const QString &cbpFile , directory.entryList()) {
        if (cbpFile.endsWith(QLatin1String(".cbp"))) {
            QFileInfo fi(directory.path() + QLatin1Char('/') + cbpFile);
            if (t.isNull() || fi.lastModified() > t) {
                file = directory.path() + QLatin1Char('/') + cbpFile;
                t = fi.lastModified();
            }
        }
    }
    return file;
}

void TeaLeafReader::parse(bool forceConfiguration)
{
    const QString cbpFile = findCbpFile(QDir(m_parameters.buildDirectory.toString()));
    const QFileInfo cbpFileFi = cbpFile.isEmpty() ? QFileInfo() : QFileInfo(cbpFile);
    if (!cbpFileFi.exists() || forceConfiguration) {
        // Initial create:
        startCMake(toArguments(m_parameters.configuration, m_parameters.expander));
        return;
    }

    const bool mustUpdate = m_cmakeFiles.isEmpty()
            || anyOf(m_cmakeFiles, [&cbpFileFi](const FileName &f) {
                   return f.toFileInfo().lastModified() > cbpFileFi.lastModified();
               });
    if (mustUpdate) {
        startCMake(QStringList());
    } else {
        extractData();
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

QList<CMakeBuildTarget> TeaLeafReader::takeBuildTargets()
{
    return m_buildTargets;
}

CMakeConfig TeaLeafReader::takeParsedConfiguration()
{
    FileName cacheFile = m_parameters.buildDirectory;
    cacheFile.appendPath(QLatin1String("CMakeCache.txt"));

    if (!cacheFile.exists())
        return { };

    QString errorMessage;
    CMakeConfig result = BuildDirManager::parseCMakeConfiguration(cacheFile, &errorMessage);

    if (!errorMessage.isEmpty()) {
        emit errorOccured(errorMessage);
        return { };
    }

    const FileName sourceOfBuildDir
            = FileName::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", result));
    const FileName canonicalSourceOfBuildDir = FileUtils::canonicalPath(sourceOfBuildDir);
    const FileName canonicalSourceDirectory = FileUtils::canonicalPath(m_parameters.sourceDirectory);
    if (canonicalSourceOfBuildDir != canonicalSourceDirectory) { // Uses case-insensitive compare where appropriate
        emit errorOccured(tr("The build directory is not for %1 but for %2")
                          .arg(canonicalSourceOfBuildDir.toUserOutput(),
                               canonicalSourceDirectory.toUserOutput()));
        return { };
    }
    return result;
}

void TeaLeafReader::generateProjectTree(CMakeProjectNode *root, const QList<const FileNode *> &allFiles)
{
    if (m_files.isEmpty())
        return;

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

    QSet<FileName> allIncludePathSet;
    for (const CMakeBuildTarget &bt : m_buildTargets) {
        const QList<Utils::FileName> targetIncludePaths
                = Utils::filtered(bt.includeFiles, [this](const Utils::FileName &fn) {
            return fn.isChildOf(m_parameters.sourceDirectory);
        });
        allIncludePathSet.unite(QSet<FileName>::fromList(targetIncludePaths));
    }
    const QList<FileName> allIncludePaths = allIncludePathSet.toList();

    const QList<const FileNode *> missingHeaders
            = Utils::filtered(allFiles, [&allIncludePaths](const FileNode *fn) -> bool {
        if (fn->fileType() != FileType::Header)
            return false;

        return Utils::contains(allIncludePaths, [fn](const FileName &inc) { return fn->filePath().isChildOf(inc); });
    });

    // filter duplicates:
    auto alreadySeen = QSet<FileName>::fromList(Utils::transform(m_files, &FileNode::filePath));
    const QList<const FileNode *> unseenMissingHeaders = Utils::filtered(missingHeaders, [&alreadySeen](const FileNode *fn) {
        const int count = alreadySeen.count();
        alreadySeen.insert(fn->filePath());
        return (alreadySeen.count() != count);
    });

    const QList<FileNode *> fileNodes = m_files
            + Utils::transform(unseenMissingHeaders, [](const FileNode *fn) { return fn->clone(); });

    root->addNestedNodes(fileNodes, m_parameters.sourceDirectory);
    m_files.clear(); // Some of the FileNodes in files() were deleted!
}

static void processCMakeIncludes(const CMakeBuildTarget &cbt, const ToolChain *tc,
                                 const QStringList& flags, const FileName &sysroot,
                                 QSet<FileName> &tcIncludes, QStringList &includePaths)
{
    if (!tc)
        return;

    foreach (const HeaderPath &hp, tc->systemHeaderPaths(flags, sysroot))
        tcIncludes.insert(FileName::fromString(hp.path()));
    foreach (const FileName &i, cbt.includeFiles) {
        if (!tcIncludes.contains(i))
            includePaths.append(i.toString());
    }
}

void TeaLeafReader::updateCodeModel(CppTools::RawProjectParts &rpps)
{
    const ToolChain *tcCxx = ToolChainManager::findToolChain(m_parameters.cxxToolChainId);
    const ToolChain *tcC = ToolChainManager::findToolChain(m_parameters.cToolChainId);
    const FileName sysroot = m_parameters.sysRoot;

    QHash<QString, QStringList> targetDataCacheCxx;
    QHash<QString, QStringList> targetDataCacheC;
    foreach (const CMakeBuildTarget &cbt, m_buildTargets) {
        if (cbt.targetType == UtilityType)
            continue;

        // CMake shuffles the include paths that it reports via the CodeBlocks generator
        // So remove the toolchain include paths, so that at least those end up in the correct
        // place.
        auto cxxflags = getFlagsFor(cbt, targetDataCacheCxx, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        auto cflags = getFlagsFor(cbt, targetDataCacheC, ProjectExplorer::Constants::C_LANGUAGE_ID);
        QSet<FileName> tcIncludes;
        QStringList includePaths;
        if (tcCxx || tcC) {
            processCMakeIncludes(cbt, tcCxx, cxxflags, sysroot, tcIncludes, includePaths);
            processCMakeIncludes(cbt, tcC, cflags, sysroot, tcIncludes, includePaths);
        } else {
            includePaths = transform(cbt.includeFiles, &FileName::toString);
        }
        includePaths += m_parameters.buildDirectory.toString();
        CppTools::RawProjectPart rpp;
        rpp.setProjectFileLocation(cbt.sourceDirectory.toString() + "/CMakeLists.txt");
        rpp.setBuildSystemTarget(cbt.title);
        rpp.setIncludePaths(includePaths);

        CppTools::RawProjectPartFlags cProjectFlags;
        cProjectFlags.commandLineFlags = cflags;
        rpp.setFlagsForC(cProjectFlags);

        CppTools::RawProjectPartFlags cxxProjectFlags;
        cxxProjectFlags.commandLineFlags = cxxflags;
        rpp.setFlagsForCxx(cxxProjectFlags);

        rpp.setMacros(cbt.macros);
        rpp.setDisplayName(cbt.title);
        rpp.setFiles(transform(cbt.files, [](const FileName &fn) { return fn.toString(); }));

        const bool isExecutable = cbt.targetType == ExecutableType;
        rpp.setBuildTargetType(isExecutable ? CppTools::ProjectPart::Executable
                                            : CppTools::ProjectPart::Library);
        rpps.append(rpp);
    }
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
    QTC_ASSERT(m_parameters.isValid() && m_parameters.cmakeTool, return);

    const FileName srcDir = m_parameters.sourceDirectory;
    const FileName bldDir = m_parameters.buildDirectory;
    const FileName topCMake = Utils::FileName(srcDir).appendPath("CMakeLists.txt");

    resetData();

    m_projectName = m_parameters.projectName;
    m_files.append(new FileNode(topCMake, FileType::Project, false));
    // Do not insert topCMake into m_cmakeFiles: The project already watches that!

    // Find cbp file
    FileName cbpFile = FileName::fromString(findCbpFile(bldDir.toString()));
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
    if (!cbpparser.parseCbpFile(m_parameters.cmakeTool->pathMapper(), cbpFile, srcDir))
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
    QTC_ASSERT(m_parameters.isValid() && m_parameters.cmakeTool, return);

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
                          .arg(m_parameters.cmakeTool->cmakeExecutable().toUserOutput())
                          .arg(args)
                          .arg(buildDirectory.toUserOutput()));

    m_future = new QFutureInterface<void>();
    m_future->setProgressRange(0, 1);
    ProgressManager::addTask(m_future->future(),
                             tr("Configuring \"%1\"").arg(m_parameters.projectName),
                             "CMake.Configure");

    m_cmakeProcess->setCommand(m_parameters.cmakeTool->cmakeExecutable().toString(), args);
    emit configurationStarted();
    m_cmakeProcess->start();
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

    emit dataAvailable();
}

void TeaLeafReader::processCMakeOutput()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardOutput(),
                     [](const QString &s) { MessageManager::write(s); });
}

void TeaLeafReader::processCMakeError()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardError(), [this](const QString &s) {
        m_parser->stdError(s);
        MessageManager::write(s);
    });
}

QStringList TeaLeafReader::getFlagsFor(const CMakeBuildTarget &buildTarget,
                                       QHash<QString, QStringList> &cache,
                                       Id lang)
{
    // check cache:
    auto it = cache.constFind(buildTarget.title);
    if (it != cache.constEnd())
        return *it;

    if (extractFlagsFromMake(buildTarget, cache, lang))
        return cache.value(buildTarget.title);

    if (extractFlagsFromNinja(buildTarget, cache, lang))
        return cache.value(buildTarget.title);

    cache.insert(buildTarget.title, QStringList());
    return QStringList();
}

bool TeaLeafReader::extractFlagsFromMake(const CMakeBuildTarget &buildTarget,
                                         QHash<QString, QStringList> &cache,
                                         Id lang)
{
    QString flagsPrefix;

    if (lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        flagsPrefix = QLatin1String("CXX_FLAGS =");
    else if (lang == ProjectExplorer::Constants::C_LANGUAGE_ID)
        flagsPrefix = QLatin1String("C_FLAGS =");
    else
        return false;

    QString makeCommand = buildTarget.makeCommand.toString();
    int startIndex = makeCommand.indexOf('\"');
    int endIndex = makeCommand.indexOf('\"', startIndex + 1);
    if (startIndex != -1 && endIndex != -1) {
        startIndex += 1;
        QString makefile = makeCommand.mid(startIndex, endIndex - startIndex);
        int slashIndex = makefile.lastIndexOf('/');
        makefile.truncate(slashIndex);
        makefile.append("/CMakeFiles/" + buildTarget.title + ".dir/flags.make");
        // Remove un-needed shell escaping:
        makefile = makefile.remove("\\");
        QFile file(makefile);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return false;
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.startsWith(flagsPrefix)) {
                    // Skip past =
                    auto flags =
                        Utils::transform(line.mid(flagsPrefix.length()).trimmed().split(' ', QString::SkipEmptyParts), [this](QString flag) -> QString {
                            // TODO: maybe Gcc-specific
                            // Remove \' (quote) for function-style macrosses:
                            //  -D'MACRO()'=xxx
                            //  -D'MACRO()=xxx'
                            //  -D'MACRO()'
                            // otherwise, compiler will fails
                            return flag
                                    .replace(m_macroFixupRe1, "-D\\1\\2=")
                                    .replace(m_macroFixupRe2, "-D\\1\\2=\\3")
                                    .replace(m_macroFixupRe3, "-D\\1\\2");
                        });
                    cache.insert(buildTarget.title, flags);
                    return true;
                }
            }
        }
    }
    return false;
}

bool TeaLeafReader::extractFlagsFromNinja(const CMakeBuildTarget &buildTarget,
                                          QHash<QString, QStringList> &cache,
                                          Id lang)
{
    Q_UNUSED(buildTarget)
    if (!cache.isEmpty()) // We fill the cache in one go!
        return false;

    QString compilerPrefix;
    if (lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        compilerPrefix = QLatin1String("CXX_COMPILER");
    else if (lang == ProjectExplorer::Constants::C_LANGUAGE_ID)
        compilerPrefix = QLatin1String("C_COMPILER");
    else
        return false;

    // Attempt to find build.ninja file and obtain FLAGS (CXX_FLAGS/C_FLAGS) from there if no suitable flags.make were
    // found
    // Get "all" target's working directory
    QByteArray ninjaFile;
    QString buildNinjaFile = takeBuildTargets().at(0).workingDirectory.toString();
    buildNinjaFile += "/build.ninja";
    QFile buildNinja(buildNinjaFile);
    if (buildNinja.exists()) {
        if (!buildNinja.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        ninjaFile = buildNinja.readAll();
        buildNinja.close();
    }

    if (ninjaFile.isEmpty())
        return false;

    QTextStream stream(ninjaFile);
    bool compilerFound = false;
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
            compilerFound = line.indexOf(compilerPrefix) != -1;
        } else if (compilerFound && line.startsWith("FLAGS =")) {
            // Skip past =
            cache.insert(currentTarget, line.mid(7).trimmed().split(' ', QString::SkipEmptyParts));
        }
    }
    return !cache.isEmpty();
}

} // namespace Internal
} // namespace CMakeProjectManager
