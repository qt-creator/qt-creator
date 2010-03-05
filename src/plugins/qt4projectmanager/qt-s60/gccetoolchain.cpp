/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gccetoolchain.h"
#include "qt4project.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QtDebug>

enum { debug = 0 };

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

// Locate the compiler via path.
static QString gcceCommand(const QString &dir)
{
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    if (!dir.isEmpty())
        env.prependOrSetPath(dir + QLatin1String("/bin"));
    QString gcce = QLatin1String("arm-none-symbianelf-gcc");
#ifdef Q_OS_WIN
    gcce += QLatin1String(".exe");
#endif
    const QString rc = env.searchInPath(gcce);
    if (rc.isEmpty()) {
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
                                     ProjectExplorer::ToolChain::ToolChainType type)
{
    const QString gccCommand = gcceCommand(gcceRoot);
    const QFileInfo gccCommandFi(gccCommand);
    const QString binPath = gccCommandFi.isRelative() ? QString() : gccCommandFi.absolutePath();
    return new GCCEToolChain(device, binPath, gccCommand, type);
}

GCCEToolChain::GCCEToolChain(const S60Devices::Device &device,
                             const QString &gcceBinPath,
                             const QString &gcceCommand,
                             ProjectExplorer::ToolChain::ToolChainType type) :
    GccToolChain(gcceCommand),
    m_mixin(device),
    m_type(type),
    m_gcceBinPath(gcceBinPath)
{
    QTC_ASSERT(m_type == ProjectExplorer::ToolChain::GCCE || m_type == ProjectExplorer::ToolChain::GCCE_GNUPOC, return)
    if (debug)
        qDebug() << "GCCEToolChain on" << m_type << gcceCommand << gcceBinPath << m_mixin.device();
}

ToolChain::ToolChainType GCCEToolChain::type() const
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
        case ProjectExplorer::ToolChain::GCCE:
            m_systemHeaderPaths += m_mixin.epocHeaderPaths();
            break;
        case ProjectExplorer::ToolChain::GCCE_GNUPOC:
            m_systemHeaderPaths += m_mixin.gnuPocHeaderPaths();
            break;
        default:
            break;
        }
    }
    return m_systemHeaderPaths;
}

void GCCEToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    if (debug)
        qDebug() << "GCCEToolChain::addToEnvironment" << m_type << gcc() << m_gcceBinPath<< m_mixin.device();

    if (!m_gcceBinPath.isEmpty())
        env.prependOrSetPath(m_gcceBinPath);
    switch (m_type) {
    case ProjectExplorer::ToolChain::GCCE:
        m_mixin.addEpocToEnvironment(&env);
    case ProjectExplorer::ToolChain::GCCE_GNUPOC:
        m_mixin.addGnuPocToEnvironment(&env);
        break;
    default:
        break;
    }
}

QString GCCEToolChain::makeCommand() const
{
    return QLatin1String("make");
}

bool GCCEToolChain::equals(ToolChain *otherIn) const
{
    if (otherIn->type() != type())
                return false;
    const GCCEToolChain *other = static_cast<const GCCEToolChain *>(otherIn);
    return m_mixin == other->m_mixin
           && m_gcceBinPath == other->m_gcceBinPath
           && gcc() == other->gcc();
}
