/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "vcsmanager.h"
#include "iversioncontrol.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QCoreApplication>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

enum { debug = 0 };

namespace Core {

typedef QList<IVersionControl *> VersionControlList;

static inline VersionControlList allVersionControls()
{
    return ExtensionSystem::PluginManager::instance()->getObjects<IVersionControl>();
}

// ---- VCSManagerPrivate
struct VCSManagerPrivate {
    QMap<QString, IVersionControl *> m_cachedMatches;
};

VCSManager::VCSManager() :
   m_d(new VCSManagerPrivate)
{
}

VCSManager::~VCSManager()
{
    delete m_d;
}

void VCSManager::setVCSEnabled(const QString &directory)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << directory;
    IVersionControl* managingVCS = findVersionControlForDirectory(directory);
    const VersionControlList versionControls = allVersionControls();
    foreach (IVersionControl *versionControl, versionControls) {
        const bool newEnabled = versionControl == managingVCS;
        if (newEnabled != versionControl->isEnabled())
            versionControl->setEnabled(newEnabled);
    }
}

void VCSManager::setAllVCSEnabled()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    const VersionControlList versionControls = allVersionControls();
    foreach (IVersionControl *versionControl, versionControls)
        if (!versionControl->isEnabled())
            versionControl->setEnabled(true);
}

IVersionControl* VCSManager::findVersionControlForDirectory(const QString &directory)
{
    // first look into the cache, check the whole name

    {
        const QMap<QString, IVersionControl *>::const_iterator it = m_d->m_cachedMatches.constFind(directory);
        if (it != m_d->m_cachedMatches.constEnd())
            return it.value();
    }

    int pos = 0;
    const QChar slash = QLatin1Char('/');
    while (true) {
        int index = directory.indexOf(slash, pos);
        if (index == -1)
            break;
        const QString directoryPart = directory.left(index);
        QMap<QString, IVersionControl *>::const_iterator it = m_d->m_cachedMatches.constFind(directoryPart);
        if (it != m_d->m_cachedMatches.constEnd())
            return it.value();
        pos = index+1;
    }

    // ah nothing so ask the IVersionControls directly
    const VersionControlList versionControls = allVersionControls();
    foreach (IVersionControl * versionControl, versionControls) {
        if (versionControl->managesDirectory(directory)) {
            m_d->m_cachedMatches.insert(versionControl->findTopLevelForDirectory(directory), versionControl);
            return versionControl;
        }
    }
    return 0;
}

bool VCSManager::showDeleteDialog(const QString &fileName)
{
    IVersionControl *vc = findVersionControlForDirectory(QFileInfo(fileName).absolutePath());
    if (!vc || !vc->supportsOperation(IVersionControl::DeleteOperation))
        return true;
    const QString title = QCoreApplication::translate("VCSManager", "Version Control");
    const QString msg = QCoreApplication::translate("VCSManager",
                                                    "Would you like to remove this file from the version control system (%1)?\n"
                                                    "Note: This might remove the local file.").arg(vc->name());
    const QMessageBox::StandardButton button =
        QMessageBox::question(0, title, msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (button != QMessageBox::Yes)
        return true;
    return vc->vcsDelete(fileName);
}

} // namespace Core
