/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "vcsmanager.h"
#include "iversioncontrol.h"
#include "icore.h"
#include "documentmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QString>
#include <QList>
#include <QMap>
#include <QCoreApplication>

#include <QFileInfo>
#include <QMessageBox>

namespace Core {

typedef QList<IVersionControl *> VersionControlList;

static inline VersionControlList allVersionControls()
{
    return ExtensionSystem::PluginManager::getObjects<IVersionControl>();
}

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

    VcsInfo *findInCache(const QString &dir)
    {
        QTC_ASSERT(QDir(dir).isAbsolute(), return 0);
        QTC_ASSERT(!dir.endsWith(QLatin1Char('/')), return 0);
        QTC_ASSERT(QDir::fromNativeSeparators(dir) == dir, return 0);

        const QMap<QString, VcsInfo *>::const_iterator it = m_cachedMatches.constFind(dir);
        if (it != m_cachedMatches.constEnd())
            return it.value();
        return 0;
    }

    VcsInfo *findUpInCache(const QString &directory)
    {
        VcsInfo *result = 0;
        const QChar slash = QLatin1Char('/');
        // Split the path, trying to find the matching repository. We start from the reverse
        // in order to detected nested repositories correctly (say, a git checkout under SVN).
        for (int pos = directory.size() - 1; pos >= 0; pos = directory.lastIndexOf(slash, pos) - 1) {
            const QString directoryPart = directory.left(pos);
            result = findInCache(directoryPart);
            if (result != 0)
                break;
        }
        return result;
    }

    void resetCache(const QString &dir)
    {
        QTC_ASSERT(QDir(dir).isAbsolute(), return);
        QTC_ASSERT(!dir.endsWith(QLatin1Char('/')), return);
        QTC_ASSERT(QDir::fromNativeSeparators(dir) == dir, return);

        const QString dirSlash = dir + QLatin1Char('/');
        foreach (const QString &key, m_cachedMatches.keys()) {
            if (key == dir || key.startsWith(dirSlash))
                m_cachedMatches.remove(key);
        }
    }

    void cache(IVersionControl *vc, const QString &topLevel, const QString &dir)
    {
        QTC_ASSERT(QDir(dir).isAbsolute(), return);
        QTC_ASSERT(!dir.endsWith(QLatin1Char('/')), return);
        QTC_ASSERT(QDir::fromNativeSeparators(dir) == dir, return);
        QTC_ASSERT(dir.startsWith(topLevel + QLatin1Char('/'))
                   || topLevel == dir || topLevel.isEmpty(), return);
        QTC_ASSERT((topLevel.isEmpty() && !vc) || (!topLevel.isEmpty() && vc), return);

        VcsInfo *newInfo = new VcsInfo(vc, topLevel);
        bool createdNewInfo(true);
        // Do we have a matching VcsInfo already?
        foreach (VcsInfo *i, m_vcsInfoList) {
            if (*i == *newInfo) {
                delete newInfo;
                newInfo = i;
                createdNewInfo = false;
                break;
            }
        }
        if (createdNewInfo)
            m_vcsInfoList.append(newInfo);

        QString tmpDir = dir;
        const QChar slash = QLatin1Char('/');
        while (tmpDir.count() >= topLevel.count() && tmpDir.count() > 0) {
            m_cachedMatches.insert(tmpDir, newInfo);
            const int slashPos = tmpDir.lastIndexOf(slash);
            if (slashPos >= 0)
                tmpDir.truncate(slashPos);
            else
                tmpDir.clear();
        }
    }

    QMap<QString, VcsInfo *> m_cachedMatches;
    QList<VcsInfo *> m_vcsInfoList;
};

VcsManager::VcsManager(QObject *parent) :
   QObject(parent),
   d(new VcsManagerPrivate)
{
}

// ---- VCSManager:

VcsManager::~VcsManager()
{
    delete d;
}

void VcsManager::extensionsInitialized()
{
    // Change signal connections
    foreach (IVersionControl *versionControl, allVersionControls()) {
        connect(versionControl, SIGNAL(filesChanged(QStringList)),
                DocumentManager::instance(), SIGNAL(filesChangedInternally(QStringList)));
        connect(versionControl, SIGNAL(repositoryChanged(QString)),
                this, SIGNAL(repositoryChanged(QString)));
    }
}

static bool longerThanPath(QPair<QString, IVersionControl *> &pair1, QPair<QString, IVersionControl *> &pair2)
{
    return pair1.first.size() > pair2.first.size();
}

void VcsManager::resetVersionControlForDirectory(const QString &inputDirectory)
{
    if (inputDirectory.isEmpty())
        return;

    const QString directory = QDir(inputDirectory).absolutePath();

    d->resetCache(directory);
    emit repositoryChanged(directory);
}

IVersionControl* VcsManager::findVersionControlForDirectory(const QString &inputDirectory,
                                                            QString *topLevelDirectory)
{
    typedef QPair<QString, IVersionControl *> StringVersionControlPair;
    typedef QList<StringVersionControlPair> StringVersionControlPairs;
    if (inputDirectory.isEmpty())
        return 0;

    // Make sure we an absolute path:
    const QString directory = QDir(inputDirectory).absolutePath();

    VcsManagerPrivate::VcsInfo *cachedData = d->findInCache(directory);
    if (cachedData) {
        if (topLevelDirectory)
            *topLevelDirectory = cachedData->topLevel;
        return cachedData->versionControl;
    }

    // Nothing: ask the IVersionControls directly.
    const VersionControlList versionControls = allVersionControls();
    StringVersionControlPairs allThatCanManage;

    foreach (IVersionControl * versionControl, versionControls) {
        QString topLevel;
        if (versionControl->managesDirectory(directory, &topLevel))
            allThatCanManage.push_back(StringVersionControlPair(topLevel, versionControl));
    }

    // To properly find a nested repository (say, git checkout inside SVN),
    // we need to select the version control with the longest toplevel pathname.
    qSort(allThatCanManage.begin(), allThatCanManage.end(), longerThanPath);

    if (allThatCanManage.isEmpty()) {
        d->cache(0, QString(), directory); // register that nothing was found!

        // report result;
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return 0;
    }

    // Register Vcs(s) with the cache
    QString tmpDir = QFileInfo(directory).canonicalFilePath();
    const QChar slash = QLatin1Char('/');
    const StringVersionControlPairs::const_iterator cend = allThatCanManage.constEnd();
    for (StringVersionControlPairs::const_iterator i = allThatCanManage.constBegin(); i != cend; ++i) {
        d->cache(i->second, i->first, tmpDir);
        tmpDir = i->first;
        const int slashPos = tmpDir.lastIndexOf(slash);
        if (slashPos >= 0)
            tmpDir.truncate(slashPos);
    }

    // return result
    if (topLevelDirectory)
        *topLevelDirectory = allThatCanManage.first().first;
    return allThatCanManage.first().second;
}

QStringList VcsManager::repositories(const IVersionControl *vc) const
{
    QStringList result;
    foreach (const VcsManagerPrivate::VcsInfo *vi, d->m_vcsInfoList)
        if (vi->versionControl == vc)
            result.push_back(vi->topLevel);
    return result;
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
                d->cache(versionControl, directory, directory);
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
    QTC_ASSERT(vc, return true);
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

QString VcsManager::msgAddToVcsTitle()
{
    return tr("Add to Version Control");
}

QString VcsManager::msgPromptToAddToVcs(const QStringList &files, const IVersionControl *vc)
{
    return files.size() == 1
        ? tr("Add the file\n%1\nto version control (%2)?")
              .arg(files.front(), vc->displayName())
        : tr("Add the files\n%1\nto version control (%2)?")
              .arg(files.join(QString(QLatin1Char('\n'))), vc->displayName());
}

QString VcsManager::msgAddToVcsFailedTitle()
{
    return tr("Adding to Version Control Failed");
}

QString VcsManager::msgToAddToVcsFailed(const QStringList &files, const IVersionControl *vc)
{
    return files.size() == 1
        ? tr("Could not add the file\n%1\nto version control (%2)\n")
              .arg(files.front(), vc->displayName())
        : tr("Could not add the following files to version control (%1)\n%2")
              .arg(vc->displayName(), files.join(QString(QLatin1Char('\n'))));
}

void VcsManager::promptToAdd(const QString &directory, const QStringList &fileNames)
{
    IVersionControl *vc = findVersionControlForDirectory(directory);
    if (!vc || !vc->supportsOperation(Core::IVersionControl::AddOperation))
        return;

    QMessageBox::StandardButton button =
           QMessageBox::question(Core::ICore::mainWindow(), VcsManager::msgAddToVcsTitle(),
                                 VcsManager::msgPromptToAddToVcs(fileNames, vc),
                                 QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes) {
        QStringList notAddedToVc;
        foreach (const QString &file, fileNames) {
            if (!vc->vcsAdd(file))
                notAddedToVc << file;
        }

        if (!notAddedToVc.isEmpty()) {
            QMessageBox::warning(Core::ICore::mainWindow(), VcsManager::msgAddToVcsFailedTitle(),
                                 VcsManager::msgToAddToVcsFailed(notAddedToVc, vc));
        }
    }
}

} // namespace Core
