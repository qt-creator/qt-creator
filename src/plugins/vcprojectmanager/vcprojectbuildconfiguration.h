#ifndef VCPROJECTMANAGER_BUILDCONFIGURATION_H
#define VCPROJECTMANAGER_BUILDCONFIGURATION_H

#include "vcproject.h"

#include <projectexplorer/buildconfiguration.h>

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class VcProjectBuildConfigurationFactory;

public:
    explicit VcProjectBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();
    QString buildDirectory() const;
    ProjectExplorer::IOutputParser *createOutputParser() const;
    BuildType buildType() const;

    const VcProjectInfo::ConfigurationInfo &info() const;
    void setInfo(const VcProjectInfo::ConfigurationInfo &info);

protected:
    VcProjectBuildConfiguration(ProjectExplorer::Target *parent, VcProjectBuildConfiguration *source);

private:
    QString m_buildDirectory;
    VcProjectInfo::ConfigurationInfo m_info;
};

class VcProjectBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit VcProjectBuildConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;
    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    VcProjectBuildConfiguration* create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    VcProjectBuildConfiguration* restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    VcProjectBuildConfiguration* clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

} // namespace Internal
} // namespace VcProjectManager
#endif // VCPROJECTBUILDCONFIGURATION_H
