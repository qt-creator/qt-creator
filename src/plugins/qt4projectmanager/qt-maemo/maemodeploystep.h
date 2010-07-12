#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include <projectexplorer/buildstep.h>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc);

private:
    MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc,
        MaemoDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }

    static const QLatin1String Id;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEP_H
