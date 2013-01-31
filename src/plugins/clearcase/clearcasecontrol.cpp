/**************************************************************************
**
** Copyright (c) 2013 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcasecontrol.h"
#include "clearcaseplugin.h"
#include "clearcaseconstants.h"

#include <vcsbase/vcsbaseconstants.h>
#include <utils/synchronousprocess.h>

#include <QFileInfo>

using namespace ClearCase;
using namespace ClearCase::Internal;

ClearCaseControl::ClearCaseControl(ClearCasePlugin *plugin) :
    m_plugin(plugin)
{
}

QString ClearCaseControl::displayName() const
{
    return QLatin1String("ClearCase");
}

Core::Id ClearCaseControl::id() const
{
    return Core::Id(ClearCase::Constants::VCS_ID_CLEARCASE);
}

bool ClearCaseControl::isConfigured() const
{
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
    case OpenOperation:
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
    case GetRepositoryRootOperation:
        break;
    case CheckoutOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
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

QString ClearCaseControl::vcsGetRepositoryURL(const QString &directory)
{
    return m_plugin->vcsGetRepositoryURL(directory);
}

bool ClearCaseControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool ClearCaseControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

QString ClearCaseControl::vcsOpenText() const
{
    return tr("&Check Out");
}

QString ClearCaseControl::vcsMakeWritableText() const
{
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

bool ClearCaseControl::vcsCheckout(const QString & /*directory*/, const QByteArray & /*url*/)
{
    return false;
}

bool ClearCaseControl::vcsCreateRepository(const QString &)
{
    return false;
}

QString ClearCaseControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList ClearCaseControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool ClearCaseControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool ClearCaseControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}
