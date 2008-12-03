#include "cmakestep.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeStep::CMakeStep(CMakeProject *pro)
    : BuildStep(pro), m_pro(pro)
{

}

CMakeStep::~CMakeStep()
{

}

bool CMakeStep::init(const QString &buildConfiguration)
{
    // TODO
}

void CMakeStep::run(QFutureInterface<bool> &fi)
{
    // TODO
    fi.reportResult(true);
}

QString CMakeStep::name()
{
    return "CMake";
}

QString CMakeStep::displayName()
{
    return Constants::CMAKESTEP;
}

ProjectExplorer::BuildStepConfigWidget *CMakeStep::createConfigWidget()
{
    return new CMakeBuildStepConfigWidget();
}

bool CMakeStep::immutable() const
{
    return true;
}

//
// CMakeBuildStepConfigWidget
//

QString CMakeBuildStepConfigWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildStepConfigWidget::init(const QString &buildConfiguration)
{
    // TODO
}

//
// CMakeBuildStepFactory
//

bool CMakeBuildStepFactory::canCreate(const QString &name) const
{
    return (Constants::CMAKESTEP == name);
}

ProjectExplorer::BuildStep *CMakeBuildStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::CMAKESTEP);
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    return new CMakeStep(pro);
}

QStringList CMakeBuildStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    return QStringList();
}

QString CMakeBuildStepFactory::displayNameForName(const QString &name) const
{
    return "CMake";
}

