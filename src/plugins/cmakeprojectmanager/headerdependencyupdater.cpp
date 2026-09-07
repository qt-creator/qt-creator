// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "headerdependencyupdater.h"

#include "cmakeprojectmanagertr.h"

#include <coreplugin/progressmanager/taskprogress.h>

#include <cppeditor/cppprojectfile.h>

#include <utils/algorithm.h>

#include <QDateTime>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static ProjectFile::Kind fileKind(const RawProjectPart &part, const FilePath &file)
{
    if (part.getMimeType)
        return ProjectFile::classifyByMimeType(part.getMimeType(file));
    return ProjectFile::classify(file);
}

/*!
    Returns the arguments that reproduce how the build system compiles a file of
    \a flags, \a macros and \a headerPaths, in the flavor that \a dialect wants.

    The built-in header paths are left out on purpose. The compiler knows them
    already, and passing them back as ordinary include directories would put
    them ahead of their proper place in the search order.
*/
QStringList scanArguments(const RawProjectPartFlags &flags,
                          const Macros &macros,
                          const HeaderPaths &headerPaths,
                          DependencyDialect dialect)
{
    const bool msvc = dialect == DependencyDialect::Msvc;

    QStringList arguments = flags.commandLineFlags;

    for (const Macro &macro : macros) {
        if (macro.type == MacroType::Define)
            arguments.append((msvc ? "/D" : "-D") + QString::fromUtf8(macro.toKeyValue({})));
        else if (macro.type == MacroType::Undefine)
            arguments.append((msvc ? "/U" : "-U") + QString::fromUtf8(macro.key));
    }

    for (const HeaderPath &headerPath : headerPaths) {
        const QString path = headerPath.path.nativePath();
        switch (headerPath.type) {
        case HeaderPathType::BuiltIn:
            break;
        case HeaderPathType::User:
            arguments.append((msvc ? "/I" : "-I") + path);
            break;
        case HeaderPathType::System:
            if (msvc)
                arguments.append("/I" + path);
            else
                arguments << "-isystem" << path;
            break;
        case HeaderPathType::Framework:
            if (!msvc)
                arguments.append("-F" + path);
            break;
        }
    }

    return arguments;
}

/*!
    Returns one scan unit per translation unit of \a parts, using the compiler of
    \a cToolchain or \a cxxToolchain as the language of the source requires.

    A source that several project parts compile is scanned once, with the
    arguments of the first part that names it. Scanning it once per part would
    store a result per part under the same source and leave every part but the
    last permanently stale.
*/
HeaderScanUnits planHeaderScan(const RawProjectParts &parts,
                               const ToolchainInfo &cToolchain,
                               const ToolchainInfo &cxxToolchain)
{
    HeaderScanUnits units;
    QSet<FilePath> planned;

    for (const RawProjectPart &part : parts) {
        for (const FilePath &file : part.files) {
            const ProjectFile::Kind kind = fileKind(part, file);
            if (!ProjectFile::isSource(kind))
                continue;

            if (planned.contains(file))
                continue;

            const bool isC = ProjectFile::isC(kind);
            const ToolchainInfo &toolchain = isC ? cToolchain : cxxToolchain;
            if (!toolchain.isValid() || toolchain.compilerFilePath.isEmpty())
                continue;

            planned.insert(file);

            HeaderScanUnit unit;
            unit.compiler = toolchain.compilerFilePath;
            unit.source = file;
            unit.arguments = scanArguments(isC ? part.flagsForC : part.flagsForCxx,
                                           part.projectMacros,
                                           part.headerPaths,
                                           dependencyDialect(toolchain.type));
            units.append(unit);
        }
    }

    return units;
}

/*!
    Returns \a parts with the project's own headers that \a store recorded for
    their sources added to their files.

    Only the headers below \a sourceDirectory or \a buildDirectory are added. The
    rest come from the toolchain or an installed package, and the include paths
    of the project part already cover them.
*/
RawProjectParts withDiscoveredHeaders(const RawProjectParts &parts,
                                      const HeaderDependencyStore &store,
                                      const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory)
{
    RawProjectParts result = parts;

    for (RawProjectPart &part : result) {
        FilePaths discovered;
        for (const FilePath &file : part.files) {
            if (!ProjectFile::isSource(fileKind(part, file)))
                continue;

            discovered += projectHeaders(store.dependencies(file),
                                         sourceDirectory,
                                         buildDirectory);
        }

        if (discovered.isEmpty())
            continue;

        FilePaths files = part.files + discovered;
        FilePath::removeDuplicates(files);
        part.setFiles(files);
    }

    return result;
}

void HeaderDependencyUpdater::setStoreFile(const FilePath &storeFile)
{
    if (m_storeFile == storeFile)
        return;

    m_storeFile = storeFile;
    m_store.clear();
    m_storeLoaded = false;
}

void HeaderDependencyUpdater::setProjectDirectories(const FilePath &sourceDirectory,
                                                    const FilePath &buildDirectory)
{
    m_sourceDirectory = sourceDirectory;
    m_buildDirectory = buildDirectory;
}

void HeaderDependencyUpdater::setShowIncludesPrefix(const QString &prefix)
{
    m_showIncludesPrefix = prefix;
}

void HeaderDependencyUpdater::cancel()
{
    m_taskTreeRunner.reset();
}

void HeaderDependencyUpdater::loadStore()
{
    if (m_storeLoaded || m_storeFile.isEmpty())
        return;

    m_store.load(m_storeFile);
    m_storeLoaded = true;
}

/*!
    Returns the project's own headers that were recorded for each scanned
    source, for the project tree to show alongside the sources.
*/
QHash<FilePath, FilePaths> HeaderDependencyUpdater::projectHeaderMap()
{
    loadStore();

    QHash<FilePath, FilePaths> result;
    const FilePaths sources = m_store.sources();
    for (const FilePath &source : sources) {
        const FilePaths headers = projectHeaders(m_store.dependencies(source),
                                                 m_sourceDirectory,
                                                 m_buildDirectory);
        if (!headers.isEmpty())
            result.insert(source, headers);
    }
    return result;
}

class ScanOutcome
{
public:
    bool recorded = false;
    bool changed = false;
};

void HeaderDependencyUpdater::update(const ProjectUpdateInfo &info,
                                     const Environment &environment,
                                     const Handler &handler)
{
    cancel();

    if (m_storeFile.isEmpty())
        return;

    loadStore();

    const HeaderScanUnits units = planHeaderScan(info.rawProjectParts,
                                                 info.cToolchainInfo,
                                                 info.cxxToolchainInfo);
    if (units.isEmpty())
        return;

    const bool pruned = m_store.retainSources(Utils::transform(units, &HeaderScanUnit::source));

    m_store.forgetTimes();
    const SourceScans stale = m_store.staleScans(
        Utils::transform(units, &HeaderScanUnit::toScan));
    const QSet<FilePath> staleSources = Utils::transform<QSet>(stale, &SourceScan::source);

    const RawProjectParts parts = info.rawProjectParts;
    const auto report = [this, parts, handler](bool storedResultsChanged) {
        handler(withDiscoveredHeaders(parts, m_store, m_sourceDirectory, m_buildDirectory),
                storedResultsChanged);
    };

    if (staleSources.isEmpty()) {
        if (pruned)
            m_store.save(m_storeFile);
        report(false);
        return;
    }

    HeaderScanSettings settings;
    settings.dialect = dependencyDialect(info.cxxToolchainInfo.isValid()
                                             ? info.cxxToolchainInfo.type
                                             : info.cToolchainInfo.type);
    settings.workingDirectory = m_buildDirectory;
    settings.environment = environment;
    settings.showIncludesPrefix = m_showIncludesPrefix;

    // Recorded before the scan starts, so a header edited while it runs leaves
    // the source stale instead of looking freshly scanned.
    const qint64 scanTime = QDateTime::currentMSecsSinceEpoch();

    const auto outcome = std::make_shared<ScanOutcome>();
    const auto onScanned = [this, scanTime, outcome](const SourceScan &scan,
                                                     const FilePaths &dependencies) {
        outcome->recorded = true;
        if (m_store.insert(scan, dependencies, scanTime))
            outcome->changed = true;
    };

    const HeaderScanUnits staleUnits = Utils::filtered(units,
                                                       [&staleSources](const HeaderScanUnit &unit) {
                                                           return staleSources.contains(unit.source);
                                                       });

    const Group recipe{headerDependencyScanRecipe(staleUnits, settings, onScanned)};

    m_taskTreeRunner.start(
        recipe,
        [](QTaskTree &taskTree) {
            auto progress = new Core::TaskProgress(&taskTree);
            progress->setDisplayName(Tr::tr("Scanning Header Dependencies"));
        },
        [this, report, pruned, outcome](DoneWith result) {
            if (result == DoneWith::Cancel)
                return;

            if (outcome->recorded || pruned)
                m_store.save(m_storeFile);
            report(outcome->changed);
        });
}

} // namespace CMakeProjectManager::Internal
