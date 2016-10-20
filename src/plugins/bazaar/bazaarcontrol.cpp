/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaarcontrol.h"
#include "bazaarclient.h"
#include "bazaarplugin.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcscommand.h>

#include <utils/fileutils.h>

#include <QFileInfo>
#include <QVariant>
#include <QStringList>

using namespace Bazaar::Internal;

BazaarControl::BazaarControl(BazaarClient *client) : m_bazaarClient(client)
{ }

QString BazaarControl::displayName() const
{
    return tr("Bazaar");
}

Core::Id BazaarControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_BAZAAR);
}

bool BazaarControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    return m_bazaarClient->isVcsDirectory(fileName);
}

bool BazaarControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = m_bazaarClient->findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool BazaarControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_bazaarClient->managesFile(workingDirectory, fileName);
}

bool BazaarControl::isConfigured() const
{
    const Utils::FileName binary = m_bazaarClient->vcsBinary();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool BazaarControl::supportsOperation(Operation operation) const
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

bool BazaarControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool BazaarControl::vcsAdd(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_bazaarClient->synchronousAdd(fi.absolutePath(), fi.fileName());
}

bool BazaarControl::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_bazaarClient->synchronousRemove(fi.absolutePath(), fi.fileName());
}

bool BazaarControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_bazaarClient->synchronousMove(fromInfo.absolutePath(),
                                           fromInfo.absoluteFilePath(),
                                           toInfo.absoluteFilePath());
}

bool BazaarControl::vcsCreateRepository(const QString &directory)
{
    return m_bazaarClient->synchronousCreateRepository(directory);
}

bool BazaarControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_bazaarClient->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

Core::ShellCommand *BazaarControl::createInitialCheckoutCommand(const QString &url,
                                                                const Utils::FileName &baseDirectory,
                                                                const QString &localName,
                                                                const QStringList &extraArgs)
{
    QStringList args;
    args << m_bazaarClient->vcsCommandString(BazaarClient::CloneCommand)
         << extraArgs << url << localName;

    QProcessEnvironment env = m_bazaarClient->processEnvironment();
    env.insert(QLatin1String("BZR_PROGRESS_BAR"), QLatin1String("text"));
    auto command = new VcsBase::VcsCommand(baseDirectory.toString(), env);
    command->addJob(m_bazaarClient->vcsBinary(), args, -1);
    return command;
}

void BazaarControl::changed(const QVariant &v)
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
