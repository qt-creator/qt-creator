// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolscompilationdb.h"

#include "clangtoolsprojectsettings.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compilationdb.h>
#include <cppeditor/cppmodelmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/temporarydirectory.h>

#include <QFutureWatcher>
#include <QHash>

#include <utility>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools::Internal {
namespace {
static QHash<std::pair<ClangToolType, BuildConfiguration *>, ClangToolsCompilationDb *> dbs;
} // namespace

class ClangToolsCompilationDb::Private
{
public:
    Private(ClangToolType toolType, ClangToolsCompilationDb *q) : q(q), toolType(toolType) {}

    void generate();
    QString toolName() const { return clangToolName(toolType); }

    ClangToolsCompilationDb * const q;
    const ClangToolType toolType;
    TemporaryDirectory dir{toolName() + "XXXXXX"};
    QFutureWatcher<GenerateCompilationDbResult> generatorWatcher;
    FutureSynchronizer generatorSynchronizer;
    bool readyAndUpToDate = false;
};

ClangToolsCompilationDb::ClangToolsCompilationDb(ClangToolType toolType, BuildConfiguration *bc)
    : QObject(bc), d(new Private(toolType, this))
{
    connect(bc, &BuildConfiguration::destroyed, [bc, toolType] {
        dbs.remove(std::make_pair(toolType, bc));
    });

    connect(&d->generatorWatcher, &QFutureWatcher<GenerateCompilationDbResult>::finished,
            this, [this] {
        const auto result = d->generatorWatcher.result();
        const bool success = result.has_value();
        QTC_CHECK(!d->readyAndUpToDate);
        d->readyAndUpToDate = success;
        if (success) {
            Core::MessageManager::writeSilently(
                Tr::tr("Compilation database for %1 successfully generated at \"%2\".")
                    .arg(d->toolName(), d->dir.path().toUserOutput()));
        } else {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Generating compilation database for %1 failed: %2")
                    .arg(d->toolName(), result.error()));
        }
        emit generated(success);
    });

    connect(ClangToolsProjectSettings::getSettings(bc->project()).get(),
            &ClangToolsProjectSettings::changed,
            this, &ClangToolsCompilationDb::invalidate);
    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated, this,
        [this, bc](Project *p) {
            if (p == bc->project())
                invalidate();
        });
}

ClangToolsCompilationDb::~ClangToolsCompilationDb() { delete d; }

void ClangToolsCompilationDb::invalidate() { d->readyAndUpToDate = false; }

FilePath ClangToolsCompilationDb::parentDir() const { return d->dir.path(); }

bool ClangToolsCompilationDb::generateIfNecessary()
{
    if (d->readyAndUpToDate)
        return false;
    d->generate();
    return true;
}

ClangToolsCompilationDb &ClangToolsCompilationDb::getDb(
    ClangToolType toolType, BuildConfiguration *bc)
{
    const auto key = std::make_pair(toolType, bc);
    if (const auto it = dbs.constFind(key); it != dbs.constEnd())
        return *it.value();
    const auto db = new ClangToolsCompilationDb(toolType, bc);
    dbs.insert(key, db);
    return *db;
}

void ClangToolsCompilationDb::Private::generate()
{
    QTC_CHECK(!readyAndUpToDate);

    if (generatorWatcher.isRunning())
        generatorWatcher.cancel();

    Core::MessageManager::writeSilently(
        Tr::tr("Generating compilation database for %1 at \"%2\" ...")
            .arg(clangToolName(toolType), dir.path().toUserOutput()));

    const auto bc = static_cast<BuildConfiguration *>(q->parent());
    const auto projectSettings = ClangToolsProjectSettings::getSettings(bc->project());
    QTC_ASSERT(projectSettings, return);
    const Id configId = projectSettings->runSettings().diagnosticConfigId();
    const ClangDiagnosticConfig config = Utils::findOrDefault(
        ClangToolsSettings::instance()->diagnosticConfigs(),
        [configId](const ClangDiagnosticConfig &c) { return c.id() == configId; });
    const auto useBuildSystemWarnings = config.useBuildSystemWarnings()
                                            ? UseBuildSystemWarnings::Yes
                                            : UseBuildSystemWarnings::No;
    const FilePath executable = toolExecutable(toolType);
    const auto [includeDir, clangVersion] = getClangIncludeDirAndVersion(executable);
    const FilePath actualClangIncludeDir = Core::ICore::clangIncludeDirectory(
        clangVersion, includeDir);
    const auto getCompilerOptionsBuilder = [=](const ProjectPart &pp) {
        CompilerOptionsBuilder optionsBuilder(pp,
                                              UseSystemHeader::No,
                                              UseTweakedHeaderPaths::Tools,
                                              UseLanguageDefines::No,
                                              useBuildSystemWarnings,
                                              actualClangIncludeDir);
        optionsBuilder.build(ProjectFile::Unclassified, UsePrecompiledHeaders::No);
        if (useBuildSystemWarnings == UseBuildSystemWarnings::No) {
            for (const QString &opt : config.clangOptions())
                optionsBuilder.add(opt, true);
        }
        const QStringList extraArgsToPrepend = extraClangToolsPrependOptions();
        for (const QString &arg : extraArgsToPrepend)
            optionsBuilder.prepend(arg);
        const QStringList extraArgsToAppend = extraClangToolsAppendOptions();
        for (const QString &arg : extraArgsToAppend)
            optionsBuilder.add(arg);
        return optionsBuilder;
    };

    generatorWatcher.setFuture(
        Utils::asyncRun(
            &generateCompilationDB,
            QList<ProjectInfo::ConstPtr>{CppModelManager::projectInfo(bc->project())},
            dir.path(),
            CompilationDbPurpose::Analysis,
            ClangDiagnosticConfigsModel::globalDiagnosticOptions(),
            getCompilerOptionsBuilder));
    generatorSynchronizer.addFuture(generatorWatcher.future());
}

} // namespace ClangTools::Internal
