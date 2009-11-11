/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QDir>
#include <QtDebug>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

GCCEToolChain::GCCEToolChain(S60Devices::Device device, const QString &gcceCommand)
    : GccToolChain(gcceCommand),
    m_deviceId(device.id),
    m_deviceName(device.name),
    m_deviceRoot(device.epocRoot),
    m_gcceCommand(gcceCommand)
{

}

ToolChain::ToolChainType GCCEToolChain::type() const
{
    return ToolChain::GCCE;
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
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis\\sys").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\variant").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    }
    return m_systemHeaderPaths;
}

void GCCEToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QString("%1\\epoc32\\tools").arg(m_deviceRoot)); // e.g. make.exe
    env.prependOrSetPath(QString("%1\\epoc32\\gcc\\bin").arg(m_deviceRoot)); // e.g. gcc.exe
    env.prependOrSetPath(QFileInfo(m_gcceCommand).absolutePath());
    env.set("EPOCDEVICE", QString("%1:%2").arg(m_deviceId, m_deviceName));
    env.set("EPOCROOT", S60Devices::cleanedRootPath(m_deviceRoot));
}

QString GCCEToolChain::makeCommand() const
{
    return "make";
}

bool GCCEToolChain::equals(ToolChain *other) const
{
    GCCEToolChain *otherGCCE = static_cast<GCCEToolChain *>(other);
    return (other->type() == type()
            && m_deviceId == otherGCCE->m_deviceId
            && m_deviceName == otherGCCE->m_deviceName
            && m_deviceRoot == otherGCCE->m_deviceRoot
            && m_gcceCommand == otherGCCE->m_gcceCommand);
}
