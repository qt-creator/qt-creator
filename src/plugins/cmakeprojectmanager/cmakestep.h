#ifndef CMAKESTEP_H
#define CMAKESTEP_H

#include <projectexplorer/buildstep.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

class CMakeBuildStepConfigWidget;

class CMakeStep : public ProjectExplorer::BuildStep
{
public:
    CMakeStep(CMakeProject *pro);
    ~CMakeStep();
    virtual bool init(const QString &buildConfiguration);

    virtual void run(QFutureInterface<bool> &fi);

    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
private:
    CMakeProject *m_pro;
};

class CMakeBuildStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
public:
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
};

class CMakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    virtual bool canCreate(const QString &name) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::Project *pro, const QString &name) const;
    virtual QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    virtual QString displayNameForName(const QString &name) const;
};


}
}
#endif // CMAKESTEP_H
