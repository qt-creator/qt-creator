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

#include "cvscontrol.h"
#include "cvsplugin.h"

using namespace CVS;
using namespace CVS::Internal;

CVSControl::CVSControl(CVSPlugin *plugin) :
    m_enabled(true),
    m_plugin(plugin)
{
}

QString CVSControl::name() const
{
    return QLatin1String("cvs");
}

bool CVSControl::isEnabled() const
{
     return m_enabled;
}

void CVSControl::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }
}

bool CVSControl::supportsOperation(Operation operation) const
{
    bool rc = true;
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
        break;
    case OpenOperation:
        rc = false;
        break;
    }
    return rc;
}

bool CVSControl::vcsOpen(const QString & /* fileName */)
{
    // Open for edit: N/A
    return true;
}

bool CVSControl::vcsAdd(const QString &fileName)
{
    return m_plugin->vcsAdd(fileName);
}

bool CVSControl::vcsDelete(const QString &fileName)
{
    return m_plugin->vcsDelete(fileName);
}

bool CVSControl::managesDirectory(const QString &directory) const
{
    return m_plugin->managesDirectory(directory);
}

QString CVSControl::findTopLevelForDirectory(const QString &directory) const
{
    return m_plugin->findTopLevelForDirectory(directory);
}

void CVSControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void CVSControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

