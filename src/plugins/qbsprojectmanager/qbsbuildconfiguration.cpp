/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsbuildconfiguration.h"

#include "qbsbuildconfigurationwidget.h"
#include "qbsbuildinfo.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <utils/qtcassert.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

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
    foreach (BuildStep *bs, stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->steps()) {
        if (QbsBuildStep *qbsBs = qobject_cast<QbsBuildStep *>(bs))
            return qbsBs;
    }
    return 0;
}

QVariantMap QbsBuildConfiguration::qbsConfiguration() const
{
    QVariantMap config;
    QbsBuildStep *qbsBs = qbsStep();
    if (qbsBs)
        config = qbsBs->qbsConfiguration();
    return config;
}

Internal::QbsProject *QbsBuildConfiguration::project() const
{
    return qobject_cast<Internal::QbsProject *>(target()->project());
}

IOutputParser *QbsBuildConfiguration::createOutputParser() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
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

QbsBuildConfigurationFactory::~QbsBuildConfigurationFactory()
{ }

bool QbsBuildConfigurationFactory::canHandle(const Target *t) const
{
    return qobject_cast<Internal::QbsProject *>(t->project());
}

BuildInfo *QbsBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                         BuildConfiguration::BuildType type) const
{
    QbsBuildInfo *info = new QbsBuildInfo(this);
    info->typeName = tr("Build");
    info->kitId = k->id();
    info->type = type;
    info->supportsShadowBuild = true;
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
    return (k && Core::MimeDatabase::findByFile(QFileInfo(projectPath))
            .matchesType(QLatin1String(Constants::MIME_TYPE))) ? 0 : -1;
}

static Utils::FileName defaultBuildDirectory(const QString &projectFilePath, const Kit *k,
                                             const QString &bcName)
{
    const QString projectName = QFileInfo(projectFilePath).completeBaseName();
    ProjectMacroExpander expander(projectName, k, bcName);
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
    info->buildDirectory = defaultBuildDirectory(projectPath, k, info->displayName);
    result << info;

    info = createBuildInfo(k, BuildConfiguration::Release);
    //: The name of the release build configuration created by default for a qbs project.
    info->displayName = tr("Release");
    info->buildDirectory = defaultBuildDirectory(projectPath, k, info->displayName);
    result << info;

    return result;
}

BuildConfiguration *QbsBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    const QbsBuildInfo *qbsInfo = static_cast<const QbsBuildInfo *>(info);

    QVariantMap configData;
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      (qbsInfo->type == BuildConfiguration::Release)
                      ? QLatin1String(Constants::QBS_VARIANT_RELEASE)
                      : QLatin1String(Constants::QBS_VARIANT_DEBUG));

    Utils::FileName buildDir = info->buildDirectory;
    if (buildDir.isEmpty())
        buildDir = defaultBuildDirectory(parent->project()->projectDirectory().toString(),
                                         parent->kit(), info->displayName);

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
