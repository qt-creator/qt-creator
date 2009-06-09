#include "gccetoolchain.h"
#include "qt4project.h"

#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtDebug>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

GCCEToolChain::GCCEToolChain(S60Devices::Device device)
    : m_deviceId(device.id),
    m_deviceName(device.name),
    m_deviceRoot(device.epocRoot)
{

}

ToolChain::ToolChainType GCCEToolChain::type() const
{
    return ToolChain::GCCE;
}

QByteArray GCCEToolChain::predefinedMacros()
{
    // TODO
    return m_predefinedMacros;
}

QList<HeaderPath> GCCEToolChain::systemHeaderPaths()
{
    // TODO
    return m_systemHeaderPaths;
}

void GCCEToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QString("%1\\epoc32\\tools").arg(m_deviceRoot)); // e.g. make.exe
    env.prependOrSetPath(QString("%1\\epoc32\\gcc\\bin").arg(m_deviceRoot)); // e.g. gcc.exe
    env.set("EPOCDEVICE", QString("%1:%2").arg(m_deviceId, m_deviceName));
    env.set("EPOCROOT", S60Devices::cleanedRootPath(m_deviceRoot));
}

QString GCCEToolChain::makeCommand() const
{
    return "make";
}

QString GCCEToolChain::defaultMakeTarget(const Project *project) const
{
    const Qt4Project *qt4project = qobject_cast<const Qt4Project *>(project);
    if (qt4project) {
        if (!(QtVersion::QmakeBuildConfig(qt4project->qmakeStep()->value(
                project->activeBuildConfiguration(),
                "buildConfiguration").toInt()) & QtVersion::DebugBuild)) {
            return "release-gcce";
        }
    }
    return "debug-gcce";
}

bool GCCEToolChain::equals(ToolChain *other) const
{
    return (other->type() == type()
            && m_deviceId == static_cast<GCCEToolChain *>(other)->m_deviceId
            && m_deviceName == static_cast<GCCEToolChain *>(other)->m_deviceName);
}
