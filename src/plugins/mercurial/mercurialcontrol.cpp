/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcscommand.h>

#include <coreplugin/vcsmanager.h>

#include <utils/fileutils.h>

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QVariant>
#include <QStringList>
#include <QDir>

namespace Mercurial {
namespace Internal {

class MercurialTopicCache : public Core::IVersionControl::TopicCache
{
public:
    MercurialTopicCache(MercurialClient *client) : m_client(client) {}

protected:
    QString trackFile(const QString &repository)
    {
        return repository + QLatin1String("/.hg/branch");
    }

    QString refreshTopic(const QString &repository)
    {
        return m_client->branchQuerySync(repository);
    }

private:
    MercurialClient *m_client;
};

MercurialControl::MercurialControl(MercurialClient *client) :
    Core::IVersionControl(new MercurialTopicCache(client)),
    mercurialClient(client)
{ }

QString MercurialControl::displayName() const
{
    return tr("Mercurial");
}

Core::Id MercurialControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_MERCURIAL);
}

bool MercurialControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    return mercurialClient->isVcsDirectory(fileName);
}

bool MercurialControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = mercurialClient->findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool MercurialControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return mercurialClient->managesFile(workingDirectory, fileName);
}

bool MercurialControl::isConfigured() const
{
    const Utils::FileName binary = mercurialClient->vcsBinary();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
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
    case Core::IVersionControl::InitialCheckoutOperation:
        break;
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

bool MercurialControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    mercurialClient->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

Core::ShellCommand *MercurialControl::createInitialCheckoutCommand(const QString &url,
                                                                   const Utils::FileName &baseDirectory,
                                                                   const QString &localName,
                                                                   const QStringList &extraArgs)
{
    QStringList args;
    args << QLatin1String("clone") << extraArgs << url << localName;
    auto command = new VcsBase::VcsCommand(baseDirectory.toString(),
                                           mercurialClient->processEnvironment());
    command->addJob(mercurialClient->vcsBinary(), args, -1);
    return command;
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

} // namespace Internal
} // namespace Mercurial
