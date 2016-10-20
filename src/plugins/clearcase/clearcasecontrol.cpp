/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcasecontrol.h"
#include "clearcaseplugin.h"
#include "clearcaseconstants.h"

#include <vcsbase/vcsbaseconstants.h>
#include <utils/synchronousprocess.h>

#include <QFileInfo>

using namespace ClearCase;
using namespace ClearCase::Internal;

ClearCaseControl::ClearCaseControl(ClearCasePlugin *plugin) : m_plugin(plugin)
{ }

QString ClearCaseControl::displayName() const
{
    return QLatin1String("ClearCase");
}

Core::Id ClearCaseControl::id() const
{
    return Constants::VCS_ID_CLEARCASE;
}

bool ClearCaseControl::isVcsFileOrDirectory(const Utils::FileName &fileName) const
{
    Q_UNUSED(fileName);
    return false; // ClearCase has no files/directories littering the sources
}

bool ClearCaseControl::isConfigured() const
{
#ifdef WITH_TESTS
    if (m_plugin->isFakeCleartool())
        return true;
#endif
    const QString binary = m_plugin->settings().ccBinaryPath;
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool ClearCaseControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case Core::IVersionControl::InitialCheckoutOperation:
        rc = false;
        break;
    }
    return rc;
}

Core::IVersionControl::OpenSupportMode ClearCaseControl::openSupportMode(const QString &fileName) const
{
    if (m_plugin->isDynamic()) {
        // NB! Has to use managesFile() and not vcsStatus() since the index can only be guaranteed
        // to be up to date if the file has been explicitly opened, which is not the case when
        // doing a search and replace as a part of a refactoring.
        if (m_plugin->managesFile(QFileInfo(fileName).absolutePath(), fileName)) {
            // Checkout is the only option for managed files in dynamic views
            return IVersionControl::OpenMandatory;
        } else {
            // Not managed files can be edited without noticing the VCS
            return IVersionControl::NoOpen;
        }

    } else {
        return IVersionControl::OpenOptional; // Snapshot views supports Hijack and check out
    }
}

bool ClearCaseControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsOpen(fi.absolutePath(), fi.fileName());
}

Core::IVersionControl::SettingsFlags ClearCaseControl::settingsFlags() const
{
    SettingsFlags rc;
    if (m_plugin->settings().autoCheckOut)
        rc|= AutoOpen;
    return rc;
}

bool ClearCaseControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool ClearCaseControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool ClearCaseControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo ifrom(from);
    const QFileInfo ito(to);
    return m_plugin->vcsMove(ifrom.absolutePath(), ifrom.fileName(), ito.fileName());
}

bool ClearCaseControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool ClearCaseControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_plugin->managesFile(workingDirectory, fileName);
}

bool ClearCaseControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

QString ClearCaseControl::vcsOpenText() const
{
    return tr("Check &Out");
}

QString ClearCaseControl::vcsMakeWritableText() const
{
    if (m_plugin->isDynamic())
        return QString();
    return tr("&Hijack");
}

QString ClearCaseControl::vcsTopic(const QString &directory)
{
    return m_plugin->ccGetView(directory).name;
}

void ClearCaseControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void ClearCaseControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void ClearCaseControl::emitConfigurationChanged()
{
    emit configurationChanged();
}

bool ClearCaseControl::vcsCreateRepository(const QString &)
{
    return false;
}
