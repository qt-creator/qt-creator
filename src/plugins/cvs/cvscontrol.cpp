/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cvscontrol.h"
#include "cvsplugin.h"
#include "cvssettings.h"

#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QFileInfo>

using namespace CVS;
using namespace CVS::Internal;

CVSControl::CVSControl(CVSPlugin *plugin) :
    m_plugin(plugin)
{
}

QString CVSControl::displayName() const
{
    return QLatin1String("cvs");
}

QString CVSControl::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_CVS);
}

bool CVSControl::isConfigured() const
{
    const QString binary = m_plugin->settings().cvsCommand;
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool CVSControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case AnnotateOperation:
    case OpenOperation:
        break;
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        rc = false;
        break;
    }
    return rc;
}

bool CVSControl::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->edit(fi.absolutePath(), QStringList(fi.fileName()));
}

bool CVSControl::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsAdd(fi.absolutePath(), fi.fileName());
}

bool CVSControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName());
}

bool CVSControl::vcsMove(const QString &from, const QString &to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return false;
}

bool CVSControl::vcsCreateRepository(const QString &)
{
    return false;
}

QString CVSControl::vcsGetRepositoryURL(const QString &)
{
    return QString();
}

bool CVSControl::vcsCheckout(const QString &, const QByteArray &)
{
    return false;
}

QString CVSControl::vcsCreateSnapshot(const QString &)
{
    return QString();
}

QStringList CVSControl::vcsSnapshots(const QString &)
{
    return QStringList();
}

bool CVSControl::vcsRestoreSnapshot(const QString &, const QString &)
{
    return false;
}

bool CVSControl::vcsRemoveSnapshot(const QString &, const QString &)
{
    return false;
}

bool CVSControl::vcsAnnotate(const QString &file, int line)
{
    m_plugin->vcsAnnotate(file, QString(), line);
    return true;
}

bool CVSControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

void CVSControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void CVSControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void CVSControl::emitConfigurationChanged()
{
    emit configurationChanged();
}
