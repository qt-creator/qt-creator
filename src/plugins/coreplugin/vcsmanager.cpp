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

#include "vcsmanager.h"
#include "iversioncontrol.h"
#include "icore.h"
#include "documentmanager.h"
#include "idocument.h"
#include "infobar.h"

#include <coreplugin/dialogs/addtovcsdialog.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <vcsbase/vcsbaseconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QString>
#include <QList>
#include <QMap>

#include <QFileInfo>
#include <QMessageBox>

namespace Core {

typedef QList<IVersionControl *> VersionControlList;

#if defined(WITH_TESTS)
const char TEST_PREFIX[] = "/8E3A9BA0-0B97-40DF-AEC1-2BDF9FC9EDBE/";
#endif

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

    VcsManagerPrivate() : m_unconfiguredVcs(0), m_cachedAdditionalToolsPathsDirty(true)
    { }

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

    void clearCache()
    {
        m_cachedMatches.clear();
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
        while (tmpDir.count() >= topLevel.count() && !tmpDir.isEmpty()) {
            m_cachedMatches.insert(tmpDir, newInfo);
            // if no vc was found, this might mean we're inside a repo internal directory (.git)
            // Cache only input directory, not parents
            if (!vc)
                break;
            const int slashPos = tmpDir.lastIndexOf(slash);
            if (slashPos >= 0)
                tmpDir.truncate(slashPos);
            else
                tmpDir.clear();
        }
    }

    QMap<QString, VcsInfo *> m_cachedMatches;
    QList<VcsInfo *> m_vcsInfoList;
    IVersionControl *m_unconfiguredVcs;

    QStringList m_cachedAdditionalToolsPaths;
    bool m_cachedAdditionalToolsPathsDirty;
};

static VcsManagerPrivate *d = 0;
static VcsManager *m_instance = 0;

VcsManager::VcsManager(QObject *parent) :
   QObject(parent)
{
    m_instance = this;
    d = new VcsManagerPrivate;
}

// ---- VCSManager:

VcsManager::~VcsManager()
{
    m_instance = 0;
    delete d;
}

VcsManager *VcsManager::instance()
{
    return m_instance;
}

void VcsManager::extensionsInitialized()
{
    // Change signal connections
    foreach (IVersionControl *versionControl, versionControls()) {
        connect(versionControl, &IVersionControl::filesChanged,
                DocumentManager::instance(), &DocumentManager::filesChangedInternally);
        connect(versionControl, &IVersionControl::repositoryChanged,
                m_instance, &VcsManager::repositoryChanged);
        connect(versionControl, &IVersionControl::configurationChanged,
                m_instance, &VcsManager::handleConfigurationChanges);
    }
}

QList<IVersionControl *> VcsManager::versionControls()
{
    return ExtensionSystem::PluginManager::getObjects<IVersionControl>();
}

IVersionControl *VcsManager::versionControl(Id id)
{
    return Utils::findOrDefault(versionControls(), Utils::equal(&Core::IVersionControl::id, id));
}

static QString absoluteWithNoTrailingSlash(const QString &directory)
{
    QString res = QDir(directory).absolutePath();
    if (res.endsWith(QLatin1Char('/')))
        res.chop(1);
    return res;
}

void VcsManager::resetVersionControlForDirectory(const QString &inputDirectory)
{
    if (inputDirectory.isEmpty())
        return;

    const QString directory = absoluteWithNoTrailingSlash(inputDirectory);
    d->resetCache(directory);
    emit m_instance->repositoryChanged(directory);
}

IVersionControl* VcsManager::findVersionControlForDirectory(const QString &inputDirectory,
                                                            QString *topLevelDirectory)
{
    typedef QPair<QString, IVersionControl *> StringVersionControlPair;
    typedef QList<StringVersionControlPair> StringVersionControlPairs;
    if (inputDirectory.isEmpty()) {
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return 0;
    }

    // Make sure we an absolute path:
    QString directory = absoluteWithNoTrailingSlash(inputDirectory);
#ifdef WITH_TESTS
    if (directory[0].isLetter() && directory.indexOf(QLatin1Char(':') + QLatin1String(TEST_PREFIX)) == 1)
        directory = directory.mid(2);
#endif
    VcsManagerPrivate::VcsInfo *cachedData = d->findInCache(directory);
    if (cachedData) {
        if (topLevelDirectory)
            *topLevelDirectory = cachedData->topLevel;
        return cachedData->versionControl;
    }

    // Nothing: ask the IVersionControls directly.
    StringVersionControlPairs allThatCanManage;

    foreach (IVersionControl * versionControl, versionControls()) {
        QString topLevel;
        if (versionControl->managesDirectory(directory, &topLevel))
            allThatCanManage.push_back(StringVersionControlPair(topLevel, versionControl));
    }

    // To properly find a nested repository (say, git checkout inside SVN),
    // we need to select the version control with the longest toplevel pathname.
    Utils::sort(allThatCanManage, [](const StringVersionControlPair &l,
                                     const StringVersionControlPair &r) {
        return l.first.size() > r.first.size();
    });

    if (allThatCanManage.isEmpty()) {
        d->cache(0, QString(), directory); // register that nothing was found!

        // report result;
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return 0;
    }

    // Register Vcs(s) with the cache
    QString tmpDir = absoluteWithNoTrailingSlash(directory);
#if defined WITH_TESTS
    // Force caching of test directories (even though they do not exist):
    if (directory.startsWith(QLatin1String(TEST_PREFIX)))
        tmpDir = directory;
#endif
    // directory might refer to a historical directory which doesn't exist.
    // In this case, don't cache it.
    if (!tmpDir.isEmpty()) {
        const QChar slash = QLatin1Char('/');
        const StringVersionControlPairs::const_iterator cend = allThatCanManage.constEnd();
        for (StringVersionControlPairs::const_iterator i = allThatCanManage.constBegin(); i != cend; ++i) {
            // If topLevel was already cached for another VC, skip this one
            if (tmpDir.count() < i->first.count())
                continue;
            d->cache(i->second, i->first, tmpDir);
            tmpDir = i->first;
            const int slashPos = tmpDir.lastIndexOf(slash);
            if (slashPos >= 0)
                tmpDir.truncate(slashPos);
        }
    }

    // return result
    if (topLevelDirectory)
        *topLevelDirectory = allThatCanManage.first().first;
    IVersionControl *versionControl = allThatCanManage.first().second;
    const bool isVcsConfigured = versionControl->isConfigured();
    if (!isVcsConfigured || d->m_unconfiguredVcs) {
        Id vcsWarning("VcsNotConfiguredWarning");
        IDocument *curDocument = EditorManager::currentDocument();
        if (isVcsConfigured) {
            if (curDocument && d->m_unconfiguredVcs == versionControl) {
                curDocument->infoBar()->removeInfo(vcsWarning);
                d->m_unconfiguredVcs = 0;
            }
            return versionControl;
        } else {
            InfoBar *infoBar = curDocument ? curDocument->infoBar() : 0;
            if (infoBar && infoBar->canInfoBeAdded(vcsWarning)) {
                InfoBarEntry info(vcsWarning,
                                  tr("%1 repository was detected but %1 is not configured.")
                                  .arg(versionControl->displayName()),
                                  InfoBarEntry::GlobalSuppressionEnabled);
                d->m_unconfiguredVcs = versionControl;
                info.setCustomButtonInfo(ICore::msgShowOptionsDialog(), []() {
                    QTC_ASSERT(d->m_unconfiguredVcs, return);
                    ICore::showOptionsDialog(d->m_unconfiguredVcs->id());
                 });

                infoBar->addInfo(info);
            }
            return 0;
        }
    }
    return versionControl;
}

QString VcsManager::findTopLevelForDirectory(const QString &directory)
{
    QString result;
    findVersionControlForDirectory(directory, &result);
    return result;
}

QStringList VcsManager::repositories(const IVersionControl *vc)
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

bool VcsManager::promptToDelete(IVersionControl *vc, const QString &fileName)
{
    QTC_ASSERT(vc, return true);
    if (!vc->supportsOperation(IVersionControl::DeleteOperation))
        return true;
    const QString title = tr("Version Control");
    const QString msg = tr("Would you like to remove this file from the version control system (%1)?\n"
                           "Note: This might remove the local file.").arg(vc->displayName());
    const QMessageBox::StandardButton button =
        QMessageBox::question(ICore::dialogParent(), title, msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
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

QStringList VcsManager::additionalToolsPath()
{
    if (d->m_cachedAdditionalToolsPathsDirty) {
        d->m_cachedAdditionalToolsPaths.clear();
        foreach (IVersionControl *vc, versionControls())
            d->m_cachedAdditionalToolsPaths.append(vc->additionalToolsPath());
        d->m_cachedAdditionalToolsPathsDirty = false;
    }
    return d->m_cachedAdditionalToolsPaths;
}

void VcsManager::promptToAdd(const QString &directory, const QStringList &fileNames)
{
    IVersionControl *vc = findVersionControlForDirectory(directory);
    if (!vc || !vc->supportsOperation(IVersionControl::AddOperation))
        return;

    QStringList unmanagedFiles;
    QDir dir(directory);
    foreach (const QString &fileName, fileNames) {
        if (!vc->managesFile(directory, dir.relativeFilePath(fileName)))
            unmanagedFiles << fileName;
    }
    if (unmanagedFiles.isEmpty())
        return;

    Internal::AddToVcsDialog dlg(ICore::mainWindow(), VcsManager::msgAddToVcsTitle(),
                                 unmanagedFiles, vc->displayName());
    if (dlg.exec() == QDialog::Accepted) {
        QStringList notAddedToVc;
        foreach (const QString &file, unmanagedFiles) {
            if (!vc->vcsAdd(file))
                notAddedToVc << file;
        }

        if (!notAddedToVc.isEmpty()) {
            QMessageBox::warning(ICore::mainWindow(), VcsManager::msgAddToVcsFailedTitle(),
                                 VcsManager::msgToAddToVcsFailed(notAddedToVc, vc));
        }
    }
}

void VcsManager::emitRepositoryChanged(const QString &repository)
{
    emit m_instance->repositoryChanged(repository);
}

void VcsManager::clearVersionControlCache()
{
    QStringList repoList = d->m_cachedMatches.keys();
    d->clearCache();
    foreach (const QString &repo, repoList)
        emit m_instance->repositoryChanged(repo);
}

void VcsManager::handleConfigurationChanges()
{
    d->m_cachedAdditionalToolsPathsDirty = true;
    IVersionControl *vcs = qobject_cast<IVersionControl *>(sender());
    if (vcs)
        emit configurationChanged(vcs);
}

} // namespace Core

#if defined(WITH_TESTS)

#include <QtTest>

#include "coreplugin.h"

#include <extensionsystem/pluginmanager.h>

namespace Core {
namespace Internal {

const char ID_VCS_A[] = "A";
const char ID_VCS_B[] = "B";

typedef QHash<QString, QString> FileHash;

template<class T>
class ObjectPoolGuard
{
public:
    ObjectPoolGuard(T *watch) : m_watched(watch)
    {
        ExtensionSystem::PluginManager::addObject(watch);
    }

    explicit operator bool() { return m_watched; }
    bool operator !() { return !m_watched; }
    T &operator*() { return *m_watched; }
    T *operator->() { return m_watched; }
    T *value() { return m_watched; }

    ~ObjectPoolGuard()
    {
        ExtensionSystem::PluginManager::removeObject(m_watched);
        delete m_watched;
    }

private:
    T *m_watched;
};

static FileHash makeHash(const QStringList &list)
{
    FileHash result;
    foreach (const QString &i, list) {
        QStringList parts = i.split(QLatin1Char(':'));
        QTC_ASSERT(parts.count() == 2, continue);
        result.insert(QString::fromLatin1(TEST_PREFIX) + parts.at(0),
                      QString::fromLatin1(TEST_PREFIX) + parts.at(1));
    }
    return result;
}

static QString makeString(const QString &s)
{
    if (s.isEmpty())
        return QString();
    return QString::fromLatin1(TEST_PREFIX) + s;
}

void CorePlugin::testVcsManager_data()
{
    // avoid conflicts with real files and directories:

    QTest::addColumn<QStringList>("dirsVcsA"); // <directory>:<toplevel>
    QTest::addColumn<QStringList>("dirsVcsB"); // <directory>:<toplevel>
    // <directory>:<toplevel>:<vcsid>:<- from cache, * from VCS>
    QTest::addColumn<QStringList>("results");

    QTest::newRow("A and B next to each other")
            << QStringList({"a:a", "a/1:a", "a/2:a", "a/2/5:a", "a/2/5/6:a"})
            << QStringList({"b:b", "b/3:b", "b/4:b"})
            << QStringList({":::-",          // empty directory to look up
                            "c:::*",         // Neither in A nor B
                            "a:a:A:*",       // in A
                            "b:b:B:*",       // in B
                            "b/3:b:B:*",     // in B
                            "b/4:b:B:*",     // in B
                            "a/1:a:A:*",     // in A
                            "a/2:a:A:*",     // in A
                            ":::-",          // empty directory to look up
                            "a/2/5/6:a:A:*", // in A
                            "a/2/5:a:A:-",   // in A (cached from before!)
                            // repeat: These need to come from the cache now:
                            "c:::-",         // Neither in A nor B
                            "a:a:A:-",       // in A
                            "b:b:B:-",       // in B
                            "b/3:b:B:-",     // in B
                            "b/4:b:B:-",     // in B
                            "a/1:a:A:-",     // in A
                            "a/2:a:A:-",     // in A
                            "a/2/5/6:a:A:-", // in A
                            "a/2/5:a:A:-"    // in A
                });
    QTest::newRow("B in A")
            << QStringList({"a:a", "a/1:a", "a/2:a", "a/2/5:a", "a/2/5/6:a"})
            << QStringList({"a/1/b:a/1/b", "a/1/b/3:a/1/b", "a/1/b/4:a/1/b", "a/1/b/3/5:a/1/b",
                            "a/1/b/3/5/6:a/1/b"})
            << QStringList({"a:a:A:*",            // in A
                            "c:::*",              // Neither in A nor B
                            "a/3:::*",            // Neither in A nor B
                            "a/1/b/x:::*",        // Neither in A nor B
                            "a/1/b:a/1/b:B:*",    // in B
                            "a/1:a:A:*",          // in A
                            "a/1/b/../../2:a:A:*" // in A
                });
    QTest::newRow("A and B") // first one wins...
            << QStringList({"a:a", "a/1:a", "a/2:a"})
            << QStringList({"a:a", "a/1:a", "a/2:a"})
            << QStringList({"a/2:a:A:*"});
}

void CorePlugin::testVcsManager()
{
    // setup:
    ObjectPoolGuard<TestVersionControl> vcsA(new TestVersionControl(ID_VCS_A, QLatin1String("A")));
    ObjectPoolGuard<TestVersionControl> vcsB(new TestVersionControl(ID_VCS_B, QLatin1String("B")));

    // test:
    QFETCH(QStringList, dirsVcsA);
    QFETCH(QStringList, dirsVcsB);
    QFETCH(QStringList, results);

    vcsA->setManagedDirectories(makeHash(dirsVcsA));
    vcsB->setManagedDirectories(makeHash(dirsVcsB));

    QString realTopLevel = QLatin1String("ABC"); // Make sure this gets cleared if needed.

    // From VCSes:
    int expectedCount = 0;
    foreach (const QString &result, results) {
        // qDebug() << "Expecting:" << result;

        QStringList split = result.split(QLatin1Char(':'));
        QCOMPARE(split.count(), 4);
        QVERIFY(split.at(3) == QLatin1String("*") || split.at(3) == QLatin1String("-"));


        const QString directory = split.at(0);
        const QString topLevel = split.at(1);
        const QString vcsId = split.at(2);
        bool fromCache = split.at(3) == QLatin1String("-");

        if (!fromCache && !directory.isEmpty())
            ++expectedCount;

        IVersionControl *vcs;
        vcs = VcsManager::findVersionControlForDirectory(makeString(directory), &realTopLevel);
        QCOMPARE(realTopLevel, makeString(topLevel));
        if (vcs)
            QCOMPARE(vcs->id().toString(), vcsId);
        else
            QCOMPARE(QString(), vcsId);
        QCOMPARE(vcsA->dirCount(), expectedCount);
        QCOMPARE(vcsA->fileCount(), 0);
        QCOMPARE(vcsB->dirCount(), expectedCount);
        QCOMPARE(vcsB->fileCount(), 0);
    }

    // teardown:
    // handled by guards
}

} // namespace Internal
} // namespace Core

#endif
