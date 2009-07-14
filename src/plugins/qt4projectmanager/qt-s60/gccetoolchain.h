#ifndef GCCETOOLCHAIN_H
#define GCCETOOLCHAIN_H

#include "s60devices.h"

#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
namespace Internal {

class GCCEToolChain : public ProjectExplorer::GccToolChain
{
public:
    GCCEToolChain(S60Devices::Device device);
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths();
    void addToEnvironment(ProjectExplorer::Environment &env);
    ProjectExplorer::ToolChain::ToolChainType type() const;
    QString makeCommand() const;
    QString defaultMakeTarget() const;

    void setProject(const ProjectExplorer::Project *project);

protected:
    bool equals(ToolChain *other) const;

private:
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceRoot;
    const ProjectExplorer::Project *m_project;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GCCETOOLCHAIN_H
