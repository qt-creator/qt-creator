/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "perforceversioncontrol.h"
#include "perforceplugin.h"

namespace Perforce {
namespace Internal {

PerforceVersionControl::PerforceVersionControl(PerforcePlugin *plugin) :
    m_enabled(true),
    m_plugin(plugin)
{
}

QString PerforceVersionControl::name() const
{
    return QLatin1String("perforce");
}

bool PerforceVersionControl::isEnabled() const
{
     return m_enabled;
}

void PerforceVersionControl::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }
}

bool PerforceVersionControl::supportsOperation(Operation operation) const
{
    bool rc = true;
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case OpenOperation:
        break;
    }
    return rc;
}

bool PerforceVersionControl::vcsOpen(const QString &fileName)
{
    return m_plugin->vcsOpen(fileName);
}

bool PerforceVersionControl::vcsAdd(const QString &fileName)
{
    return m_plugin->vcsAdd(fileName);
}

bool PerforceVersionControl::vcsDelete(const QString &fileName)
{
    return m_plugin->vcsDelete(fileName);
}

bool PerforceVersionControl::managesDirectory(const QString &directory) const
{
    return m_plugin->managesDirectory(directory);
}

QString PerforceVersionControl::findTopLevelForDirectory(const QString &directory) const
{
    return m_plugin->findTopLevelForDirectory(directory);
}

} // Internal
} // Perforce
