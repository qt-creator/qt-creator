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

#include "cvscontrol.h"

#include "cvsclient.h"
#include "cvsplugin.h"
#include "cvssettings.h"

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcscommand.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace Cvs;
using namespace Cvs::Internal;

CvsControl::CvsControl(CvsPlugin *plugin) :
    m_plugin(plugin)
{ }

QString CvsControl::displayName() const
{
    return QLatin1String("cvs");
}

Core::Id CvsControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_CVS);
}

bool CvsControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    return fileName.toFileInfo().isDir()
            && !fileName.fileName().compare("CVS", Utils::HostOsInfo::fileNameCaseSensitivity());
}

bool CvsControl::isConfigured() const
{
    const Utils::FileName binary = m_plugin->client()->vcsBinary();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool CvsControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        break;
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

Core::IVersionControl::OpenSupportMode CvsControl::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName);
    return OpenOptional;
}

bool CvsControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->edit(fi.absolutePath(), QStringList(fi.fileName()));
}

bool CvsControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool CvsControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool CvsControl::vcsMove(const QString &from, const QString &to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return false;
}

bool CvsControl::vcsCreateRepository(const QString &)
{
    return false;
}

bool CvsControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

QString CvsControl::vcsOpenText() const
{
    return tr("&Edit");
}

Core::ShellCommand *CvsControl::createInitialCheckoutCommand(const QString &url,
                                                             const Utils::FileName &baseDirectory,
                                                             const QString &localName,
                                                             const QStringList &extraArgs)
{
    QTC_ASSERT(localName == url, return 0);

    const CvsSettings settings = CvsPlugin::instance()->client()->settings();

    QStringList args;
    args << QLatin1String("checkout") << url << extraArgs;

    auto command = new VcsBase::VcsCommand(baseDirectory.toString(),
                                           QProcessEnvironment::systemEnvironment());
    command->setDisplayName(tr("CVS Checkout"));
    command->addJob(m_plugin->client()->vcsBinary(), settings.addOptions(args), -1);
    return command;
}

bool CvsControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool CvsControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_plugin->managesFile(workingDirectory, fileName);
}

void CvsControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void CvsControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}
