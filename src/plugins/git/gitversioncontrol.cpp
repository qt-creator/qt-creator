/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "gitversioncontrol.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "gitutils.h"

#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QDebug>
#include <QFileInfo>

static const char stashMessageKeywordC[] = "IVersionControl@";
static const char stashRevisionIdC[] = "revision";

namespace Git {
namespace Internal {

GitVersionControl::GitVersionControl(GitClient *client) :
    m_client(client)
{
}

QString GitVersionControl::displayName() const
{
    return QLatin1String("git");
}

Core::Id GitVersionControl::id() const
{
    return VcsBase::Constants::VCS_ID_GIT;
}

bool GitVersionControl::isConfigured() const
{
    bool ok = false;
    m_client->gitBinaryPath(&ok);
    return ok;
}

bool GitVersionControl::supportsOperation(Operation operation) const
{
    if (!isConfigured())
        return false;

    bool rc = false;
    switch (operation) {
    case AddOperation:
        rc = m_client->gitVersion() >= version(1, 6, 1);;
        break;
    case DeleteOperation:
        rc = true;
        break;
    case MoveOperation:
        rc = true;
        break;
    case OpenOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = true;
        break;
    case AnnotateOperation:
        rc = true;
        break;
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        rc = true;
        break;
    }
    return rc;
}

bool GitVersionControl::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitVersionControl::vcsAdd(const QString & fileName)
{
    // Implement in terms of using "--intent-to-add"
    QTC_ASSERT(m_client->gitVersion() >= version(1, 6, 1), return false);
    const QFileInfo fi(fileName);
    return m_client->synchronousAdd(fi.absolutePath(), true, QStringList(fi.fileName()));
}

bool GitVersionControl::vcsDelete(const QString & fileName)
{
    const QFileInfo fi(fileName);
    return m_client->synchronousDelete(fi.absolutePath(), true, QStringList(fi.fileName()));
}

bool GitVersionControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_client->synchronousMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool GitVersionControl::vcsCreateRepository(const QString &directory)
{
    return m_client->synchronousInit(directory);
}

bool GitVersionControl::vcsCheckout(const QString &directory, const QByteArray &url)
{
    return m_client->cloneRepository(directory,url);
}

QString GitVersionControl::vcsGetRepositoryURL(const QString &directory)
{
    return m_client->vcsGetRepositoryURL(directory);
}

/* Snapshots are implement using stashes, relying on stash messages for
 * naming as the actual stash names (stash{n}) are rotated as one adds stashes.
 * Note that the snapshot interface does not care whether we have an unmodified
 * repository state, in which case git refuses to stash.
 * In that case, return a special identifier as "specialprefix:<branch>:<head revision>",
 * which will trigger a checkout in restore(). */

QString GitVersionControl::vcsCreateSnapshot(const QString &topLevel)
{
    bool repositoryUnchanged;
    // Create unique keyword
    static int n = 1;
    QString keyword = QLatin1String(stashMessageKeywordC) + QString::number(n++);
    const QString stashMessage =
            m_client->synchronousStash(topLevel, keyword,
                                          GitClient::StashImmediateRestore|GitClient::StashIgnoreUnchanged,
                                          &repositoryUnchanged);
    if (!stashMessage.isEmpty())
        return stashMessage;
    if (repositoryUnchanged) {
        // For unchanged repository state: return identifier + top revision
        QString topRevision;
        QString branch;
        if (!m_client->synchronousTopRevision(topLevel, &topRevision, &branch))
            return QString();
        const QChar colon = QLatin1Char(':');
        QString id = QLatin1String(stashRevisionIdC);
        id += colon;
        id += branch;
        id += colon;
        id += topRevision;
        return id;
    }
    return QString(); // Failure
}

QStringList GitVersionControl::vcsSnapshots(const QString &topLevel)
{
    QList<Stash> stashes;
    if (!m_client->synchronousStashList(topLevel, &stashes))
        return QStringList();
    // Return the git stash 'message' as identifier, ignoring empty ones
    QStringList rc;
    foreach(const Stash &s, stashes)
        if (!s.message.isEmpty())
            rc.push_back(s.message);
    return rc;
}

bool GitVersionControl::vcsRestoreSnapshot(const QString &topLevel, const QString &name)
{
    bool success = false;
    do {
        // Is this a revision or a stash
        if (name.startsWith(QLatin1String(stashRevisionIdC))) {
            // Restore "id:branch:revision"
            const QStringList tokens = name.split(QLatin1Char(':'));
            if (tokens.size() != 3)
                break;
            const QString branch = tokens.at(1);
            const QString revision = tokens.at(2);
            success = m_client->synchronousReset(topLevel)
                      && m_client->synchronousCheckoutBranch(topLevel, branch)
                      && m_client->synchronousCheckoutFiles(topLevel, QStringList(), revision);
        } else {
            // Restore stash if it can be resolved.
            QString stashName;
            success = m_client->stashNameFromMessage(topLevel, name, &stashName)
                      && m_client->synchronousReset(topLevel)
                      && m_client->synchronousStashRestore(topLevel, stashName);
        }
    }  while (false);
    return success;
}

bool GitVersionControl::vcsRemoveSnapshot(const QString &topLevel, const QString &name)
{
    // Is this a revision -> happy
    if (name.startsWith(QLatin1String(stashRevisionIdC)))
        return true;
    QString stashName;
    return m_client->stashNameFromMessage(topLevel, name, &stashName)
            && m_client->synchronousStashRemove(topLevel, stashName);
}

bool GitVersionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    const QString topLevelFound = m_client->findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitVersionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_client->blame(fi.absolutePath(), QStringList(), fi.fileName(), QString(), line);
    return true;
}

void GitVersionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void GitVersionControl::emitRepositoryChanged(const QString &r)
{
    emit repositoryChanged(r);
}

void GitVersionControl::emitConfigurationChanged()
{
    emit configurationChanged();
}

} // Internal
} // Git
