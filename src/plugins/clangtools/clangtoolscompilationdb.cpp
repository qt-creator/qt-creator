// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolscompilationdb.h"

#include "clangtoolssettings.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compilationdb.h>
#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/temporarydirectory.h>

#include <QHash>
#include <QRegularExpression>

#include <utility>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace QtTaskTree;
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
    QStringList generationInputs() const;
    QString toolName() const { return clangToolName(toolType); }

    ClangToolsCompilationDb * const q;
    const ClangToolType toolType;
    TemporaryDirectory dir{toolName() + "XXXXXX"};
    bool readyAndUpToDate = false;
    int invalidations = 0;
    QStringList generatedWithInputs;
    FutureSynchronizer generatorSynchronizer;
    QSingleTaskTreeRunner taskTreeRunner;
};

ClangToolsCompilationDb::ClangToolsCompilationDb(ClangToolType toolType, BuildConfiguration *bc)
    : QObject(bc), d(new Private(toolType, this))
{
    connect(bc, &BuildConfiguration::destroyed, [bc, toolType] {
        dbs.remove(std::make_pair(toolType, bc));
    });
    connect(clangToolsProjectSettings(bc->project()).get(),
            &ClangToolsProjectSettings::changed,
            this, &ClangToolsCompilationDb::invalidate);
    connect(ClangToolsSettings::instance(), &ClangToolsSettings::changed, this, [this] {
        if (d->generationInputs() != d->generatedWithInputs)
            invalidate();
    });
    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated, this,
        [this, bc](Project *p) {
            if (p == bc->project())
                invalidate();
        });
}

ClangToolsCompilationDb::~ClangToolsCompilationDb() { delete d; }

void ClangToolsCompilationDb::invalidate()
{
    ++d->invalidations;
    d->readyAndUpToDate = false;
}

FilePath ClangToolsCompilationDb::parentDir() const { return d->dir.path(); }

bool ClangToolsCompilationDb::generateIfNecessary()
{
    if (d->readyAndUpToDate)
        return false;
    if (!d->taskTreeRunner.isRunning())
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

static ClangDiagnosticConfig diagnosticConfigFor(const RunSettings &runSettings)
{
    const Id configId = runSettings.safeDiagnosticConfigId();
    return Utils::findOrDefault(
        ClangToolsSettings::instance()->diagnosticConfigId.customConfigs(),
        [configId](const ClangDiagnosticConfig &c) { return c.id() == configId; });
}

QStringList ClangToolsCompilationDb::Private::generationInputs() const
{
    const auto bc = static_cast<BuildConfiguration *>(q->parent());
    const RunSettings &runSettings = runSettingsFor(bc->project());
    const ClangDiagnosticConfig config = diagnosticConfigFor(runSettings);
    return runSettings.ignoredCompilerFlagPatterns()
           + QStringList{config.useBuildSystemWarnings() ? "1" : "0"} + config.clangOptions();
}

void ClangToolsCompilationDb::Private::generate()
{
    QTC_CHECK(!readyAndUpToDate);

    Core::MessageManager::writeSilently(
        Tr::tr("Generating compilation database for %1 at \"%2\" ...")
            .arg(clangToolName(toolType), dir.path().toUserOutput()));

    const auto bc = static_cast<BuildConfiguration *>(q->parent());
    generatedWithInputs = generationInputs();
    const int generation = invalidations;
    const RunSettings &runSettings = runSettingsFor(bc->project());
    const ClangDiagnosticConfig config = diagnosticConfigFor(runSettings);
    const QList<QRegularExpression> ignoredFlags
        = CompilerOptionsBuilder::wildcardsToRegularExpressions(
            runSettings.ignoredCompilerFlagPatterns());
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
        optionsBuilder.setIgnoredCompilerFlags(ignoredFlags);
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

    const auto onSetup = [this, bc, getCompilerOptionsBuilder](
                             Async<GenerateCompilationDbResult> &task) {
        task.setFutureSynchronizer(&generatorSynchronizer);
        task.setConcurrentCallData(
            &generateCompilationDB,
            QList<ProjectInfo::ConstPtr>{CppModelManager::projectInfo(bc->project())},
            dir.path(),
            CompilationDbPurpose::Analysis,
            ClangDiagnosticConfigsModel::globalDiagnosticOptions(),
            getCompilerOptionsBuilder);
    };
    const auto onDone = [this, generation](const Async<GenerateCompilationDbResult> &task) {
        const auto result = task.result();
        const bool success = result.has_value();
        QTC_CHECK(!readyAndUpToDate);
        readyAndUpToDate = success && generation == invalidations;
        if (success) {
            Core::MessageManager::writeSilently(
                Tr::tr("Compilation database for %1 successfully generated at \"%2\".")
                    .arg(toolName(), dir.path().toUserOutput()));
        } else {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Generating compilation database for %1 failed: %2")
                    .arg(toolName(), result.error()));
        }
    };
    taskTreeRunner.start({AsyncTask<GenerateCompilationDbResult>(onSetup, onDone)}, {},
                         [this, generation] {
        if (generation != invalidations) {
            // The inputs changed while generating: this result is outdated.
            generate();
            return;
        }
        emit q->generated(readyAndUpToDate);
    });
}

} // namespace ClangTools::Internal
