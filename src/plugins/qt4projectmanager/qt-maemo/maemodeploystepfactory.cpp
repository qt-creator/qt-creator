#include "maemodeploystepfactory.h"

#include "maemodeploystep.h"
#include "maemoglobal.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepFactory::MaemoDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList MaemoDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && MaemoGlobal::isMaemoTargetId(parent->target()->id())
        && !parent->contains(MaemoDeployStep::Id))
        return QStringList() << MaemoDeployStep::Id;
    return QStringList();
}

QString MaemoDeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == MaemoDeployStep::Id)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::MaemoDeployStepFactory",
                                           "Deploy to device");
    return QString();
}

bool MaemoDeployStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            && id == QLatin1String(MaemoDeployStep::Id)
            && MaemoGlobal::isMaemoTargetId(parent->target()->id())
            && !parent->contains(MaemoDeployStep::Id);
}

BuildStep *MaemoDeployStepFactory::create(BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new MaemoDeployStep(parent);
}

bool MaemoDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MaemoDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    MaemoDeployStep * const step = new MaemoDeployStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MaemoDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MaemoDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new MaemoDeployStep(parent, static_cast<MaemoDeployStep*>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
