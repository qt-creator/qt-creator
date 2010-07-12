#ifndef MAEMODEPLOYSTEPFACTORY_H
#define MAEMODEPLOYSTEPFACTORY_H

#include <projectexplorer/buildstep.h>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
public:
    MaemoDeployStepFactory(QObject *parent);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *parent,
                                             ProjectExplorer::BuildStep::Type type) const;
    virtual QString displayNameForId(const QString &id) const;

    virtual bool canCreate(ProjectExplorer::BuildConfiguration *parent,
                           ProjectExplorer::BuildStep::Type type,
                           const QString &id) const;
    virtual ProjectExplorer::BuildStep *
            create(ProjectExplorer::BuildConfiguration *parent,
                   ProjectExplorer::BuildStep::Type type, const QString &id);

    virtual bool canRestore(ProjectExplorer::BuildConfiguration *parent,
                            ProjectExplorer::BuildStep::Type type,
                            const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *
            restore(ProjectExplorer::BuildConfiguration *parent,
                    ProjectExplorer::BuildStep::Type type, const QVariantMap &map);

    virtual bool canClone(ProjectExplorer::BuildConfiguration *parent,
                          ProjectExplorer::BuildStep::Type type,
                          ProjectExplorer::BuildStep *product) const;
    virtual ProjectExplorer::BuildStep *
            clone(ProjectExplorer::BuildConfiguration *parent,
                  ProjectExplorer::BuildStep::Type type,
                  ProjectExplorer::BuildStep *product);

private:
    bool canCreateInternally(ProjectExplorer::BuildConfiguration *parent,
                             ProjectExplorer::BuildStep::Type type,
                             const QString &id) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEPFACTORY_H
