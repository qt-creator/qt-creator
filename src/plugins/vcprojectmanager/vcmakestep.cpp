/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "msbuildoutputparser.h"
#include "vcmakestep.h"
#include "vcprojectbuildconfiguration.h"
#include "vcprojectfile.h"
#include "vcprojectkitinformation.h"
#include "vcprojectmanagerconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QFormLayout>
#include <QLabel>

namespace VcProjectManager {
namespace Internal {

namespace {
const char MS_ID[] = "VcProjectManager.MakeStep";
const char CLEAN_KEY[] = "VcProjectManager.MakeStep.Clean";
const char BUILD_TARGETS_KEY[] = "VcProjectManager.MakeStep.BuildTargets";
const char ADDITIONAL_ARGUMENTS_KEY[] = "VcProjectManager.MakeStep.AdditionalArguments";
}

VcMakeStep::VcMakeStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractProcessStep(bsl, Core::Id(MS_ID))
{
}

VcMakeStep::~VcMakeStep()
{
}

bool VcMakeStep::init()
{
    VcProjectBuildConfiguration *bc = vcProjectBuildConfiguration();
    MsBuildInformation *msBuild = VcProjectKitInformation::msBuildInfo(target()->kit());

    if (!bc || !msBuild || msBuild->m_executable.isEmpty()) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Kit doesn't contain any proper MS Build tool for .vcproj files."),
                                             Utils::FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    m_processParams = processParameters();

    if (msBuild)
        m_processParams->setCommand(msBuild->m_executable);

    m_processParams->setMacroExpander(bc->macroExpander());
    m_processParams->setEnvironment(bc->environment());
    m_processParams->setWorkingDirectory(bc->buildDirectory());

    if (!m_buildArguments.isEmpty())
        m_processParams->setArguments(m_buildArguments.join(QLatin1String(" ")));

    setOutputParser(new MsBuildOutputParser);
    return AbstractProcessStep::init();
}

void VcMakeStep::run(QFutureInterface<bool> &fi)
{
    bool canContinue = true;

    foreach (const ProjectExplorer::Task &t, m_tasks) {
        addTask(t);
        canContinue = false;
    }

    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details."), BuildStep::MessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget *VcMakeStep::createConfigWidget()
{
    return new VcMakeStepConfigWidget(this);
}

bool VcMakeStep::immutable() const
{
    return false;
}

VcProjectBuildConfiguration *VcMakeStep::vcProjectBuildConfiguration() const
{
    return static_cast<VcProjectBuildConfiguration *>(buildConfiguration());
}

QStringList VcMakeStep::buildArguments() const
{
    return m_buildArguments;
}

QString VcMakeStep::buildArgumentsToString() const
{
    return m_buildArguments.join(QLatin1String(" "));
}

void VcMakeStep::addBuildArgument(const QString &argument)
{
    if (m_buildArguments.contains(argument))
        return;
    m_buildArguments.append(argument);
}

void VcMakeStep::removeBuildArgument(const QString &buildArgument)
{
    m_buildArguments.removeAll(buildArgument);
}

QVariantMap VcMakeStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(QLatin1String(Constants::VC_PROJECT_MS_BUILD_ARGUMENT_LIST), m_buildArguments);
    return map;
}

bool VcMakeStep::fromMap(const QVariantMap &map)
{
    m_buildArguments = map.value(QLatin1String(Constants::VC_PROJECT_MS_BUILD_ARGUMENT_LIST)).toStringList();
    return BuildStep::fromMap(map);
}

void VcMakeStep::stdOutput(const QString &line)
{
    AbstractProcessStep::stdOutput(line);
}

VcMakeStep::VcMakeStep(ProjectExplorer::BuildStepList *parent, VcMakeStep *vcMakeStep) :
    AbstractProcessStep(parent, vcMakeStep)
{
}

/////////////////////////
// VcMakeStepConfigWidget
/////////////////////////
VcMakeStepConfigWidget::VcMakeStepConfigWidget(VcMakeStep *makeStep) :
    m_makeStep(makeStep)
{
    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_msBuildPath = new QLabel(this);
    mainLayout->addRow(tr("Command:"), m_msBuildPath);
    setLayout(mainLayout);

    MsBuildInformation *msBuild = VcProjectKitInformation::msBuildInfo(m_makeStep->target()->kit());

    if (m_makeStep)
        m_msBuildPath->setText(msBuild->m_executable);

    connect(m_makeStep->target(), SIGNAL(kitChanged()), this, SLOT(msBuildUpdated()));
}

QString VcMakeStepConfigWidget::displayName() const
{
    return tr("Vc Make Step Config Widget");
}

QString VcMakeStepConfigWidget::summaryText() const
{
    MsBuildInformation *msBuild = VcProjectKitInformation::msBuildInfo(m_makeStep->target()->kit());

    QFileInfo fileInfo(msBuild->m_executable);
    return QString(QLatin1String("<b>MsBuild:</b> %1 %2")).arg(fileInfo.fileName())
            .arg(m_makeStep->buildArgumentsToString());
}

void VcMakeStepConfigWidget::msBuildUpdated()
{
    VcProjectBuildConfiguration *bc = static_cast<VcProjectBuildConfiguration *>(m_makeStep->buildConfiguration());

    if (bc && bc->target() && bc->target()->kit()) {
        MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
        MsBuildInformation *info = msBVM->msBuildInformation(Core::Id::fromSetting(bc->target()->kit()->value(Core::Id(Constants::VC_PROJECT_KIT_INFO_ID))));

        if (info)
            m_msBuildPath->setText(info->m_executable);

        else
            m_msBuildPath->setText(tr("<MS Build not available>"));
    }
}

////////////////////
// VcMakeStepFactory
////////////////////
VcMakeStepFactory::VcMakeStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

VcMakeStepFactory::~VcMakeStepFactory()
{
}

bool VcMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::VC_PROJECT_ID ||
            parent->target()->project()->id() == Constants::VC_PROJECT_2005_ID)
        return id == MS_ID;
    return false;
}

ProjectExplorer::BuildStep* VcMakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    VcMakeStep *step = new VcMakeStep(parent);
    return step;
}

bool VcMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *VcMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new VcMakeStep(parent, static_cast<VcMakeStep *>(product));
}

bool VcMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *VcMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    VcMakeStep *bs = new VcMakeStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> VcMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target() && parent->target()->project() &&
            (parent->target()->project()->id() == Constants::VC_PROJECT_ID || parent->target()->project()->id() == Constants::VC_PROJECT_2005_ID))
            return QList<Core::Id>() << Core::Id(MS_ID);

    return QList<Core::Id>();
}

QString VcMakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MS_ID)
        return tr("Make", "Vc Project Make Step Factory id.");
    return QString();
}


} // namespace Internal
} // namespace VcProjectManager
