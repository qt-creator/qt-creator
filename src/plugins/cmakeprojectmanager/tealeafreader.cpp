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
#include "cmakeprocess.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
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
    CMakeFile(TeaLeafReader *r, const FilePath &fileName);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    TeaLeafReader *m_reader;
};

CMakeFile::CMakeFile(TeaLeafReader *r, const FilePath &fileName) : m_reader(r)
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
    Q_UNUSED(errorString)
    Q_UNUSED(flag)

    if (type != TypePermissions)
        emit m_reader->dirty();
    return true;
}

// --------------------------------------------------------------------
// TeaLeafReader:
// --------------------------------------------------------------------

TeaLeafReader::TeaLeafReader()
{
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, [this](const IDocument *document) {
        if (m_cmakeFiles.contains(document->filePath())
                || !m_parameters.cmakeTool()
                || !m_parameters.cmakeTool()->isAutoRun())
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

void TeaLeafReader::setParameters(const BuildDirParameters &p)
{
    m_parameters = p;
    emit isReadyNow();
}

bool TeaLeafReader::isCompatible(const BuildDirParameters &p)
{
    return p.cmakeTool() && p.cmakeTool()->readerType() == CMakeTool::TeaLeaf;
}

void TeaLeafReader::resetData()
{
    qDeleteAll(m_watchedFiles);
    m_watchedFiles.clear();

    m_projectName.clear();
    m_buildTargets.clear();
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

void TeaLeafReader::parse(bool forceCMakeRun, bool forceConfiguration)
{
    emit configurationStarted();

    const QString cbpFile = findCbpFile(QDir(m_parameters.workDirectory.toString()));
    const QFileInfo cbpFileFi = cbpFile.isEmpty() ? QFileInfo() : QFileInfo(cbpFile);
    if (!cbpFileFi.exists() || forceConfiguration) {
        // Initial create:
        startCMake(CMakeProcess::toArguments(m_parameters.configuration, m_parameters.expander));
        return;
    }

    const bool mustUpdate = forceCMakeRun || m_cmakeFiles.isEmpty()
            || anyOf(m_cmakeFiles, [&cbpFileFi](const FilePath &f) {
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
    m_cmakeProcess.reset();
}

bool TeaLeafReader::isParsing() const
{
    return m_cmakeProcess && m_cmakeProcess->state() != QProcess::NotRunning;
}

QList<CMakeBuildTarget> TeaLeafReader::takeBuildTargets(QString &errorMessage)
{
    Q_UNUSED(errorMessage)
    return m_buildTargets;
}

CMakeConfig TeaLeafReader::takeParsedConfiguration(QString &errorMessage)
{
    const FilePath cacheFile = m_parameters.workDirectory.pathAppended("CMakeCache.txt");

    if (!cacheFile.exists())
        return { };

    CMakeConfig result = BuildDirManager::parseCMakeConfiguration(cacheFile, &errorMessage);

    if (!errorMessage.isEmpty()) {
        return { };
    }

    const FilePath sourceOfBuildDir
            = FilePath::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", result));
    const FilePath canonicalSourceOfBuildDir = sourceOfBuildDir.canonicalPath();
    const FilePath canonicalSourceDirectory = m_parameters.sourceDirectory.canonicalPath();
    if (canonicalSourceOfBuildDir != canonicalSourceDirectory) { // Uses case-insensitive compare where appropriate
        errorMessage = tr("The build directory is not for %1 but for %2")
                          .arg(canonicalSourceOfBuildDir.toUserOutput(),
                               canonicalSourceDirectory.toUserOutput());
        return { };
    }
    return result;
}

std::unique_ptr<CMakeProjectNode> TeaLeafReader::generateProjectTree(
    const QList<const FileNode *> &allFiles, QString &errorMessage)
{
    Q_UNUSED(errorMessage)
    if (m_files.size() == 0)
        return {};

    auto root = std::make_unique<CMakeProjectNode>(m_parameters.sourceDirectory);
    root->setDisplayName(m_projectName);

    // Delete no longer necessary file watcher based on m_cmakeFiles:
    const QSet<FilePath> currentWatched
            = transform(m_watchedFiles, &CMakeFile::filePath);
    const QSet<FilePath> toWatch = m_cmakeFiles;
    QSet<FilePath> toDelete = currentWatched;
    toDelete.subtract(toWatch);
    m_watchedFiles = filtered(m_watchedFiles, [&toDelete](Internal::CMakeFile *cmf) {
            if (toDelete.contains(cmf->filePath())) {
                delete cmf;
                return false;
            }
            return true;
        });

    // Add new file watchers:
    QSet<FilePath> toAdd = toWatch;
    toAdd.subtract(currentWatched);
    foreach (const FilePath &fn, toAdd) {
        auto cm = new CMakeFile(this, fn);
        DocumentManager::addDocument(cm);
        m_watchedFiles.insert(cm);
    }

    QSet<FilePath> allIncludePathSet;
    for (const CMakeBuildTarget &bt : m_buildTargets) {
        const QList<Utils::FilePath> targetIncludePaths
                = Utils::filtered(bt.includeFiles, [this](const Utils::FilePath &fn) {
            return fn.isChildOf(m_parameters.sourceDirectory);
        });
        allIncludePathSet.unite(Utils::toSet(targetIncludePaths));
    }
    const QList<FilePath> allIncludePaths = Utils::toList(allIncludePathSet);

    const QList<const FileNode *> missingHeaders
            = Utils::filtered(allFiles, [&allIncludePaths](const FileNode *fn) -> bool {
        if (fn->fileType() != FileType::Header)
            return false;

        return Utils::contains(allIncludePaths, [fn](const FilePath &inc) { return fn->filePath().isChildOf(inc); });
    });

    // filter duplicates:
    auto alreadySeen = Utils::transform<QSet>(m_files, &FileNode::filePath);
    const QList<const FileNode *> unseenMissingHeaders = Utils::filtered(missingHeaders, [&alreadySeen](const FileNode *fn) {
        const int count = alreadySeen.count();
        alreadySeen.insert(fn->filePath());
        return (alreadySeen.count() != count);
    });

    root->addNestedNodes(std::move(m_files), m_parameters.sourceDirectory);

    std::vector<std::unique_ptr<FileNode>> fileNodes
            = transform<std::vector>(unseenMissingHeaders, [](const FileNode *fn) {
        return std::unique_ptr<FileNode>(fn->clone());
    });
    root->addNestedNodes(std::move(fileNodes), m_parameters.sourceDirectory);

    return root;
}

static void processCMakeIncludes(const CMakeBuildTarget &cbt, const ToolChain *tc,
                                 const QStringList& flags, const FilePath &sysroot,
                                 QSet<FilePath> &tcIncludes, QStringList &includePaths)
{
    if (!tc)
        return;

    foreach (const HeaderPath &hp, tc->builtInHeaderPaths(flags, sysroot,
                                                          Environment::systemEnvironment())) {
        tcIncludes.insert(FilePath::fromString(hp.path));
    }
    foreach (const FilePath &i, cbt.includeFiles) {
        if (!tcIncludes.contains(i))
            includePaths.append(i.toString());
    }
}

CppTools::RawProjectParts TeaLeafReader::createRawProjectParts(QString &errorMessage)
{
    Q_UNUSED(errorMessage)
    const ToolChain *tcCxx = ToolChainManager::findToolChain(m_parameters.cxxToolChainId);
    const ToolChain *tcC = ToolChainManager::findToolChain(m_parameters.cToolChainId);
    const FilePath sysroot = m_parameters.sysRoot;

    CppTools::RawProjectParts rpps;
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
        QSet<FilePath> tcIncludes;
        QStringList includePaths;
        if (tcCxx || tcC) {
            processCMakeIncludes(cbt, tcCxx, cxxflags, sysroot, tcIncludes, includePaths);
            processCMakeIncludes(cbt, tcC, cflags, sysroot, tcIncludes, includePaths);
        } else {
            includePaths = transform(cbt.includeFiles, &FilePath::toString);
        }
        includePaths += m_parameters.workDirectory.toString();
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
        rpp.setFiles(transform(cbt.files, &FilePath::toString));

        const bool isExecutable = cbt.targetType == ExecutableType;
        rpp.setBuildTargetType(isExecutable ? CppTools::ProjectPart::Executable
                                            : CppTools::ProjectPart::Library);
        rpps.append(rpp);
    }

    return rpps;
}

void TeaLeafReader::extractData()
{
    CMakeTool *cmake = m_parameters.cmakeTool();
    QTC_ASSERT(m_parameters.isValid() && cmake, return);

    const FilePath srcDir = m_parameters.sourceDirectory;
    const FilePath bldDir = m_parameters.workDirectory;
    const FilePath topCMake = srcDir.pathAppended("CMakeLists.txt");

    resetData();

    m_projectName = m_parameters.projectName;
    m_files.emplace_back(std::make_unique<FileNode>(topCMake, FileType::Project));
    // Do not insert topCMake into m_cmakeFiles: The project already watches that!

    // Find cbp file
    FilePath cbpFile = FilePath::fromString(findCbpFile(bldDir.toString()));
    if (cbpFile.isEmpty())
        return;
    m_cmakeFiles.insert(cbpFile);

    // Add CMakeCache.txt file:
    const FilePath cacheFile = m_parameters.workDirectory.pathAppended("CMakeCache.txt");
    if (cacheFile.exists())
        m_cmakeFiles.insert(cacheFile);

    // setFolderName
    CMakeCbpParser cbpparser;
    // Parsing
    if (!cbpparser.parseCbpFile(cmake->pathMapper(), cbpFile, srcDir))
        return;

    m_projectName = cbpparser.projectName();

    m_files = cbpparser.takeFileList();
    if (cbpparser.hasCMakeFiles()) {
        std::vector<std::unique_ptr<FileNode>> cmakeNodes = cbpparser.takeCmakeFileList();
        for (const std::unique_ptr<FileNode> &node : cmakeNodes)
            m_cmakeFiles.insert(node->filePath());

        std::move(std::begin(cmakeNodes), std::end(cmakeNodes), std::back_inserter(m_files));
    }

    // Make sure the top cmakelists.txt file is always listed:
    if (!contains(m_files, [topCMake](const std::unique_ptr<FileNode> &fn) {
                      return fn->filePath() == topCMake;
                  }))
        m_files.emplace_back(std::make_unique<FileNode>(topCMake, FileType::Project));

    m_buildTargets = cbpparser.buildTargets();
}

void TeaLeafReader::startCMake(const QStringList &configurationArguments)
{
    QTC_ASSERT(!m_cmakeProcess, return);

    m_cmakeProcess = std::make_unique<CMakeProcess>();

    connect(m_cmakeProcess.get(), &CMakeProcess::finished,
            this, &TeaLeafReader::cmakeFinished);

    m_cmakeProcess->run(m_parameters, configurationArguments);
}

void TeaLeafReader::cmakeFinished(int code, QProcess::ExitStatus status)
{
    Q_UNUSED(code)
    Q_UNUSED(status)

    QTC_ASSERT(m_cmakeProcess, return);
    m_cmakeProcess.reset();

    extractData(); // try even if cmake failed...

    emit dataAvailable();
}

QStringList TeaLeafReader::getFlagsFor(const CMakeBuildTarget &buildTarget,
                                       QHash<QString, QStringList> &cache,
                                       Id lang) const
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
                                         Id lang) const
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
                                          Id lang) const
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
    QString buildNinjaFile = m_buildTargets.at(0).workingDirectory.toString();
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
