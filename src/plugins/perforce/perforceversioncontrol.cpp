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

#include "perforceversioncontrol.h"
#include "perforceplugin.h"
#include "perforceconstants.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

namespace Perforce {
namespace Internal {

PerforceVersionControl::PerforceVersionControl(PerforcePlugin *plugin) :
    m_enabled(true),
    m_plugin(plugin)
{
}

QString PerforceVersionControl::displayName() const
{
    return QLatin1String("perforce");
}

bool PerforceVersionControl::supportsOperation(Operation operation) const
{
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case OpenOperation:
        return true;
    case CreateRepositoryOperation:
    case SnapshotOperations:
        break;
    }
    return false;
}

bool PerforceVersionControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsOpen(fi.absolutePath(), fi.fileName());
}

bool PerforceVersionControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool PerforceVersionControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool PerforceVersionControl::vcsCreateRepository(const QString &)
{
    return false;
}

QString PerforceVersionControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList PerforceVersionControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool PerforceVersionControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool PerforceVersionControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool PerforceVersionControl::managesDirectory(const QString &directory) const
{
    const bool rc = m_plugin->managesDirectory(directory);
    if (Perforce::Constants::debug)
        qDebug() << "managesDirectory" << directory << rc;
    return rc;
}

QString PerforceVersionControl::findTopLevelForDirectory(const QString &directory) const
{
    const QString rc = m_plugin->findTopLevelForDirectory(directory);
    if (Perforce::Constants::debug)
        qDebug() << "findTopLevelForDirectory" << directory << rc;
    return rc;
}

void PerforceVersionControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void PerforceVersionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

} // Internal
} // Perforce
