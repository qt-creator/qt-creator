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

#include "winscwtoolchain.h"

#include "winscwparser.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

WINSCWToolChain::WINSCWToolChain(S60Devices::Device device, const QString &mwcDirectory)
    : m_mixin(device),
    m_carbidePath(mwcDirectory),
    m_deviceId(device.id),
    m_deviceName(device.name),
    m_deviceRoot(device.epocRoot)
{

}

ToolChain::ToolChainType WINSCWToolChain::type() const
{
    return ToolChain::WINSCW;
}

QByteArray WINSCWToolChain::predefinedMacros()
{
    return QByteArray("#define __SYMBIAN32__\n");
}

QList<HeaderPath> WINSCWToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        foreach (const QString &value, systemIncludes()) {
            m_systemHeaderPaths.append(HeaderPath(value, HeaderPath::GlobalHeaderPath));
        }
        m_systemHeaderPaths += m_mixin.epocHeaderPaths();
    }
    return m_systemHeaderPaths;
}

QStringList WINSCWToolChain::systemIncludes() const
{
    if (m_carbidePath.isEmpty()) {
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        QString symIncludesValue = env.value("MWCSYM2INCLUDES");
        if (!symIncludesValue.isEmpty())
            return symIncludesValue.split(QLatin1Char(';'));
    } else {
        QStringList symIncludes = QStringList()
            << "\\MSL\\MSL_C\\MSL_Common\\Include"
            << "\\MSL\\MSL_C\\MSL_Win32\\Include"
            << "\\MSL\\MSL_CMSL_X86"
            << "\\MSL\\MSL_C++\\MSL_Common\\Include"
            << "\\MSL\\MSL_Extras\\MSL_Common\\Include"
            << "\\MSL\\MSL_Extras\\MSL_Win32\\Include"
            << "\\Win32-x86 Support\\Headers\\Win32 SDK";
        for (int i = 0; i < symIncludes.size(); ++i)
            symIncludes[i].prepend(QString("%1\\x86Build\\Symbian_Support").arg(m_carbidePath));
        return symIncludes;
    }
    return QStringList();
}

void WINSCWToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    if (!m_carbidePath.isEmpty()) {
        env.set("MWCSYM2INCLUDES", systemIncludes().join(QString(QLatin1Char(';'))));
        QStringList symLibraries = QStringList()
            << "\\Win32-x86 Support\\Libraries\\Win32 SDK"
            << "\\Runtime\\Runtime_x86\\Runtime_Win32\\Libs";
        for (int i = 0; i < symLibraries.size(); ++i)
            symLibraries[i].prepend(QString("%1\\x86Build\\Symbian_Support").arg(m_carbidePath));
        env.set("MWSYM2LIBRARIES", symLibraries.join(";"));
        env.set("MWSYM2LIBRARYFILES", "MSL_All_MSE_Symbian_D.lib;gdi32.lib;user32.lib;kernel32.lib");
        env.prependOrSetPath(QString("%1\\x86Build\\Symbian_Tools\\Command_Line_Tools").arg(m_carbidePath)); // compiler
    }
    m_mixin.addEpocToEnvironment(&env);
}

QString WINSCWToolChain::makeCommand() const
{
    return QLatin1String("make");
}

IOutputParser *WINSCWToolChain::outputParser() const
{
    return new WinscwParser;
}

bool WINSCWToolChain::equals(ToolChain *other) const
{
    WINSCWToolChain *otherWINSCW = static_cast<WINSCWToolChain *>(other);
    return (other->type() == type()
            && m_deviceId == otherWINSCW->m_deviceId
            && m_deviceName == otherWINSCW->m_deviceName
            && m_deviceRoot == otherWINSCW->m_deviceRoot
            && m_carbidePath == otherWINSCW->m_carbidePath);
}
