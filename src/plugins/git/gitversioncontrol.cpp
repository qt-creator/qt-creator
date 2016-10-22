/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gitversioncontrol.h"
#include "gitclient.h"
#include "gitutils.h"

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcscommand.h>

#include <utils/hostosinfo.h>

#include <QFileInfo>
#include <QProcessEnvironment>

namespace Git {
namespace Internal {

class GitTopicCache : public Core::IVersionControl::TopicCache
{
public:
    GitTopicCache(GitClient *client) :
        m_client(client)
    { }

protected:
    QString trackFile(const QString &repository) override
    {
        const QString gitDir = m_client->findGitDirForRepository(repository);
        return gitDir.isEmpty() ? QString() : (gitDir + "/HEAD");
    }

    QString refreshTopic(const QString &repository) override
    {
        return m_client->synchronousTopic(repository);
    }

private:
    GitClient *m_client;
};

GitVersionControl::GitVersionControl(GitClient *client) :
    Core::IVersionControl(new GitTopicCache(client)),
    m_client(client)
{ }

QString GitVersionControl::displayName() const
{
    return QLatin1String("Git");
}

Core::Id GitVersionControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_GIT);
}

bool GitVersionControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    return fileName.toFileInfo().isDir()
            && !fileName.fileName().compare(".git", Utils::HostOsInfo::fileNameCaseSensitivity());
}

bool GitVersionControl::isConfigured() const
{
    return !m_client->vcsBinary().isEmpty();
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
    case InitialCheckoutOperation:
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
    return m_client->synchronousAdd(fi.absolutePath(), { fi.fileName() });
}

bool GitVersionControl::vcsDelete(const QString & fileName)
{
    const QFileInfo fi(fileName);
    return m_client->synchronousDelete(fi.absolutePath(), true, { fi.fileName() });
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
        topic += " (" + commandInProgress + ')';
    return topic;
}

Core::ShellCommand *GitVersionControl::createInitialCheckoutCommand(const QString &url,
                                                                    const Utils::FileName &baseDirectory,
                                                                    const QString &localName,
                                                                    const QStringList &extraArgs)
{
    QStringList args = { "clone", "--progress" };
    args << extraArgs << url << localName;

    auto command = new VcsBase::VcsCommand(baseDirectory.toString(), m_client->processEnvironment());
    command->addFlags(VcsBase::VcsCommand::SuppressStdErr);
    command->addJob(m_client->vcsBinary(), args, -1);
    return command;
}

QStringList GitVersionControl::additionalToolsPath() const
{
    QStringList res = m_client->settings().searchPathList();
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
    m_client->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
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

} // Internal
} // Git
