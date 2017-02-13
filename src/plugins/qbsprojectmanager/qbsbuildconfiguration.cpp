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

#include "qbsbuildconfiguration.h"

#include "qbsbuildconfigurationwidget.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagersettings.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>

#include <QCoreApplication>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

const char QBS_BC_ID[] = "Qbs.QbsBuildConfiguration";

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(Target *target) :
    BuildConfiguration(target, Core::Id(QBS_BC_ID)),
    m_isParsing(true),
    m_parsingError(false)
{
    connect(project(), &QbsProject::projectParsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(project(), &QbsProject::projectParsingDone, this, &BuildConfiguration::enabledChanged);

    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    connect(bsl, &BuildStepList::stepInserted, this, &QbsBuildConfiguration::buildStepInserted);
}

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, Core::Id id) :
    BuildConfiguration(target, id)
{ }

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, QbsBuildConfiguration *source) :
    BuildConfiguration(target, source)
{
    cloneSteps(source);
}

bool QbsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    // Fix up the existing build steps:
    for (int i = 0; i < bsl->count(); ++i) {
        QbsBuildStep *bs = qobject_cast<QbsBuildStep *>(bsl->at(i));
        if (bs)
            connect(bs, &QbsBuildStep::qbsConfigurationChanged, this, &QbsBuildConfiguration::qbsConfigurationChanged);
    }

    return true;
}

void QbsBuildConfiguration::buildStepInserted(int pos)
{
    QbsBuildStep *step = qobject_cast<QbsBuildStep *>(stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->at(pos));
    if (step) {
        connect(step, &QbsBuildStep::qbsConfigurationChanged, this, &QbsBuildConfiguration::qbsConfigurationChanged);
        emit qbsConfigurationChanged();
    }
}

NamedWidget *QbsBuildConfiguration::createConfigWidget()
{
    return new QbsBuildConfigurationWidget(this);
}

QbsBuildStep *QbsBuildConfiguration::qbsStep() const
{
    return stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->firstOfType<QbsBuildStep>();
}

QVariantMap QbsBuildConfiguration::qbsConfiguration() const
{
    QVariantMap config;
    QbsBuildStep *qbsBs = qbsStep();
    if (qbsBs)
        config = qbsBs->qbsConfiguration(QbsBuildStep::ExpandVariables);
    return config;
}

Internal::QbsProject *QbsBuildConfiguration::project() const
{
    return qobject_cast<Internal::QbsProject *>(target()->project());
}

IOutputParser *QbsBuildConfiguration::createOutputParser() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return tc ? tc->outputParser() : 0;
}

bool QbsBuildConfiguration::isEnabled() const
{
    return !project()->isParsing() && project()->hasParseResult();
}

QString QbsBuildConfiguration::disabledReason() const
{
    if (project()->isParsing())
        return tr("Parsing the Qbs project.");
    if (!project()->hasParseResult())
        return tr("Parsing of Qbs project has failed.");
    return QString();
}

BuildConfiguration::BuildType QbsBuildConfiguration::buildType() const
{
    QString variant;
    if (qbsStep())
        variant = qbsStep()->buildVariant();

    if (variant == QLatin1String(Constants::QBS_VARIANT_DEBUG))
        return Debug;
    if (variant == QLatin1String(Constants::QBS_VARIANT_RELEASE))
        return Release;
    return Unknown;
}

void QbsBuildConfiguration::setChangedFiles(const QStringList &files)
{
    m_changedFiles = files;
}

QStringList QbsBuildConfiguration::changedFiles() const
{
    return m_changedFiles;
}

void QbsBuildConfiguration::setActiveFileTags(const QStringList &fileTags)
{
    m_activeFileTags = fileTags;
}

QStringList QbsBuildConfiguration::activeFileTags() const
{
    return m_activeFileTags;
}

void QbsBuildConfiguration::setProducts(const QStringList &products)
{
    m_products = products;
}

QStringList QbsBuildConfiguration::products() const
{
    return m_products;
}

void QbsBuildConfiguration::emitBuildTypeChanged()
{
    emit buildTypeChanged();
}

QString QbsBuildConfiguration::configurationName() const
{
    const QString profileName = QbsManager::instance()->profileForKit(target()->kit());
    const QString buildVariant = qbsConfiguration()
            .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
    return profileName + QLatin1Char('-') + buildVariant;
}

class StepProxy
{
public:
    StepProxy(const BuildStep *buildStep)
        : m_qbsBuildStep(qobject_cast<const QbsBuildStep *>(buildStep))
        , m_qbsCleanStep(qobject_cast<const QbsCleanStep *>(buildStep))
    {
    }

    QString command() const {
        if (m_qbsBuildStep)
            return QLatin1String("build");
        return QLatin1String("clean");
    }

    bool dryRun() const {
        if (m_qbsBuildStep)
            return false;
        return m_qbsCleanStep->dryRun();
    }

    bool keepGoing() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->keepGoing();
        return m_qbsCleanStep->keepGoing();
    }

    bool showCommandLines() const {
        return m_qbsBuildStep ? m_qbsBuildStep->showCommandLines() : false;
    }

    bool noInstall() const {
        return m_qbsBuildStep ? !m_qbsBuildStep->install() : false;
    }

    bool cleanInstallRoot() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->cleanInstallRoot();
        return false;
    }

    int jobCount() const {
        return m_qbsBuildStep ? m_qbsBuildStep->maxJobs() : 0;
    }

    Utils::FileName installRoot() const {
        if (m_qbsBuildStep && m_qbsBuildStep->hasCustomInstallRoot())
            return m_qbsBuildStep->installRoot();
        return Utils::FileName();
    }

private:
    const QbsBuildStep * const m_qbsBuildStep;
    const QbsCleanStep * const m_qbsCleanStep;
};

QString QbsBuildConfiguration::equivalentCommandLine(const BuildStep *buildStep) const
{
    QString commandLine;
    const QString qbsInstallDir = QString::fromLocal8Bit(qgetenv("QBS_INSTALL_DIR"));
    const QString qbsFilePath = Utils::HostOsInfo::withExecutableSuffix(!qbsInstallDir.isEmpty()
            ? qbsInstallDir + QLatin1String("/bin/qbs")
            : QCoreApplication::applicationDirPath() + QLatin1String("/qbs"));
    Utils::QtcProcess::addArg(&commandLine, QDir::toNativeSeparators(qbsFilePath));
    const StepProxy stepProxy(buildStep);
    Utils::QtcProcess::addArg(&commandLine, stepProxy.command());
    const QString buildDir = buildDirectory().toUserOutput();
    Utils::QtcProcess::addArgs(&commandLine, QStringList({ "-d", buildDir }));
    Utils::QtcProcess::addArgs(&commandLine, QStringList("-f")
                               << buildStep->project()->projectFilePath().toUserOutput());
    if (QbsProjectManagerSettings::useCreatorSettingsDirForQbs()) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList({ "--settings-dir",
                QDir::toNativeSeparators(QbsProjectManagerSettings::qbsSettingsBaseDir()) }));
    }
    if (stepProxy.dryRun())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--dry-run"));
    if (stepProxy.keepGoing())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--keep-going"));
    if (stepProxy.showCommandLines())
        Utils::QtcProcess::addArgs(&commandLine, QStringList({ "--command-echo-mode",
                                                               "command-line" }));
    if (stepProxy.noInstall())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--no-install"));
    if (stepProxy.cleanInstallRoot())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--clean-install-root"));
    const int jobCount = stepProxy.jobCount();
    if (jobCount > 0) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList({ "--jobs",
                                                               QString::number(jobCount) }));
    }
    const QString profileName = QbsManager::instance()->profileForKit(buildStep->target()->kit());
    const QString buildVariant = qbsConfiguration()
            .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
    Utils::QtcProcess::addArg(&commandLine, configurationName());
    Utils::QtcProcess::addArg(&commandLine, QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)
                                  + QLatin1Char(':') + buildVariant);
    const Utils::FileName installRoot = stepProxy.installRoot();
    if (!installRoot.isEmpty()) {
        Utils::QtcProcess::addArg(&commandLine, QLatin1String(Constants::QBS_INSTALL_ROOT_KEY)
                                  + QLatin1Char(':') + installRoot.toUserOutput());
    }
    Utils::QtcProcess::addArg(&commandLine, QLatin1String("profile:") + profileName);

    return commandLine;
}

QbsBuildConfiguration *QbsBuildConfiguration::setup(Target *t,
                                                    const QString &defaultDisplayName,
                                                    const QString &displayName,
                                                    const QVariantMap &buildData,
                                                    const Utils::FileName &directory)
{
    // Add the build configuration.
    QbsBuildConfiguration *bc = new QbsBuildConfiguration(t);
    bc->setDefaultDisplayName(defaultDisplayName);
    bc->setDisplayName(displayName);
    bc->setBuildDirectory(directory);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    QbsBuildStep *bs = new QbsBuildStep(buildSteps);
    bs->setQbsConfiguration(buildData);
    buildSteps->insertStep(0, bs);

    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    QbsCleanStep *cs = new QbsCleanStep(cleanSteps);
    cleanSteps->insertStep(0, cs);

    return bc;
}

// ---------------------------------------------------------------------------
// QbsBuildConfigurationFactory:
// ---------------------------------------------------------------------------

QbsBuildConfigurationFactory::QbsBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{ }

bool QbsBuildConfigurationFactory::canHandle(const Target *t) const
{
    return qobject_cast<Internal::QbsProject *>(t->project());
}

BuildInfo *QbsBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                         BuildConfiguration::BuildType type) const
{
    auto info = new ProjectExplorer::BuildInfo(this);
    info->typeName = tr("Build");
    info->kitId = k->id();
    info->buildType = type;
    return info;
}

int QbsBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> QbsBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;

    BuildInfo *info = createBuildInfo(parent->kit(), BuildConfiguration::Debug);
    result << info;

    return result;
}

int QbsBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::MIME_TYPE)))
        return 0;
    return -1;
}

static Utils::FileName defaultBuildDirectory(const QString &projectFilePath, const Kit *k,
                                             const QString &bcName,
                                             BuildConfiguration::BuildType buildType)
{
    const QString projectName = QFileInfo(projectFilePath).completeBaseName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, bcName, buildType);
    QString projectDir = Project::projectDirectory(Utils::FileName::fromString(projectFilePath)).toString();
    QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return Utils::FileName::fromString(Utils::FileUtils::resolvePath(projectDir, buildPath));
}

QList<BuildInfo *> QbsBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;

    BuildInfo *info = createBuildInfo(k, BuildConfiguration::Debug);
    //: The name of the debug build configuration created by default for a qbs project.
    info->displayName = tr("Debug");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Debug", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    info = createBuildInfo(k, BuildConfiguration::Release);
    //: The name of the release build configuration created by default for a qbs project.
    info->displayName = tr("Release");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Release", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    return result;
}

BuildConfiguration *QbsBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    QVariantMap configData;
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      (info->buildType == BuildConfiguration::Debug)
                          ? QLatin1String(Constants::QBS_VARIANT_DEBUG)
                          : QLatin1String(Constants::QBS_VARIANT_RELEASE));

    Utils::FileName buildDir = info->buildDirectory;
    if (buildDir.isEmpty())
        buildDir = defaultBuildDirectory(parent->project()->projectDirectory().toString(),
                                         parent->kit(), info->displayName, info->buildType);

    BuildConfiguration *bc
            = QbsBuildConfiguration::setup(parent, info->displayName, info->displayName,
                                           configData, buildDir);

    return bc;
}

bool QbsBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    return canHandle(parent) && qobject_cast<QbsBuildConfiguration *>(source);
}

BuildConfiguration *QbsBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QbsBuildConfiguration *oldbc(static_cast<QbsBuildConfiguration *>(source));
    return new QbsBuildConfiguration(parent, oldbc);
}

bool QbsBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Core::Id(QBS_BC_ID);
}

BuildConfiguration *QbsBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QbsBuildConfiguration *bc = new QbsBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

} // namespace Internal
} // namespace QbsProjectManager
