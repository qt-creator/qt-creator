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

#include "subversioncontrol.h"

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversionplugin.h"
#include "subversionsettings.h"

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseclientsettings.h>

#include <utils/fileutils.h>

#include <QFileInfo>

namespace Subversion {
namespace Internal {

class SubversionTopicCache : public Core::IVersionControl::TopicCache
{
public:
    SubversionTopicCache(SubversionPlugin *plugin) :
        m_plugin(plugin)
    { }

protected:
    QString trackFile(const QString &repository)
    {
        return m_plugin->monitorFile(repository);
    }

    QString refreshTopic(const QString &repository)
    {
        return m_plugin->synchronousTopic(repository);
    }

private:
    SubversionPlugin *m_plugin;
};

SubversionControl::SubversionControl(SubversionPlugin *plugin) :
    Core::IVersionControl(new SubversionTopicCache(plugin)),
    m_plugin(plugin)
{ }

QString SubversionControl::displayName() const
{
    return QLatin1String("subversion");
}

Core::Id SubversionControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_SUBVERSION);
}

bool SubversionControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    return m_plugin->isVcsDirectory(fileName);
}

bool SubversionControl::isConfigured() const
{
    const Utils::FileName binary = m_plugin->client()->vcsBinary();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool SubversionControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        break;
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

bool SubversionControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_plugin->vcsMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool SubversionControl::vcsCreateRepository(const QString &)
{
    return false;
}

bool SubversionControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool SubversionControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_plugin->managesFile(workingDirectory, fileName);
}

bool SubversionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

Core::ShellCommand *SubversionControl::createInitialCheckoutCommand(const QString &url,
                                                                    const Utils::FileName &baseDirectory,
                                                                    const QString &localName,
                                                                    const QStringList &extraArgs)
{
    SubversionClient *client = m_plugin->client();

    QStringList args;
    args << QLatin1String("checkout");
    args << SubversionClient::addAuthenticationOptions(client->settings());
    args << QLatin1String(Subversion::Constants::NON_INTERACTIVE_OPTION);
    args << extraArgs << url << localName;

    auto command = new VcsBase::VcsCommand(baseDirectory.toString(), client->processEnvironment());
    command->addJob(client->vcsBinary(), args, -1);
    return command;
}

void SubversionControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void SubversionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

} // namespace Internal
} // namespace Subversion
