#include "vcmakestep.h"

#include "vcprojectbuildconfiguration.h"
#include "vcprojectfile.h"
#include "vcprojectmanagerconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

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

bool VcMakeStep::init()
{
    VcProjectBuildConfiguration *bc = vcProjectBuildConfiguration();

    if (!bc) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Qt Creator didn't detected any proper build tool for .vcproj files."),
                                             Utils::FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return true;
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommand(QString("C:/Windows/Microsoft.NET/Framework/v3.5/MSBuild.exe"));
    ProjectExplorer::Project *project = bc->target()->project();
    VcProjectFile* document = static_cast<VcProjectFile *>(project->document());

    pp->setArguments(document->filePath());
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
        return;
    }

    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractProcessStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface->reportFinished();
    m_futureInterface = 0;
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

QVariantMap VcMakeStep::toMap() const
{
    return BuildStep::toMap();
}

bool VcMakeStep::fromMap(const QVariantMap &map)
{
    return BuildStep::fromMap(map);
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
    QLabel *label = new QLabel("C:/Windows/Microsoft.NET/Framework/v3.5/MSBuild.exe");
    mainLayout->addRow(tr("Command:"), label);
    setLayout(mainLayout);
}

QString VcMakeStepConfigWidget::displayName() const
{
    return tr("Vc Make Step Config Widget");
}

QString VcMakeStepConfigWidget::summaryText() const
{
    return tr("This is Vc Project's build step configuration widget.");
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
    qDebug() << "VcMakeStepFactory::canCreate()";
    if (parent->target()->project()->id() == Constants::VC_PROJECT_ID)
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
    if (parent->target() && parent->target()->project()) {
        if (parent->target()->project()->id() == Constants::VC_PROJECT_ID) {
            return QList<Core::Id>() << Core::Id(MS_ID);
        }
    }
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
