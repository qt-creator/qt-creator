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

#include "subversioncontrol.h"
#include "subversionplugin.h"

#include <QtCore/QFileInfo>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionControl::SubversionControl(SubversionPlugin *plugin) :
    m_enabled(true),
    m_plugin(plugin)
{
}

QString SubversionControl::displayName() const
{
    return QLatin1String("subversion");
}

bool SubversionControl::supportsOperation(Operation operation) const
{
    bool rc = true;
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case AnnotateOperation:
        break;
    case OpenOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

bool SubversionControl::vcsOpen(const QString & /* fileName */)
{
    // Open for edit: N/A
    return true;
}

bool SubversionControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool SubversionControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool SubversionControl::vcsCreateRepository(const QString &)
{
    return false;
}

QString SubversionControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList SubversionControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool SubversionControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool SubversionControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool SubversionControl::managesDirectory(const QString &directory) const
{
    return m_plugin->managesDirectory(directory);
}

QString SubversionControl::findTopLevelForDirectory(const QString &directory) const
{
    return m_plugin->findTopLevelForDirectory(directory);
}

bool SubversionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

void SubversionControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void SubversionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}
