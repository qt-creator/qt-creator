// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcompilercleanstep.h"
#include "nimbuildconfiguration.h"

#include "../nimconstants.h"
#include "../nimtr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <solutions/tasking/tasktree.h>

#include <utils/aspects.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDateTime>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Nim {

class NimCompilerCleanStep final : public BuildStep
{
public:
    NimCompilerCleanStep(BuildStepList *parentList, Id id)
        : BuildStep(parentList, id)
    {
        workingDir.setLabelText(Tr::tr("Working directory:"));

        setSummaryUpdater([this] {
            workingDir.setValue(buildDirectory());
            return displayName();
        });
    }

private:
    bool init() final;
    GroupItem runRecipe() final;

    bool removeCacheDirectory();
    bool removeOutFilePath();

    FilePath m_buildDir;
    FilePathAspect workingDir{this};
};

bool NimCompilerCleanStep::init()
{
    const FilePath buildDir = buildDirectory();
    const bool exists = buildDir.exists();
    if (exists)
        m_buildDir = buildDir;
    return exists;
}

GroupItem NimCompilerCleanStep::runRecipe()
{
    const auto onSetup = [this] {
        if (!m_buildDir.exists()) {
            emit addOutput(Tr::tr("Build directory \"%1\" does not exist.")
                               .arg(m_buildDir.toUserOutput()), OutputFormat::ErrorMessage);
            return false;
        }
        if (!removeCacheDirectory()) {
            emit addOutput(Tr::tr("Failed to delete the cache directory."),
                           OutputFormat::ErrorMessage);
            return false;
        }
        if (!removeOutFilePath()) {
            emit addOutput(Tr::tr("Failed to delete the out file."), OutputFormat::ErrorMessage);
            return false;
        }
        emit addOutput(Tr::tr("Clean step completed successfully."), OutputFormat::NormalMessage);
        return true;
    };
    return Sync(onSetup);
}

bool NimCompilerCleanStep::removeCacheDirectory()
{
    auto bc = qobject_cast<NimBuildConfiguration*>(buildConfiguration());
    QTC_ASSERT(bc, return false);
    if (!bc->cacheDirectory().exists())
        return true;
    QDir dir = QDir::fromNativeSeparators(bc->cacheDirectory().toString());
    const QString dirName = dir.dirName();
    if (!dir.cdUp())
        return false;
    const QString newName = QStringLiteral("%1.bkp.%2").arg(dirName, QString::number(QDateTime::currentMSecsSinceEpoch()));
    return dir.rename(dirName, newName);
}

bool NimCompilerCleanStep::removeOutFilePath()
{
    auto bc = qobject_cast<NimBuildConfiguration*>(buildConfiguration());
    QTC_ASSERT(bc, return false);
    if (!bc->outFilePath().exists())
        return true;
    return QFile(bc->outFilePath().toFileInfo().absoluteFilePath()).remove();
}

// NimCompilerCleanStepFactory

NimCompilerCleanStepFactory::NimCompilerCleanStepFactory()
{
    registerStep<NimCompilerCleanStep>(Constants::C_NIMCOMPILERCLEANSTEP_ID);
    setFlags(BuildStep::Unclonable);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    setSupportedConfiguration(Constants::C_NIMBUILDCONFIGURATION_ID);
    setRepeatable(false);
    setDisplayName(Tr::tr("Nim Clean Step"));
}

} // Nim
