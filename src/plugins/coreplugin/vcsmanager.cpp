/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "vcsmanager.h"
#include "iversioncontrol.h"
#include "icore.h"
#include "filemanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

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

class VcsManagerPrivate
{
public:
    VersionControlCache m_cachedMatches;
};

VcsManager::VcsManager(QObject *parent) :
   QObject(parent),
   m_d(new VcsManagerPrivate)
{
}

VcsManager::~VcsManager()
{
    delete m_d;
}

void VcsManager::extensionsInitialized()
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

static bool longerThanPath(QPair<QString, IVersionControl *> &pair1, QPair<QString, IVersionControl *> &pair2)
{
    return pair1.first.size() > pair2.first.size();
}

IVersionControl* VcsManager::findVersionControlForDirectory(const QString &directory,
                                                            QString *topLevelDirectory)
{
    typedef VersionControlCache::const_iterator VersionControlCacheConstIterator;

    if (debug) {
        qDebug(">findVersionControlForDirectory %s topLevelPtr %d",
               qPrintable(directory), (topLevelDirectory != 0));
        if (debug > 1) {
            const VersionControlCacheConstIterator cend = m_d->m_cachedMatches.constEnd();
            for (VersionControlCacheConstIterator it = m_d->m_cachedMatches.constBegin(); it != cend; ++it)
                qDebug("Cache %s -> '%s'", qPrintable(it.key()), qPrintable(it.value()->displayName()));
        }
    }
    QTC_ASSERT(!directory.isEmpty(), return 0);

    const VersionControlCacheConstIterator cacheEnd = m_d->m_cachedMatches.constEnd();

    if (topLevelDirectory)
        topLevelDirectory->clear();

    // First check if the directory has an entry, meaning it is a top level
    const VersionControlCacheConstIterator fullPathIt = m_d->m_cachedMatches.constFind(directory);
    if (fullPathIt != cacheEnd) {
        if (topLevelDirectory)
            *topLevelDirectory = directory;
        if (debug)
            qDebug("<findVersionControlForDirectory: full cache match for VCS '%s'", qPrintable(fullPathIt.value()->displayName()));
        return fullPathIt.value();
    }

    // Split the path, trying to find the matching repository. We start from the reverse
    // in order to detected nested repositories correctly (say, a git checkout under SVN).
    // Note that detection of a nested version control will still fail if the
    // above-located version control is detected and entered into the cache first.
    // The nested one can then no longer be found due to the splitting of the paths.
    int pos = directory.size() - 1;
    const QChar slash = QLatin1Char('/');
    while (true) {
        const int index = directory.lastIndexOf(slash, pos);
        if (index <= 0) // Stop at '/' or not found
            break;
        const QString directoryPart = directory.left(index);
        const VersionControlCacheConstIterator it = m_d->m_cachedMatches.constFind(directoryPart);
        if (it != cacheEnd) {
            if (topLevelDirectory)
                *topLevelDirectory = it.key();
            if (debug)
                qDebug("<findVersionControlForDirectory: cache match for VCS '%s', topLevel: %s",
                       qPrintable(it.value()->displayName()), qPrintable(it.key()));
            return it.value();
        }
        pos = index - 1;
    }

    // Nothing: ask the IVersionControls directly, insert the toplevel into the cache.
    const VersionControlList versionControls = allVersionControls();
    QList<QPair<QString, IVersionControl *> > allThatCanManage;

    foreach (IVersionControl * versionControl, versionControls) {
        QString topLevel;
        if (versionControl->managesDirectory(directory, &topLevel)) {
            if (debug)
                qDebug("<findVersionControlForDirectory: %s manages %s",
                       qPrintable(versionControl->displayName()),
                       qPrintable(topLevel));
            allThatCanManage.push_back(qMakePair(topLevel, versionControl));
        }
    }

    // To properly find a nested repository (say, git checkout inside SVN),
    // we need to select the version control with the longest toplevel pathname.
    qSort(allThatCanManage.begin(), allThatCanManage.end(), longerThanPath);

    if (!allThatCanManage.isEmpty()) {
        QString toplevel = allThatCanManage.first().first;
        IVersionControl *versionControl = allThatCanManage.first().second;
        m_d->m_cachedMatches.insert(toplevel, versionControl);
        if (topLevelDirectory)
            *topLevelDirectory = toplevel;
        if (debug)
            qDebug("<findVersionControlForDirectory: invocation of '%s' matches: %s",
                   qPrintable(versionControl->displayName()), qPrintable(toplevel));
        return versionControl;
    }
    if (debug)
        qDebug("<findVersionControlForDirectory: No match for %s", qPrintable(directory));
    return 0;
}

bool VcsManager::promptToDelete(const QString &fileName)
{
    if (IVersionControl *vc = findVersionControlForDirectory(QFileInfo(fileName).absolutePath()))
        return promptToDelete(vc, fileName);
    return true;
}

IVersionControl *VcsManager::checkout(const QString &versionControlType,
                                      const QString &directory,
                                      const QByteArray &url)
{
    foreach (IVersionControl *versionControl, allVersionControls()) {
        if (versionControl->displayName() == versionControlType
            && versionControl->supportsOperation(Core::IVersionControl::CheckoutOperation)) {
            if (versionControl->vcsCheckout(directory, url)) {
                m_d->m_cachedMatches.insert(directory, versionControl);
                return versionControl;
            }
            return 0;
        }
    }
    return 0;
}

bool VcsManager::findVersionControl(const QString &versionControlType)
{
    foreach (IVersionControl * versionControl, allVersionControls()) {
        if (versionControl->displayName() == versionControlType)
            return true;
    }
    return false;
}

QString VcsManager::repositoryUrl(const QString &directory)
{
    IVersionControl *vc = findVersionControlForDirectory(directory);

    if (vc && vc->supportsOperation(Core::IVersionControl::GetRepositoryRootOperation))
       return vc->vcsGetRepositoryURL(directory);
    return QString();
}

bool VcsManager::promptToDelete(IVersionControl *vc, const QString &fileName)
{
    QTC_ASSERT(vc, return true)
    if (!vc->supportsOperation(IVersionControl::DeleteOperation))
        return true;
    const QString title = tr("Version Control");
    const QString msg = tr("Would you like to remove this file from the version control system (%1)?\n"
                           "Note: This might remove the local file.").arg(vc->displayName());
    const QMessageBox::StandardButton button =
        QMessageBox::question(0, title, msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (button != QMessageBox::Yes)
        return true;
    return vc->vcsDelete(fileName);
}

CORE_EXPORT QDebug operator<<(QDebug in, const VcsManager &v)
{
    QDebug nospace = in.nospace();
    const VersionControlCache::const_iterator cend = v.m_d->m_cachedMatches.constEnd();
    for (VersionControlCache::const_iterator it = v.m_d->m_cachedMatches.constBegin(); it != cend; ++it)
        nospace << "Directory: " << it.key() << ' ' << it.value()->displayName() << '\n';
    nospace << '\n';
    return in;
}

} // namespace Core
