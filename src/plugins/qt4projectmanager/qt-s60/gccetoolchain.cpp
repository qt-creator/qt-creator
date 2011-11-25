/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gccetoolchain.h"
#include "qt4projectmanagerconstants.h"

#include <utils/environment.h>
#include <utils/synchronousprocess.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtversionmanager.h>

#include <QtCore/QDir>

namespace Qt4ProjectManager {
namespace Internal {

static QString gcceVersion(const QString &command)
{
    if (command.isEmpty())
        return QString();

    QProcess gxx;
    QStringList arguments;
    arguments << QLatin1String("-dumpversion");
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));   //override current locale settings
    gxx.setEnvironment(env.toStringList());
    gxx.setReadChannelMode(QProcess::MergedChannels);
    gxx.start(command, arguments);
    if (!gxx.waitForStarted()) {
        qWarning("Cannot start '%s': %s", qPrintable(command), qPrintable(gxx.errorString()));
        return QString();
    }
    gxx.closeWriteChannel();
    if (!gxx.waitForFinished())      {
        Utils::SynchronousProcess::stopProcess(gxx);
        qWarning("Timeout running '%s'.", qPrintable(command));
        return QString();
    }
    if (gxx.exitStatus() != QProcess::NormalExit) {
        qWarning("'%s' crashed.", qPrintable(command));
        return QString();
    }

    QString version = QString::fromLocal8Bit(gxx.readLine().trimmed());
    if (version.contains(QRegExp("^\\d+\\.\\d+\\.\\d+.*$")))
        return version;

    return QString();
}


// ==========================================================================
// GcceToolChain
// ==========================================================================

QString GcceToolChain::typeName() const
{
    return GcceToolChainFactory::tr("GCCE");
}

QByteArray GcceToolChain::predefinedMacros() const
{
    if (m_predefinedMacros.isEmpty()) {
        ProjectExplorer::GccToolChain::predefinedMacros();
        m_predefinedMacros += "\n"
                "#define __GCCE__\n"
                "#define __SYMBIAN32__\n";
    }
    return m_predefinedMacros;
}

void GcceToolChain::addToEnvironment(Utils::Environment &env) const
{
    GccToolChain::addToEnvironment(env);

    if (m_gcceVersion.isEmpty())
        m_gcceVersion = gcceVersion(compilerPath());
    if (m_gcceVersion.isEmpty())
        return;

    env.set(QLatin1String("QT_GCCE_VERSION"), m_gcceVersion);
    QString version = m_gcceVersion;
    env.set(QString::fromLatin1("SBS_GCCE") + version.remove(QLatin1Char('.'))
            + QLatin1String("BIN"),
            QDir::toNativeSeparators(QFileInfo(compilerPath()).absolutePath()));
    // Required for SBS, which checks the version output from its tools
    // and gets confused by localized output.
    env.set(QLatin1String("LANG"), QString(QLatin1Char('C')));
}

QString GcceToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

Utils::FileName GcceToolChain::mkspec() const
{
    return Utils::FileName(); // always use default from Qt version
}

QString GcceToolChain::defaultMakeTarget() const
{
    return QLatin1String("gcce");
}

void GcceToolChain::setCompilerPath(const QString &path)
{
    m_gcceVersion.clear();
    GccToolChain::setCompilerPath(path);
}

ProjectExplorer::ToolChain *GcceToolChain::clone() const
{
    return new GcceToolChain(*this);
}

GcceToolChain::GcceToolChain(bool autodetected) :
    GccToolChain(QLatin1String(Constants::GCCE_TOOLCHAIN_ID), autodetected)
{ }

// ==========================================================================
// GcceToolChainFactory
// ==========================================================================

QString GcceToolChainFactory::displayName() const
{
    return tr("GCCE");
}

QString GcceToolChainFactory::id() const
{
    return QLatin1String(Constants::GCCE_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> GcceToolChainFactory::autoDetect()
{
    QList<ProjectExplorer::ToolChain *> result;

    // Compatibility to pre-2.2:
    while (true) {
        const QString path = QtSupport::QtVersionManager::instance()->popPendingGcceUpdate();
        if (path.isNull())
            break;

        QFileInfo fi(path + QLatin1String("/bin/arm-none-symbianelf-g++.exe"));
        if (fi.exists() && fi.isExecutable()) {
            GcceToolChain *tc = new GcceToolChain(false);
            tc->setCompilerPath(fi.absoluteFilePath());
            tc->setDisplayName(tr("GCCE from Qt version"));
            tc->setDebuggerCommand(ProjectExplorer::ToolChainManager::instance()->defaultDebugger(tc->targetAbi()));
            result.append(tc);
        }
    }

    QString fullPath = Utils::Environment::systemEnvironment().searchInPath(QLatin1String("arm-none-symbianelf-gcc"));
    QString version = gcceVersion(fullPath);
    // If version is empty then this is not a GCC but e.g. bullseye!
    if (!fullPath.isEmpty() && !version.isEmpty()) {
        GcceToolChain *tc = new GcceToolChain(true);
        tc->setCompilerPath(fullPath);
        tc->setDisplayName(tr("GCCE (%1)").arg(version));
        tc->setDebuggerCommand(ProjectExplorer::ToolChainManager::instance()->defaultDebugger(tc->targetAbi()));
        if (tc->targetAbi() == ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture,
                                                    ProjectExplorer::Abi::SymbianOS,
                                                    ProjectExplorer::Abi::SymbianDeviceFlavor,
                                                    ProjectExplorer::Abi::ElfFormat,
                                                    32))
            result.append(tc);
    }
    return result;
}

bool GcceToolChainFactory::canCreate()
{
    return true;
}

ProjectExplorer::ToolChain *GcceToolChainFactory::create()
{
    GcceToolChain *tc = new GcceToolChain(false);
    tc->setDisplayName(tr("GCCE"));
    return tc;
}

bool GcceToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::GCCE_TOOLCHAIN_ID));
}

ProjectExplorer::ToolChain *GcceToolChainFactory::restore(const QVariantMap &data)
{
    GcceToolChain *tc = new GcceToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

} // namespace Internal
} // namespace Qt4ProjectManager
