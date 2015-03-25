/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gitversioncontrol.h"
#include "gitclient.h"
#include "gitutils.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QFileInfo>

namespace Git {
namespace Internal {

class GitTopicCache : public Core::IVersionControl::TopicCache
{
public:
    GitTopicCache(GitClient *client) :
        m_client(client)
    {
    }

protected:
    QString trackFile(const QString &repository)
    {
        const QString gitDir = m_client->findGitDirForRepository(repository);
        return gitDir.isEmpty() ? QString() : (gitDir + QLatin1String("/HEAD"));
    }

    QString refreshTopic(const QString &repository)
    {
        return m_client->synchronousTopic(repository);
    }

private:
    GitClient *m_client;
};

GitVersionControl::GitVersionControl(GitClient *client) :
    Core::IVersionControl(new GitTopicCache(client)),
    m_client(client)
{
}

QString GitVersionControl::displayName() const
{
    return QLatin1String("Git");
}

Core::Id GitVersionControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_GIT);
}

bool GitVersionControl::isConfigured() const
{
    bool ok = false;
    m_client->vcsBinary(&ok);
    return ok;
}

bool GitVersionControl::supportsOperation(Operation operation) const
{
    if (!isConfigured())
        return false;

    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case AnnotateOperation:
        return true;
    }
    return false;
}

bool GitVersionControl::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitVersionControl::vcsAdd(const QString & fileName)
{
    const QFileInfo fi(fileName);
    return m_client->synchronousAdd(fi.absolutePath(), QStringList(fi.fileName()));
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

QString GitVersionControl::vcsTopic(const QString &directory)
{
    QString topic = Core::IVersionControl::vcsTopic(directory);
    const QString commandInProgress = m_client->commandInProgressDescription(directory);
    if (!commandInProgress.isEmpty())
        topic += QLatin1String(" (") + commandInProgress + QLatin1Char(')');
    return topic;
}

QStringList GitVersionControl::additionalToolsPath() const
{
    QStringList res = m_client->settings()->searchPathList();
    const QString binaryPath = m_client->gitBinDirectory().toString();
    if (!binaryPath.isEmpty() && !res.contains(binaryPath))
        res << binaryPath;
    return res;
}

bool GitVersionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    const QString topLevelFound = m_client->findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitVersionControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_client->managesFile(workingDirectory, fileName);
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
