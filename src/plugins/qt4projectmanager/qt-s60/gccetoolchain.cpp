/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gccetoolchain.h"
#include "qt4project.h"

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QtDebug>

enum { debug = 0 };

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

// Locate the compiler via path.
static QString gcceCommand(const QString &dir)
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    if (!dir.isEmpty()) {
        env.prependOrSetPath(dir + QLatin1String("/bin"));
        env.prependOrSetPath(dir);
    }
    QString gcce = QLatin1String("arm-none-symbianelf-gcc");
#ifdef Q_OS_WIN
    gcce += QLatin1String(".exe");
#endif
    const QString rc = env.searchInPath(gcce);
    if (debug && rc.isEmpty()) {
        const QString msg = QString::fromLatin1("GCCEToolChain: Unable to locate '%1' in '%2' (GCCE root: '%3')")
                            .arg(gcce, env.value(QLatin1String("PATH")), dir);
        qWarning("%s", qPrintable(msg));
        return gcce;
    }
    return rc;
}

// The GccToolChain base class constructor wants to know the gcc command
GCCEToolChain *GCCEToolChain::create(const S60Devices::Device &device,
                                     const QString &gcceRoot,
                                     ProjectExplorer::ToolChainType type)
{
    const QString gccCommand = gcceCommand(gcceRoot);
    const QFileInfo gccCommandFi(gccCommand);
    const QString binPath = gccCommandFi.isRelative() ? QString() : gccCommandFi.absolutePath();
    return new GCCEToolChain(device, binPath, gccCommand, type);
}

GCCEToolChain::GCCEToolChain(const S60Devices::Device &device,
                             const QString &gcceBinPath,
                             const QString &gcceCommand,
                             ProjectExplorer::ToolChainType type) :
    GccToolChain(gcceCommand),
    m_mixin(device),
    m_type(type),
    m_gcceBinPath(gcceBinPath)
{
    QTC_ASSERT(m_type == ProjectExplorer::ToolChain_GCCE || m_type == ProjectExplorer::ToolChain_GCCE_GNUPOC, return)
    if (debug)
        qDebug() << "GCCEToolChain on" << m_type << gcceCommand << gcceBinPath << m_mixin.device();
}

ProjectExplorer::ToolChainType GCCEToolChain::type() const
{
    return m_type;
}

QByteArray GCCEToolChain::predefinedMacros()
{
    if (m_predefinedMacros.isEmpty()) {
        ProjectExplorer::GccToolChain::predefinedMacros();
        m_predefinedMacros += "\n"
                "#define __GCCE__\n"
                "#define __SYMBIAN32__\n";
    }
    return m_predefinedMacros;
}

QList<HeaderPath> GCCEToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        GccToolChain::systemHeaderPaths();
        switch (m_type) {
        case ProjectExplorer::ToolChain_GCCE:
            m_systemHeaderPaths += m_mixin.epocHeaderPaths();
            break;
        case ProjectExplorer::ToolChain_GCCE_GNUPOC:
            m_systemHeaderPaths += m_mixin.gnuPocHeaderPaths();
            break;
        default:
            break;
        }
    }
    return m_systemHeaderPaths;
}

void GCCEToolChain::addToEnvironment(Utils::Environment &env)
{
    if (debug)
        qDebug() << "GCCEToolChain::addToEnvironment" << m_type << gcc() << m_gcceBinPath<< m_mixin.device();

    if (!m_gcceBinPath.isEmpty())
        env.prependOrSetPath(m_gcceBinPath);
    switch (m_type) {
    case ProjectExplorer::ToolChain_GCCE:
        m_mixin.addEpocToEnvironment(&env);
        break;
    case ProjectExplorer::ToolChain_GCCE_GNUPOC:
        m_mixin.addGnuPocToEnvironment(&env);
        break;
    default:
        break;
    }
    QString version = gcceVersion();
    env.set(QLatin1String("QT_GCCE_VERSION"), version);
    version = version.remove(QLatin1Char('.'));
    env.set(QString::fromLatin1("SBS_GCCE") + version + QLatin1String("BIN"), QDir::toNativeSeparators(m_gcceBinPath));
}

QString GCCEToolChain::makeCommand() const
{
#if defined (Q_OS_WIN)
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

bool GCCEToolChain::equals(const ToolChain *otherIn) const
{
    if (otherIn->type() != type())
                return false;
    const GCCEToolChain *other = static_cast<const GCCEToolChain *>(otherIn);
    return m_mixin == other->m_mixin
           && m_gcceBinPath == other->m_gcceBinPath
           && gcc() == other->gcc();
}

QString GCCEToolChain::gcceVersion() const
{
    if (m_gcceVersion.isEmpty()) {
        QString command = gcceCommand(m_gcceBinPath);
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

        if (gxx.canReadLine())
            m_gcceVersion = gxx.readLine().trimmed();
    }
    return m_gcceVersion;
}
