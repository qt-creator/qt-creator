/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include <utils/qtcassert.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <QInputDialog>

static const char QBS_BC_ID[] = "Qbs.QbsBuildConfiguration";
static const char QBS_BUILD_DIRECTORY_KEY[] = "Qbs.BuildDirectory";

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

    connect(this, SIGNAL(buildDirectoryChanged()), target, SLOT(onBuildDirectoryChanged()));
    ProjectExplorer::BuildStepList *bsl
            = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    connect(bsl, SIGNAL(stepInserted(int)), this, SLOT(buildStepInserted(int)));
}

QbsBuildConfiguration::QbsBuildConfiguration(ProjectExplorer::Target *target, const Core::Id id) :
    BuildConfiguration(target, id)
{ }

QbsBuildConfiguration::QbsBuildConfiguration(ProjectExplorer::Target *target, QbsBuildConfiguration *source) :
    BuildConfiguration(target, source),
    m_buildDirectory(source->m_buildDirectory)
{
    cloneSteps(source);
}

QVariantMap QbsBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(QBS_BUILD_DIRECTORY_KEY), m_buildDirectory.toUserOutput());
    return map;
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

    m_buildDirectory = Utils::FileName::fromUserInput(map.value(QLatin1String(QBS_BUILD_DIRECTORY_KEY)).toString());

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

QString QbsBuildConfiguration::buildDirectory() const
{
    QString path = QDir::cleanPath(environment().expandVariables(m_buildDirectory.toString()));
    return QDir::cleanPath(QDir(target()->project()->projectDirectory()).absoluteFilePath(path));
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

void QbsBuildConfiguration::setBuildDirectory(const Utils::FileName &dir)
{
    if (m_buildDirectory == dir)
        return;
    m_buildDirectory = dir;
    emit buildDirectoryChanged();
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

QList<Core::Id> QbsBuildConfigurationFactory::availableCreationIds(const ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(QBS_BC_ID);
}

QString QbsBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == QBS_BC_ID)
        return tr("Qbs based build");
    return QString();
}

bool QbsBuildConfigurationFactory::canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == QBS_BC_ID;
}

ProjectExplorer::BuildConfiguration *QbsBuildConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                          const Core::Id id,
                                                                          const QString &name)
{
    if (!canCreate(parent, id))
        return 0;

    Internal::QbsProject *project = static_cast<Internal::QbsProject *>(parent->project());

    bool ok = true;
    QString buildConfigurationName = name;
    if (buildConfigurationName.isNull())
        buildConfigurationName = QInputDialog::getText(0,
                                                       tr("New Configuration"),
                                                       tr("New configuration name:"),
                                                       QLineEdit::Normal,
                                                       QString(), &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    //: Debug build configuration. We recommend not translating it.
    QString firstName = tr("%1 Debug").arg(buildConfigurationName).trimmed();

    //: Release build configuration. We recommend not translating it.
    QString secondName = tr("%1 Release").arg(buildConfigurationName).trimmed();

    QVariantMap configData;
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      QLatin1String(Constants::QBS_VARIANT_DEBUG));

    ProjectExplorer::BuildConfiguration *bc
            = QbsBuildConfiguration::setup(parent, firstName, firstName,
                                           configData, project->defaultBuildDirectory());
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      QLatin1String(Constants::QBS_VARIANT_RELEASE));
    parent->addBuildConfiguration(
                QbsBuildConfiguration::setup(parent, secondName, secondName,
                                             configData, project->defaultBuildDirectory()));
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
