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
    case MoveOperation:
    case OpenOperation:
    case AnnotateOperation:
        return true;
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        break;
    }
    return false;
}

bool PerforceVersionControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsOpen(fi.absolutePath(), fi.fileName());
}

Core::IVersionControl::SettingsFlags PerforceVersionControl::settingsFlags() const
{
    SettingsFlags rc;
    if (m_plugin->settings().autoOpen())
        rc|= AutoOpen;
    return rc;
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

bool PerforceVersionControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_plugin->vcsMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
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

bool PerforceVersionControl::vcsAnnotate(const QString &file, int line)
{
    m_plugin->vcsAnnotate(file, QString(), line);
    return true;
}

bool PerforceVersionControl::vcsCheckout(const QString &,const QByteArray &)
{
    return false;
}

QString PerforceVersionControl::vcsGetRepositoryURL(const QString &)
{
    return QString();
}

bool PerforceVersionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    const bool rc = m_plugin->managesDirectory(directory, topLevel);
    if (Perforce::Constants::debug) {
        QDebug nsp = qDebug().nospace();
        nsp << "managesDirectory" << directory << rc;
        if (topLevel)
            nsp << topLevel;
    }
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
