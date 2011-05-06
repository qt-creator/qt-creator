/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsmanager.h"
#include "iversioncontrol.h"
#include "icore.h"
#include "filemanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QCoreApplication>

#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

namespace Core {

typedef QList<IVersionControl *> VersionControlList;

static inline VersionControlList allVersionControls()
{
    return ExtensionSystem::PluginManager::instance()->getObjects<IVersionControl>();
}

static const QChar SLASH('/');

// ---- VCSManagerPrivate:
// Maintains a cache of top-level directory->version control.

class VcsManagerPrivate
{
public:
    class VcsInfo {
    public:
        VcsInfo(IVersionControl *vc, const QString &tl) :
            versionControl(vc), topLevel(tl)
        { }

        bool operator == (const VcsInfo &other) const
        {
            return versionControl == other.versionControl &&
                    topLevel == other.topLevel;
        }

        IVersionControl *versionControl;
        QString topLevel;
    };

    ~VcsManagerPrivate()
    {
        qDeleteAll(m_vcsInfoList);
    }

    VcsInfo *findInCache(const QString &directory)
    {
        const QMap<QString, VcsInfo *>::const_iterator it = m_cachedMatches.constFind(directory);
        if (it != m_cachedMatches.constEnd())
            return it.value();
        return 0;
    }

    VcsInfo *findUpInCache(const QString &directory)
    {
        VcsInfo *result = 0;

        // Split the path, trying to find the matching repository. We start from the reverse
        // in order to detected nested repositories correctly (say, a git checkout under SVN).
        for (int pos = directory.size() - 1; pos >= 0; pos = directory.lastIndexOf(SLASH, pos) - 1) {
            const QString directoryPart = directory.left(pos);
            result = findInCache(directoryPart);
            if (result != 0)
                break;
        }
        return result;
    }

    void cache(IVersionControl *vc, const QString topLevel, const QString directory)
    {
        Q_ASSERT(directory.startsWith(topLevel));

        VcsInfo *newInfo = new VcsInfo(vc, topLevel);
        bool createdNewInfo(true);
        // Do we have a matching VcsInfo already?
        foreach(VcsInfo *i, m_vcsInfoList) {
            if (*i == *newInfo) {
                delete newInfo;
                newInfo = i;
                createdNewInfo = false;
                break;
            }
        }
        if (createdNewInfo)
            m_vcsInfoList.append(newInfo);

        QString tmpDir = directory;
        while (tmpDir.count() >= topLevel.count() && tmpDir.count() > 0) {
            m_cachedMatches.insert(tmpDir, newInfo);
            int slashPos = tmpDir.lastIndexOf(SLASH);
            tmpDir = slashPos >= 0 ? tmpDir.left(tmpDir.lastIndexOf(SLASH)) : QString();
        }
    }

    QMap<QString, VcsInfo *> m_cachedMatches;
    QList<VcsInfo *> m_vcsInfoList;
};

VcsManager::VcsManager(QObject *parent) :
   QObject(parent),
   m_d(new VcsManagerPrivate)
{
}

// ---- VCSManager:

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

IVersionControl* VcsManager::findVersionControlForDirectory(const QString &inputDirectory,
                                                            QString *topLevelDirectory)
{
    if (inputDirectory.isEmpty())
        return 0;

    // Make sure we a clean absolute path:
    const QString directory = QDir(inputDirectory).absolutePath();

    VcsManagerPrivate::VcsInfo *cachedData = m_d->findInCache(directory);
    if (cachedData) {
        if (topLevelDirectory)
            *topLevelDirectory = cachedData->topLevel;
        return cachedData->versionControl;
    }

    // Nothing: ask the IVersionControls directly.
    const VersionControlList versionControls = allVersionControls();
    QList<QPair<QString, IVersionControl *> > allThatCanManage;

    foreach (IVersionControl * versionControl, versionControls) {
        QString topLevel;
        if (versionControl->managesDirectory(directory, &topLevel))
            allThatCanManage.push_back(qMakePair(topLevel, versionControl));
    }

    // To properly find a nested repository (say, git checkout inside SVN),
    // we need to select the version control with the longest toplevel pathname.
    qSort(allThatCanManage.begin(), allThatCanManage.end(), longerThanPath);

    if (allThatCanManage.isEmpty()) {
        m_d->cache(0, QString(), directory); // register that nothing was found!

        // report result;
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return 0;
    }

    // Register Vcs(s) with the cache
    QString tmpDir = directory;
    for (QList<QPair<QString, IVersionControl *> >::const_iterator i = allThatCanManage.constBegin();
         i != allThatCanManage.constEnd(); ++i) {
        m_d->cache(i->second, i->first, tmpDir);
        tmpDir = i->first;
        tmpDir = tmpDir.left(tmpDir.lastIndexOf(SLASH));
    }

    // return result
    if (topLevelDirectory)
        *topLevelDirectory = allThatCanManage.first().first;
    return allThatCanManage.first().second;
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
                m_d->cache(versionControl, directory, directory);
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

} // namespace Core
