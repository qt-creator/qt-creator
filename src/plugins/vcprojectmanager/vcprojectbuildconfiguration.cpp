#include "vcprojectbuildconfiguration.h"
#include "vcprojectmanagerconstants.h"
#include "vcmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <projectexplorer/target.h>

#include <QFormLayout>
#include <QLabel>
#include <QInputDialog>

////////////////////////////////////
// VcProjectBuildConfiguration class
////////////////////////////////////
namespace VcProjectManager {
namespace Internal {

VcProjectBuildConfiguration::VcProjectBuildConfiguration(ProjectExplorer::Target *parent) :
    BuildConfiguration(parent, Core::Id(Constants::VC_PROJECT_BC_ID))
{
    m_buildDirectory = static_cast<VcProject *>(parent->project())->defaultBuildDirectory();
}

ProjectExplorer::BuildConfigWidget *VcProjectBuildConfiguration::createConfigWidget()
{
    return new VcProjectBuildSettingsWidget;
}

QString VcProjectBuildConfiguration::buildDirectory() const
{
    return QString();
}

ProjectExplorer::IOutputParser *VcProjectBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::IOutputParser *parserchain = new ProjectExplorer::GnuMakeParser;

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc)
        parserchain->appendOutputParser(tc->outputParser());
    return parserchain;
}

ProjectExplorer::BuildConfiguration::BuildType VcProjectBuildConfiguration::buildType() const
{
    return Debug;
}

VcProjectBuildConfiguration::VcProjectBuildConfiguration(ProjectExplorer::Target *parent, VcProjectBuildConfiguration *source)
    : BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

///////////////////////////////////////////
// VcProjectBuildConfigurationFactory class
///////////////////////////////////////////
VcProjectBuildConfigurationFactory::VcProjectBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{
}

QList<Core::Id> VcProjectBuildConfigurationFactory::availableCreationIds(const ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    return QList<Core::Id>() << Core::Id(Constants::VC_PROJECT_BC_ID);
}

QString VcProjectBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::VC_PROJECT_BC_ID)
        return tr("Vc Project");

    return QString();
}

bool VcProjectBuildConfigurationFactory::canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (id == Constants::VC_PROJECT_BC_ID)
        return true;
    return false;
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;
//    VcProject *project = static_cast<VcProject *>(parent->project());

    bool ok = true;
    QString buildConfigName = name;
    if (buildConfigName.isEmpty())
        buildConfigName = QInputDialog::getText(0,
                                                tr("New Vc Project Configuration"),
                                                tr("New Configuration name:"),
                                                QLineEdit::Normal,
                                                QString(), &ok);
    buildConfigName = buildConfigName.trimmed();
    if (!ok || buildConfigName.isEmpty())
        return 0;

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    bc->setDisplayName(buildConfigName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);

    VcMakeStep *makeStep = new VcMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    return bc;
}

bool VcProjectBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool VcProjectBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    VcProjectBuildConfiguration *old = static_cast<VcProjectBuildConfiguration *>(source);
    return new VcProjectBuildConfiguration(parent, old);
}

bool VcProjectBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<VcProject *>(t->project());
}

} // namespace Internal
} // namespace VcProjectManager
