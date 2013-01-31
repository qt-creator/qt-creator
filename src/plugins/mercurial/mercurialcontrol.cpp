/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QFileInfo>
#include <QVariant>
#include <QStringList>
#include <QDir>

using namespace Mercurial::Internal;

MercurialControl::MercurialControl(MercurialClient *client)
    : mercurialClient(client)
{
}

QString MercurialControl::displayName() const
{
    return tr("Mercurial");
}

Core::Id MercurialControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_MERCURIAL);
}

bool MercurialControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = mercurialClient->findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool MercurialControl::isConfigured() const
{
    const QString binary = mercurialClient->settings()->binaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool MercurialControl::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();
    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
    case Core::IVersionControl::MoveOperation:
    case Core::IVersionControl::CreateRepositoryOperation:
    case Core::IVersionControl::AnnotateOperation:
    case Core::IVersionControl::CheckoutOperation:
    case Core::IVersionControl::GetRepositoryRootOperation:
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
    return mercurialClient->synchronousAdd(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return mercurialClient->synchronousRemove(fi.absolutePath(), fi.fileName());
}

bool MercurialControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return mercurialClient->synchronousMove(fromInfo.absolutePath(),
                                            fromInfo.absoluteFilePath(),
                                            toInfo.absoluteFilePath());
}

bool MercurialControl::vcsCreateRepository(const QString &directory)
{
    return mercurialClient->synchronousCreateRepository(directory);
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
    QString topLevel;
    const bool managed = managesDirectory(fi.absolutePath(), &topLevel);
    if (!managed || topLevel.isEmpty())
        return false;
    const QDir topLevelDir(topLevel);
    return mercurialClient->manifestSync(topLevel, topLevelDir.relativeFilePath(filename));
}

bool MercurialControl::vcsCheckout(const QString &directory, const QByteArray &url)
{
    return mercurialClient->synchronousClone(QString(), directory, QLatin1String(url));
}

QString MercurialControl::vcsGetRepositoryURL(const QString &directory)
{
    return mercurialClient->vcsGetRepositoryURL(directory);
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

void MercurialControl::emitConfigurationChanged()
{
    emit configurationChanged();
}
