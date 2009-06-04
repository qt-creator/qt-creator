#ifndef WINSCWTOOLCHAIN_H
#define WINSCWTOOLCHAIN_H

#include "s60devices.h"

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
namespace Internal {

class WINSCWToolChain : public ProjectExplorer::ToolChain
{
public:
    WINSCWToolChain(S60Devices::Device device, const QString &mwcDirectory);
    QByteArray predefinedMacros();
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths();
    void addToEnvironment(ProjectExplorer::Environment &env);
    ProjectExplorer::ToolChain::ToolChainType type() const;
    QString makeCommand() const;

protected:
    bool equals(ToolChain *other) const;

private:
    QString m_carbidePath;
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceRoot;
    QByteArray m_predefinedMacros;
    QList<ProjectExplorer::HeaderPath> m_systemHeaderPaths;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // WINSCWTOOLCHAIN_H
