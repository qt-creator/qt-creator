/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "vcsmanager.h"
#include "iversioncontrol.h"
#include "icore.h"
#include "filemanager.h"

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
typedef QMap<QString, IVersionControl *> VersionControlCache;

static inline VersionControlList allVersionControls()
{
    return ExtensionSystem::PluginManager::instance()->getObjects<IVersionControl>();
}

// ---- VCSManagerPrivate:
// Maintains a cache of top-level directory->version control.

struct VCSManagerPrivate {
    VersionControlCache m_cachedMatches;
};

VCSManager::VCSManager(QObject *parent) :
   QObject(parent),
   m_d(new VCSManagerPrivate)
{
}

VCSManager::~VCSManager()
{
    delete m_d;
}

void VCSManager::extensionsInitialized()
{
    // Change signal connections
    FileManager *fileManager = ICore::instance()->fileManager();
    foreach (IVersionControl *versionControl, allVersionControls()) {
        connect(versionControl, SIGNAL(filesChanged(QStringList)),
                fileManager, SIGNAL(filesChangedInternally(QStringList)));
        connect(versionControl, SIGNAL(repositoryChanged(QString)),
                this, SIGNAL(repositoryChanged(QString)));
    }
}

IVersionControl* VCSManager::findVersionControlForDirectory(const QString &directory,
                                                            QString *topLevelDirectory)
{
    typedef VersionControlCache::const_iterator VersionControlCacheConstIterator;
    const VersionControlCacheConstIterator cacheEnd = m_d->m_cachedMatches.constEnd();

    if (topLevelDirectory)
        topLevelDirectory->clear();

    // First check if the directory has an entry, meaning it is a top level
    const VersionControlCacheConstIterator fullPathIt = m_d->m_cachedMatches.constFind(directory);
    if (fullPathIt != cacheEnd) {
        if (topLevelDirectory)
            *topLevelDirectory = directory;
        return fullPathIt.value();
    }

    // Split the path, starting from top, try to find the matching repository
    int pos = 0;
    const QChar slash = QLatin1Char('/');
    while (true) {
        const int index = directory.indexOf(slash, pos);
        if (index == -1)
            break;
        const QString directoryPart = directory.left(index);
        const VersionControlCacheConstIterator it = m_d->m_cachedMatches.constFind(directoryPart);
        if (it != cacheEnd) {
            if (topLevelDirectory)
                *topLevelDirectory = it.key();
            return it.value();
        }
        pos = index + 1;
    }

    // Nothing: ask the IVersionControls directly, insert the toplevel into the cache.
    const VersionControlList versionControls = allVersionControls();
    foreach (IVersionControl * versionControl, versionControls) {
        if (versionControl->managesDirectory(directory)) {
            const QString topLevel = versionControl->findTopLevelForDirectory(directory);
            m_d->m_cachedMatches.insert(topLevel, versionControl);
            if (topLevelDirectory)
                *topLevelDirectory = topLevel;
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

CORE_EXPORT QDebug operator<<(QDebug in, const VCSManager &v)
{
    QDebug nospace = in.nospace();
    const VersionControlCache::const_iterator cend = v.m_d->m_cachedMatches.constEnd();
    for (VersionControlCache::const_iterator it = v.m_d->m_cachedMatches.constBegin(); it != cend; ++it)
        nospace << "Directory: " << it.key() << ' ' << it.value()->name() << '\n';
    nospace << '\n';
    return in;
}

} // namespace Core
