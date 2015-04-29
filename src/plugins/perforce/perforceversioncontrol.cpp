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

#include "perforceversioncontrol.h"
#include "perforceplugin.h"
#include "perforceconstants.h"
#include "perforcesettings.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QFileInfo>
#include <QDebug>

namespace Perforce {
namespace Internal {

PerforceVersionControl::PerforceVersionControl(PerforcePlugin *plugin) :
    m_plugin(plugin)
{
}

QString PerforceVersionControl::displayName() const
{
    return QLatin1String("perforce");
}

Core::Id PerforceVersionControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_PERFORCE);
}

bool PerforceVersionControl::isConfigured() const
{
    const QString binary = m_plugin->settings().p4BinaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool PerforceVersionControl::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
        return supported;
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case InitialCheckoutOperation:
        break;
    }
    return false;
}

Core::IVersionControl::OpenSupportMode PerforceVersionControl::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName);
    return OpenOptional;
}

bool PerforceVersionControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsOpen(fi.absolutePath(), fi.fileName(), true);
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

bool PerforceVersionControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

QString PerforceVersionControl::vcsOpenText() const
{
    return tr("&Edit");
}

QString PerforceVersionControl::vcsMakeWritableText() const
{
    return tr("&Hijack");
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

bool PerforceVersionControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_plugin->managesFile(workingDirectory, fileName);
}

void PerforceVersionControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void PerforceVersionControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void PerforceVersionControl::emitConfigurationChanged()
{
    emit configurationChanged();
}

} // Internal
} // Perforce
