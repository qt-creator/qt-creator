#include "maemodeploystep.h"

#include "maemodeploystepwidget.h"

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String MaemoDeployStep::Id("Qt4ProjectManager.MaemoDeployStep");


MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc)
    : BuildStep(bc, Id)
{
}

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc,
    MaemoDeployStep *other)
    : BuildStep(bc, other)
{
}

bool MaemoDeployStep::init()
{
    return true;
}

void MaemoDeployStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(true);
}

BuildStepConfigWidget *MaemoDeployStep::createConfigWidget()
{
    return new MaemoDeployStepWidget;
}

} // namespace Internal
} // namespace Qt4ProjectManager
