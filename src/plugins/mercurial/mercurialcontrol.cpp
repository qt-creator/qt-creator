/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QDir>

using namespace Mercurial::Internal;

MercurialControl::MercurialControl(MercurialClient *client)
        :   mercurialClient(client)
{
}

QString MercurialControl::displayName() const
{
    return tr("Mercurial");
}

bool MercurialControl::managesDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return !mercurialClient->findTopLevelForFile(dir).isEmpty();
}

QString MercurialControl::findTopLevelForDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return mercurialClient->findTopLevelForFile(dir);
}

bool MercurialControl::supportsOperation(Operation operation) const
{
    bool supported = true;
    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
    case Core::IVersionControl::CreateRepositoryOperation:
    case Core::IVersionControl::AnnotateOperation:
        break;
    case Core::IVersionControl::OpenOperation:
    case Core::IVersionControl::SnapshotOperations:
        supported = false;
        break;
    }
    return supported;
}

bool MercurialControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool MercurialControl::vcsAdd(const QString &filename)
{
    const QFileInfo fi(filename);
    return mercurialClient->add(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return mercurialClient->remove(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsCreateRepository(const QString &directory)
{
    return mercurialClient->createRepositorySync(directory);
}

QString MercurialControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList MercurialControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool MercurialControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool MercurialControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool MercurialControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    mercurialClient->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

bool MercurialControl::sccManaged(const QString &filename)
{
    const QFileInfo fi(filename);
    const QString topLevel = findTopLevelForDirectory(fi.absolutePath());
    if (topLevel.isEmpty())
        return false;
    const QDir topLevelDir(topLevel);
    return mercurialClient->manifestSync(topLevel, topLevelDir.relativeFilePath(filename));
}

void MercurialControl::changed(const QVariant &v)
{
    switch (v.type()) {
    case QVariant::String:
        emit repositoryChanged(v.toString());
        break;
    case QVariant::StringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
}
