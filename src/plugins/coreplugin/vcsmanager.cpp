// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsmanager.h"

#include "coreplugintr.h"
#include "dialogs/addtovcsdialog.h"
#include "documentmanager.h"
#include "editormanager/editormanager.h"
#include "icore.h"
#include "idocument.h"
#include "iversioncontrol.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>

#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QString>

#include <optional>

using namespace Utils;

namespace Core {

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
        IVersionControl *versionControl = nullptr;
        FilePath topLevel;
    };

    std::optional<VcsInfo> findInCache(const FilePath &dir) const
    {
        QTC_ASSERT(dir.isAbsolutePath(), return std::nullopt);

        const auto it = m_cachedMatches.constFind(dir);
        return it == m_cachedMatches.constEnd() ? std::nullopt : std::make_optional(it.value());
    }

    void clearCache()
    {
        m_cachedMatches.clear();
    }

    void resetCache(const FilePath &dir)
    {
        QTC_ASSERT(dir.isAbsolutePath(), return);

        const FilePaths keys = m_cachedMatches.keys();
        for (const FilePath &key : keys) {
            if (key == dir || key.isChildOf(dir))
                m_cachedMatches.remove(key);
        }
    }

    void cache(IVersionControl *vc, const FilePath &topLevel, const FilePath &dir)
    {
        QTC_ASSERT(dir.isAbsolutePath(), return);

        const QString topLevelString = topLevel.toString();
        QTC_ASSERT(dir.isChildOf(topLevel) || topLevel == dir || topLevel.isEmpty(), return);
        QTC_ASSERT((topLevel.isEmpty() && !vc) || (!topLevel.isEmpty() && vc), return);

        FilePath tmpDir = dir;
        while (tmpDir.toString().size() >= topLevelString.size() && !tmpDir.isEmpty()) {
            m_cachedMatches.insert(tmpDir, {vc, topLevel});
            // if no vc was found, this might mean we're inside a repo internal directory (.git)
            // Cache only input directory, not parents
            if (!vc)
                break;
            tmpDir = tmpDir.parentDir();
        }
    }

    QList<IVersionControl *> m_versionControlList;
    QMap<FilePath, VcsInfo> m_cachedMatches;
    IVersionControl *m_unconfiguredVcs = nullptr;

    FilePaths m_cachedAdditionalToolsPaths;
    bool m_cachedAdditionalToolsPathsDirty = true;
};

static VcsManagerPrivate *d = nullptr;
static VcsManager *m_instance = nullptr;

VcsManager::VcsManager(QObject *parent) :
   QObject(parent)
{
    m_instance = this;
    d = new VcsManagerPrivate;
}

// ---- VCSManager:

VcsManager::~VcsManager()
{
    m_instance = nullptr;
    delete d;
}

void VcsManager::addVersionControl(IVersionControl *vc)
{
    QTC_ASSERT(!d->m_versionControlList.contains(vc), return);
    d->m_versionControlList.append(vc);
}

VcsManager *VcsManager::instance()
{
    return m_instance;
}

void VcsManager::extensionsInitialized()
{
    // Change signal connections
    const QList<IVersionControl *> vcs = versionControls();
    for (IVersionControl *vc : vcs) {
        connect(vc, &IVersionControl::filesChanged, DocumentManager::instance(),
                [](const QStringList &fileNames) {
            DocumentManager::notifyFilesChangedInternally(
                        FileUtils::toFilePathList(fileNames));
        });
        connect(vc, &IVersionControl::repositoryChanged,
                m_instance, &VcsManager::repositoryChanged);
        connect(vc, &IVersionControl::configurationChanged, m_instance, [vc] {
            m_instance->handleConfigurationChanges(vc);
        });
    }
}

const QList<IVersionControl *> VcsManager::versionControls()
{
    return d->m_versionControlList;
}

IVersionControl *VcsManager::versionControl(Id id)
{
    return Utils::findOrDefault(versionControls(), Utils::equal(&Core::IVersionControl::id, id));
}

void VcsManager::resetVersionControlForDirectory(const FilePath &inputDirectory)
{
    if (inputDirectory.isEmpty())
        return;

    const FilePath directory = inputDirectory.absolutePath();
    d->resetCache(directory);
    emit m_instance->repositoryChanged(directory);
}

static FilePath fixedDir(const FilePath &directory)
{
#ifdef WITH_TESTS
    const QString directoryString = directory.toString();
    if (!directoryString.isEmpty() && directoryString[0].isLetter()
        && directoryString.indexOf(QLatin1Char(':') + QLatin1String(TEST_PREFIX)) == 1) {
        return FilePath::fromString(directoryString.mid(2));
    }
#endif
    return directory;
}


IVersionControl* VcsManager::findVersionControlForDirectory(const FilePath &inputDirectory,
                                                            FilePath *topLevelDirectory)
{
    using FilePathVersionControlPair = QPair<FilePath, IVersionControl *>;
    using FilePathVersionControlPairs = QList<FilePathVersionControlPair>;
    if (inputDirectory.isEmpty()) {
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return nullptr;
    }

    // Make sure we an absolute path:
    const FilePath directory = fixedDir(inputDirectory.absoluteFilePath());
    auto cachedData = d->findInCache(directory);
    if (cachedData) {
        if (topLevelDirectory)
            *topLevelDirectory = cachedData->topLevel;
        return cachedData->versionControl;
    }

    // Nothing: ask the IVersionControls directly.
    FilePathVersionControlPairs allThatCanManage;

    const QList<IVersionControl *> versionControlList = versionControls();
    for (IVersionControl *versionControl : versionControlList) {
        FilePath topLevel;
        if (versionControl->managesDirectory(directory, &topLevel))
            allThatCanManage.push_back({topLevel, versionControl});
    }

    // To properly find a nested repository (say, git checkout inside SVN),
    // we need to select the version control with the longest toplevel pathname.
    Utils::sort(allThatCanManage, [](const FilePathVersionControlPair &l,
                                     const FilePathVersionControlPair &r) {
        return l.first.toString().size() > r.first.toString().size();
    });

    if (allThatCanManage.isEmpty()) {
        d->cache(nullptr, {}, directory); // register that nothing was found!

        // report result;
        if (topLevelDirectory)
            topLevelDirectory->clear();
        return nullptr;
    }

    // Register Vcs(s) with the cache
    FilePath tmpDir = directory.absolutePath();
#if defined WITH_TESTS
    // Force caching of test directories (even though they do not exist):
    if (directory.startsWith(TEST_PREFIX))
        tmpDir = directory;
#endif
    // directory might refer to a historical directory which doesn't exist.
    // In this case, don't cache it.
    if (!tmpDir.isEmpty()) {
        for (auto i = allThatCanManage.constBegin(); i != allThatCanManage.constEnd(); ++i) {
            const QString firstString = i->first.toString();
            // If topLevel was already cached for another VC, skip this one
            if (tmpDir.toString().size() < firstString.size())
                continue;
            d->cache(i->second, i->first, tmpDir);
            tmpDir = i->first.parentDir();
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
                d->m_unconfiguredVcs = nullptr;
            }
            return versionControl;
        } else {
            Utils::InfoBar *infoBar = curDocument ? curDocument->infoBar() : nullptr;
            if (infoBar && infoBar->canInfoBeAdded(vcsWarning)) {
                Utils::InfoBarEntry info(vcsWarning,
                                         Tr::tr("%1 repository was detected but %1 is not configured.")
                                             .arg(versionControl->displayName()),
                                         Utils::InfoBarEntry::GlobalSuppression::Enabled);
                d->m_unconfiguredVcs = versionControl;
                info.addCustomButton(ICore::msgShowOptionsDialog(), [] {
                    QTC_ASSERT(d->m_unconfiguredVcs, return);
                    ICore::showOptionsDialog(d->m_unconfiguredVcs->id());
                 });

                infoBar->addInfo(info);
            }
            return nullptr;
        }
    }
    return versionControl;
}

FilePath VcsManager::findTopLevelForDirectory(const FilePath &directory)
{
    FilePath result;
    findVersionControlForDirectory(directory, &result);
    return result;
}

FilePaths VcsManager::repositories(const IVersionControl *versionControl)
{
    FilePaths result;
    for (auto it = d->m_cachedMatches.constBegin(); it != d->m_cachedMatches.constEnd(); ++it) {
        if (it.value().versionControl == versionControl)
            result.append(it.value().topLevel);
    }
    return result;
}

bool VcsManager::promptToDelete(IVersionControl *versionControl, const FilePath &filePath)
{
    return promptToDelete(versionControl, FilePaths({filePath})).isEmpty();
}

FilePaths VcsManager::promptToDelete(const FilePaths &filePaths)
{
    // Categorize files by their parent directory, so we won't call
    // findVersionControlForDirectory() more often than necessary.
    QMap<FilePath, FilePaths> filesByParentDir;
    for (const FilePath &fp : filePaths)
        filesByParentDir[fp.absolutePath()].append(fp);

    // Categorize by version control system.
    QHash<IVersionControl *, FilePaths> filesByVersionControl;
    for (auto it = filesByParentDir.cbegin(); it != filesByParentDir.cend(); ++it) {
        IVersionControl * const vc = findVersionControlForDirectory(it.key());
        if (vc)
            filesByVersionControl[vc] << it.value();
    }

    // Remove the files.
    FilePaths failedFiles;
    for (auto it = filesByVersionControl.cbegin(); it != filesByVersionControl.cend(); ++it)
        failedFiles << promptToDelete(it.key(), it.value());

    return failedFiles;
}

FilePaths VcsManager::promptToDelete(IVersionControl *vc, const FilePaths &filePaths)
{
    QTC_ASSERT(vc, return {});
    if (!vc->supportsOperation(IVersionControl::DeleteOperation))
        return {};

    const QString fileListForUi = "<ul><li>" + transform(filePaths, [](const FilePath &fp) {
        return fp.toUserOutput();
    }).join("</li><li>") + "</li></ul>";
    const QString title = Tr::tr("Version Control");
    const QString msg = Tr::tr("Remove the following files from the version control system (%1)?")
                            .arg(vc->displayName())
                        + fileListForUi + Tr::tr("Note: This might remove the local file.");
    const QMessageBox::StandardButton button =
        QMessageBox::question(ICore::dialogParent(), title, msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (button != QMessageBox::Yes)
        return {};

    FilePaths failedFiles;
    for (const FilePath &fp : filePaths) {
        if (!vc->vcsDelete(fp))
            failedFiles << fp;
    }
    return failedFiles;
}

QString VcsManager::msgAddToVcsTitle()
{
    return Tr::tr("Add to Version Control");
}

QString VcsManager::msgPromptToAddToVcs(const QStringList &files, const IVersionControl *vc)
{
    return files.size() == 1
        ? Tr::tr("Add the file\n%1\nto version control (%2)?")
              .arg(files.front(), vc->displayName())
        : Tr::tr("Add the files\n%1\nto version control (%2)?")
              .arg(files.join(QString(QLatin1Char('\n'))), vc->displayName());
}

QString VcsManager::msgAddToVcsFailedTitle()
{
    return Tr::tr("Adding to Version Control Failed");
}

QString VcsManager::msgToAddToVcsFailed(const QStringList &files, const IVersionControl *vc)
{
    return files.size() == 1
        ? Tr::tr("Could not add the file\n%1\nto version control (%2)\n")
              .arg(files.front(), vc->displayName())
        : Tr::tr("Could not add the following files to version control (%1)\n%2")
              .arg(vc->displayName(), files.join(QString(QLatin1Char('\n'))));
}

FilePaths VcsManager::additionalToolsPath()
{
    if (d->m_cachedAdditionalToolsPathsDirty) {
        d->m_cachedAdditionalToolsPaths.clear();
        for (IVersionControl *vc : versionControls())
            d->m_cachedAdditionalToolsPaths.append(vc->additionalToolsPath());
        d->m_cachedAdditionalToolsPathsDirty = false;
    }
    return d->m_cachedAdditionalToolsPaths;
}

void VcsManager::promptToAdd(const FilePath &directory, const FilePaths &filePaths)
{
    IVersionControl *vc = findVersionControlForDirectory(directory);
    if (!vc || !vc->supportsOperation(IVersionControl::AddOperation))
        return;

    const FilePaths unmanagedFiles = vc->unmanagedFiles(filePaths);
    if (unmanagedFiles.isEmpty())
        return;

    Internal::AddToVcsDialog dlg(ICore::dialogParent(), VcsManager::msgAddToVcsTitle(),
                                 unmanagedFiles, vc->displayName());
    if (dlg.exec() == QDialog::Accepted) {
        QStringList notAddedToVc;
        for (const FilePath &file : unmanagedFiles) {
            if (!vc->vcsAdd(directory.resolvePath(file)))
                notAddedToVc << file.toUserOutput();
        }

        if (!notAddedToVc.isEmpty()) {
            QMessageBox::warning(ICore::dialogParent(),
                                 VcsManager::msgAddToVcsFailedTitle(),
                                 VcsManager::msgToAddToVcsFailed(notAddedToVc, vc));
        }
    }
}

void VcsManager::emitRepositoryChanged(const FilePath &repository)
{
    emit m_instance->repositoryChanged(repository);
}

void VcsManager::clearVersionControlCache()
{
    const FilePaths repoList = d->m_cachedMatches.keys();
    d->clearCache();
    for (const FilePath &repo : repoList)
        emit m_instance->repositoryChanged(repo);
}

void VcsManager::handleConfigurationChanges(IVersionControl *vc)
{
    d->m_cachedAdditionalToolsPathsDirty = true;
    emit configurationChanged(vc);
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

using FileHash = QHash<FilePath, FilePath>;

static FileHash makeHash(const QStringList &list)
{
    FileHash result;
    for (const QString &i : list) {
        QStringList parts = i.split(QLatin1Char(':'));
        QTC_ASSERT(parts.size() == 2, continue);
        result.insert(FilePath::fromString(QString::fromLatin1(TEST_PREFIX) + parts.at(0)),
                      FilePath::fromString(QString::fromLatin1(TEST_PREFIX) + parts.at(1)));
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
    QList<IVersionControl *> orig = Core::d->m_versionControlList;
    TestVersionControl *vcsA(new TestVersionControl(ID_VCS_A, QLatin1String("A")));
    TestVersionControl *vcsB(new TestVersionControl(ID_VCS_B, QLatin1String("B")));

    Core::d->m_versionControlList = {vcsA, vcsB};

    // test:
    QFETCH(QStringList, dirsVcsA);
    QFETCH(QStringList, dirsVcsB);
    QFETCH(QStringList, results);

    vcsA->setManagedDirectories(makeHash(dirsVcsA));
    vcsB->setManagedDirectories(makeHash(dirsVcsB));

    // From VCSes:
    int expectedCount = 0;
    for (const QString &result : std::as_const(results)) {
        // qDebug() << "Expecting:" << result;

        const QStringList split = result.split(QLatin1Char(':'));
        QCOMPARE(split.size(), 4);
        QVERIFY(split.at(3) == QLatin1String("*") || split.at(3) == QLatin1String("-"));


        const QString directory = split.at(0);
        const QString topLevel = split.at(1);
        const QString vcsId = split.at(2);
        bool fromCache = split.at(3) == QLatin1String("-");

        if (!fromCache && !directory.isEmpty())
            ++expectedCount;

        IVersionControl *vcs;
        FilePath realTopLevel;
        vcs = VcsManager::findVersionControlForDirectory(
            FilePath::fromString(makeString(directory)), &realTopLevel);
        QCOMPARE(realTopLevel.toString(), makeString(topLevel));
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
    qDeleteAll(Core::d->m_versionControlList);
    Core::d->m_versionControlList = orig;
}

} // namespace Internal
} // namespace Core

#endif
