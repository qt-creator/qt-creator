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

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QtDebug>

enum { debug = 0 };

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

GCCEToolChain::GCCEToolChain(const S60Devices::Device &device,
                             const QString &gcceCommand,
                             ProjectExplorer::ToolChain::ToolChainType type) :
    GccToolChain(gcceCommand),
    m_mixin(device),
    m_type(type),
    m_gcceCommand(gcceCommand)
{
    QTC_ASSERT(m_type == ProjectExplorer::ToolChain::GCCE || m_type == ProjectExplorer::ToolChain::GCCE_GNUPOC, return)
    if (debug)
        qDebug() << "GCCEToolChain on" << m_type << m_mixin.device();
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
    switch (m_type) {
    case ProjectExplorer::ToolChain::GCCE:
        m_mixin.addEpocToEnvironment(&env);
        env.prependOrSetPath(QFileInfo(m_gcceCommand).absolutePath());
    case ProjectExplorer::ToolChain::GCCE_GNUPOC:
        break;
    default:
        m_mixin.addGnuPocToEnvironment(&env);
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
           && m_gcceCommand == other->m_gcceCommand;
}
