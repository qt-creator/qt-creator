#include "gccetoolchain.h"
#include "qt4project.h"

#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtDebug>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

namespace {
    const char *GCCE_COMMAND = "arm-none-symbianelf-gcc.exe";
}

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
    if (m_predefinedMacros.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-dM")
                  << QLatin1String("-");

        QProcess cpp;
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        cpp.setEnvironment(env.toStringList());
        cpp.start(QLatin1String(GCCE_COMMAND), arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();
        m_predefinedMacros = cpp.readAllStandardOutput();
    }
    return m_predefinedMacros;
}

QList<HeaderPath> GCCEToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-v")
                  << QLatin1String("-");

        QProcess cpp;
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        cpp.setEnvironment(env.toStringList());
        cpp.setReadChannelMode(QProcess::MergedChannels);
        cpp.start(QLatin1String(GCCE_COMMAND), arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();

        QByteArray line;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include"))
                break;
        }

        if (! line.isEmpty() && line.startsWith("#include")) {
            HeaderPath::Kind kind = HeaderPath::UserHeaderPath;
            while (cpp.canReadLine()) {
                line = cpp.readLine();
                if (line.startsWith("#include")) {
                    kind = HeaderPath::GlobalHeaderPath;
                } else if (! line.isEmpty() && QChar(line.at(0)).isSpace()) {
                    HeaderPath::Kind thisHeaderKind = kind;

                    line = line.trimmed();
                    if (line.endsWith('\n'))
                        line.chop(1);

                    int index = line.indexOf(" (framework directory)");
                    if (index != -1) {
                        line = line.left(index);
                        thisHeaderKind = HeaderPath::FrameworkHeaderPath;
                    }

                    m_systemHeaderPaths.append(HeaderPath(QFile::decodeName(line), thisHeaderKind));
                } else if (line.startsWith("End of search list.")) {
                    break;
                } else {
                    qWarning() << "ignore line:" << line;
                }
            }
        }
    }
    m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis\\sys").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\variant").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    return m_systemHeaderPaths;
}

void GCCEToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    // TODO: do we need to set path to gcce?
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
