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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <utils/qtcassert.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <QInputDialog>

static const char QBS_BC_ID[] = "Qbs.QbsBuildConfiguration";

namespace QbsProjectManager {
namespace Internal {

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(ProjectExplorer::Target *target) :
    BuildConfiguration(target, Core::Id(QBS_BC_ID)),
    m_isParsing(true),
    m_parsingError(false)
{
    connect(project(), SIGNAL(projectParsingStarted()), this, SIGNAL(enabledChanged()));
    connect(project(), SIGNAL(projectParsingDone(bool)), this, SIGNAL(enabledChanged()));

    ProjectExplorer::BuildStepList *bsl
            = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    connect(bsl, SIGNAL(stepInserted(int)), this, SLOT(buildStepInserted(int)));
}

QbsBuildConfiguration::QbsBuildConfiguration(ProjectExplorer::Target *target, const Core::Id id) :
    BuildConfiguration(target, id)
{ }

QbsBuildConfiguration::QbsBuildConfiguration(ProjectExplorer::Target *target, QbsBuildConfiguration *source) :
    BuildConfiguration(target, source)
{
    cloneSteps(source);
}

bool QbsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    ProjectExplorer::BuildStepList *bsl
            = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    // Fix up the existing build steps:
    for (int i = 0; i < bsl->count(); ++i) {
        QbsBuildStep *bs = qobject_cast<QbsBuildStep *>(bsl->at(i));
        if (bs)
            connect(bs, SIGNAL(qbsConfigurationChanged()), this, SIGNAL(qbsConfigurationChanged()));
    }

    return true;
}

void QbsBuildConfiguration::buildStepInserted(int pos)
{
    QbsBuildStep *step = qobject_cast<QbsBuildStep *>(stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->at(pos));
    if (step) {
        connect(step, SIGNAL(qbsConfigurationChanged()), this, SIGNAL(qbsConfigurationChanged()));
        emit qbsConfigurationChanged();
    }
}

ProjectExplorer::NamedWidget *QbsBuildConfiguration::createConfigWidget()
{
    return new QbsBuildConfigurationWidget(this);
}

QbsBuildStep *QbsBuildConfiguration::qbsStep() const
{
    foreach (ProjectExplorer::BuildStep *bs,
             stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD))->steps()) {
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

ProjectExplorer::IOutputParser *QbsBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
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

ProjectExplorer::BuildConfiguration::BuildType QbsBuildConfiguration::buildType() const
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

QbsBuildConfiguration *QbsBuildConfiguration::setup(ProjectExplorer::Target *t,
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

    ProjectExplorer::BuildStepList *buildSteps
            = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    QbsBuildStep *bs = new QbsBuildStep(buildSteps);
    bs->setQbsConfiguration(buildData);
    buildSteps->insertStep(0, bs);

    ProjectExplorer::BuildStepList *cleanSteps
            = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
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

bool QbsBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    return qobject_cast<Internal::QbsProject *>(t->project());
}

ProjectExplorer::BuildInfo *QbsBuildConfigurationFactory::createBuildInfo(const ProjectExplorer::Kit *k,
                                                                          ProjectExplorer::BuildConfiguration::BuildType type) const
{
    QbsBuildInfo *info = new QbsBuildInfo(this);
    info->typeName = tr("Build");
    info->kitId = k->id();
    info->type = type;
    info->supportsShadowBuild = true;
    return info;
}

int QbsBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo *> QbsBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    QList<ProjectExplorer::BuildInfo *> result;

    ProjectExplorer::BuildInfo *info = createBuildInfo(parent->kit(),
                                                       ProjectExplorer::BuildConfiguration::Debug);
    result << info;

    return result;
}

int QbsBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    return (k && Core::MimeDatabase::findByFile(QFileInfo(projectPath))
            .matchesType(QLatin1String(Constants::MIME_TYPE))) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo *> QbsBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    QList<ProjectExplorer::BuildInfo *> result;

    ProjectExplorer::BuildInfo *info = createBuildInfo(k, ProjectExplorer::BuildConfiguration::Debug);
    //: The name of the debug build configuration created by default for a qbs project.
    info->displayName = tr("Debug");
    info->buildDirectory = QbsProject::defaultBuildDirectory(projectPath, k, info->displayName);
    result << info;

    info = createBuildInfo(k, ProjectExplorer::BuildConfiguration::Release);
    //: The name of the release build configuration created by default for a qbs project.
    info->displayName = tr("Release");
    info->buildDirectory = QbsProject::defaultBuildDirectory(projectPath, k, info->displayName);
    result << info;

    return result;
}

ProjectExplorer::BuildConfiguration *QbsBuildConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                          const ProjectExplorer::BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    const QbsBuildInfo *qbsInfo = static_cast<const QbsBuildInfo *>(info);

    QVariantMap configData;
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      (qbsInfo->type == ProjectExplorer::BuildConfiguration::Release)
                      ? QLatin1String(Constants::QBS_VARIANT_RELEASE)
                      : QLatin1String(Constants::QBS_VARIANT_DEBUG));

    Utils::FileName buildDir = info->buildDirectory;
    if (buildDir.isEmpty())
        buildDir = QbsProject::defaultBuildDirectory(parent->project()->projectDirectory().toString(),
                                                     parent->kit(), info->displayName);

    ProjectExplorer::BuildConfiguration *bc
            = QbsBuildConfiguration::setup(parent, info->displayName, info->displayName,
                                           configData, buildDir);

    return bc;
}

bool QbsBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent,
                                            ProjectExplorer::BuildConfiguration *source) const
{
    return canHandle(parent) && qobject_cast<QbsBuildConfiguration *>(source);
}

ProjectExplorer::BuildConfiguration *QbsBuildConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                         ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QbsBuildConfiguration *oldbc(static_cast<QbsBuildConfiguration *>(source));
    return new QbsBuildConfiguration(parent, oldbc);
}

bool QbsBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent,
                                              const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Core::Id(QBS_BC_ID);
}

ProjectExplorer::BuildConfiguration *QbsBuildConfigurationFactory::restore(ProjectExplorer::Target *parent,
                                                                           const QVariantMap &map)
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
