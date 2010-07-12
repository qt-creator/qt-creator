#include "maemodeploystepfactory.h"

#include "maemodeploystep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepFactory::MaemoDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList MaemoDeployStepFactory::availableCreationIds(BuildConfiguration *,
    BuildStep::Type) const
{
    return QStringList();
}

QString MaemoDeployStepFactory::displayNameForId(const QString &) const
{
    return QString();
}

bool MaemoDeployStepFactory::canCreate(BuildConfiguration *,
    BuildStep::Type, const QString &) const
{
    return false;
}

BuildStep *MaemoDeployStepFactory::create(BuildConfiguration *,
    BuildStep::Type, const QString &)
{
    Q_ASSERT(false);
    return 0;
}

bool MaemoDeployStepFactory::canRestore(BuildConfiguration *parent,
    BuildStep::Type type, const QVariantMap &map) const
{
    return canCreateInternally(parent, type, idFromMap(map));
}

BuildStep *MaemoDeployStepFactory::restore(BuildConfiguration *parent,
    BuildStep::Type type, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, type, map));
    MaemoDeployStep * const step = new MaemoDeployStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MaemoDeployStepFactory::canClone(BuildConfiguration *parent,
    BuildStep::Type type, BuildStep *product) const
{
    return canCreateInternally(parent, type, product->id());
}

BuildStep *MaemoDeployStepFactory::clone(BuildConfiguration *parent,
    BuildStep::Type type, BuildStep *product)
{
    Q_ASSERT(canClone(parent, type, product));
    return new MaemoDeployStep(parent, static_cast<MaemoDeployStep*>(product));
}

bool MaemoDeployStepFactory::canCreateInternally(BuildConfiguration *parent,
    BuildStep::Type type, const QString &id) const
{
    return type == BuildStep::Deploy && id == MaemoDeployStep::Id
        && parent->target()->id() == Constants::MAEMO_DEVICE_TARGET_ID;
}

} // namespace Internal
} // namespace Qt4ProjectManager
