// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iversioncontrol.h"

#include "coreplugintr.h"
#include "editormanager/documentmodel.h"
#include "idocument.h"
#include "vcsmanager.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QTimer>

#include <utility>

using namespace Utils;

namespace Core {

namespace Internal {

class TopicData
{
public:
    QDateTime timeStamp;
    QString topic;
    bool upToDate = false;
    bool error = false;
    bool resolving = false;
    quint64 generation = 0;
    quint64 runningGeneration = 0;
    bool hasValue = false;
    bool forceRefresh = true;
    QDeadlineTimer interest{std::chrono::minutes(5)};
    FilePaths watchedPaths;
};

class IVersionControlPrivate
{
public:
    IVersionControl::FileTracker m_fileTracker;
    IVersionControl::TopicRefresher m_topicRefresher;
    IVersionControl::AsyncTopicRefresher m_asyncTopicRefresher;
    QHash<FilePath, TopicData> m_topicCache;
    quint64 m_topicGeneration = 0;
    QHash<FilePath, QSet<QObject *>> m_topicUsers;
    QHash<QObject *, QMetaObject::Connection> m_topicOwners;
    QThread m_watcherThread;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer m_debounceTimer;
    QTimer m_reconcileTimer;
    QSet<FilePath> m_pendingRefreshes;
};

} // Internal

IVersionControl::IVersionControl()
    : d(new Internal::IVersionControlPrivate)
{
    Core::VcsManager::addVersionControl(this);
    d->m_debounceTimer.setSingleShot(true);
    d->m_debounceTimer.setInterval(std::chrono::milliseconds(150));
    connect(&d->m_debounceTimer, &QTimer::timeout, this, [this] {
        const QSet<FilePath> repositories = std::exchange(d->m_pendingRefreshes, {});
        for (const FilePath &repository : repositories)
            requestTopic(repository);
    });
    d->m_reconcileTimer.setInterval(std::chrono::seconds(60));
    connect(&d->m_reconcileTimer, &QTimer::timeout, this, [this] {
        if (qApp->applicationState() == Qt::ApplicationActive)
            reconcileTopics();
    });
    connect(this, &IVersionControl::configurationChanged,
            this, &IVersionControl::reconcileTopics);
    connect(qApp, &QApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive)
            reconcileTopics();
    });
}

IVersionControl::~IVersionControl()
{
    d->m_watcherThread.quit();
    d->m_watcherThread.wait();
    delete d;
}

QString IVersionControl::vcsOpenText() const
{
    return Tr::tr("Open with Version Control (%1)").arg(displayName());
}

QString IVersionControl::vcsMakeWritableText() const
{
    return QString();
}

FilePaths IVersionControl::additionalToolsPath() const
{
    return {};
}

FilePaths IVersionControl::monitorDirectory(const Utils::FilePath &path, bool monitor)
{
    Q_UNUSED(path)
    Q_UNUSED(monitor)
    return {};
}

IVersionControl::RepoUrl::RepoUrl(const QString &location)
{
    if (location.isEmpty())
        return;

    // Check for local remotes (refer to the root or relative path)
    // On Windows, local paths typically starts with <drive>:
    auto locationIsOnWindowsDrive = [&location] {
        if (!Utils::HostOsInfo::isWindowsHost() || location.size() < 2)
            return false;
        const QChar drive = location.at(0).toLower();
        return drive >= 'a' && drive <= 'z' && location.at(1) == ':';
    };
    if (location.startsWith("file://") || location.startsWith('/') || location.startsWith('.')
            || locationIsOnWindowsDrive()) {
        protocol = "file";
        path = QDir::fromNativeSeparators(location.startsWith("file://")
                                               ? location.mid(7) : location);
        isValid = true;
        return;
    }

    // TODO: Why not use QUrl?
    static const QRegularExpression remotePattern(
                "^(?:(?<protocol>[^:]+)://)?(?:(?<user>[^@]+)@)?(?<host>[^:/]+)"
                "(?::(?<port>\\d+))?:?(?<path>.*)$");
    const QRegularExpressionMatch match = remotePattern.match(location);
    if (!match.hasMatch())
        return;

    bool ok  = false;
    protocol = match.captured("protocol");
    userName = match.captured("user");
    host = match.captured("host");
    port = match.captured("port").toUShort(&ok);
    path = match.captured("path");
    isValid = !host.isEmpty() && (ok || match.captured("port").isEmpty());
}

IVersionControl::RepoUrl IVersionControl::getRepoUrl(const QString &location) const
{
    return RepoUrl(location);
}

FilePath IVersionControl::trackFile(const FilePath &repository)
{
    QTC_ASSERT(d->m_fileTracker, return {});
    return d->m_fileTracker(repository);
}

/*!
    Returns the topic for repository under \a topLevel.

    A version control topic is typically the current active branch name, but it can
    also have other values (for example the latest tag) when there is no active branch.

    It is displayed:
    \list
    \li In the project tree, next to each root project - corresponding to the project.
    \li In the main window title - corresponding to the current active editor.
    \endlist

    Asynchronous backends return the last cached value. Call requestTopic() to start
    an initial refresh and listen for topicChanged() to update the display. Metadata
    watches, repository notifications and periodic reconciliation keep the cache current.

    Synchronous backends track a file expected to change with the topic, using
    setTopicFileTracker() and setTopicRefresher().

    The base implementation features a cache. If the cache for \a topLevel is valid,
    it will be used. Otherwise it can be refreshed using the items provided by
    \c setTopicFileTracker() and \c setTopicRefresher(), or by an asynchronous refresher installed
    with \c setAsyncTopicRefresher().

    \sa setTopicFileTracker(), setTopicRefresher(), setAsyncTopicRefresher(), requestTopic()
 */

IVersionControl::VcsTopic IVersionControl::vcsTopic(const FilePath &topLevel)
{
    QTC_ASSERT(!topLevel.isEmpty(), return {});
    if (d->m_asyncTopicRefresher) {
        auto &entry = d->m_topicCache[topLevel];
        entry.interest = QDeadlineTimer(std::chrono::minutes(5));
        return {entry.topic, entry.upToDate, entry.error, entry.resolving};
    }
    const FilePath file = trackFile(topLevel);
    if (file.isEmpty())
        return {};
    const QDateTime lastModified = file.lastModified();
    const auto cached = d->m_topicCache.constFind(topLevel);
    if (cached != d->m_topicCache.constEnd() && cached->timeStamp == lastModified)
        return {cached->topic, true};
    QTC_ASSERT(d->m_topicRefresher, return {});
    // Stamp before refreshing: the refresher may ask for this topic again.
    d->m_topicCache[topLevel].timeStamp = lastModified;
    const QString topic = d->m_topicRefresher(topLevel);
    // Re-entrant refreshers may rehash the cache or store a newer result.
    const auto it = d->m_topicCache.find(topLevel);
    if (it != d->m_topicCache.end() && it->timeStamp == lastModified)
        it->topic = topic;
    return {topic, true};
}

QString IVersionControl::vcsTopicForExpansion(const FilePath &topLevel)
{
    if (d->m_asyncTopicRefresher && d->m_topicRefresher)
        return d->m_topicRefresher(topLevel);
    return vcsTopic(topLevel).topic;
}

void IVersionControl::requestTopic(const FilePath &repository)
{
    QTC_ASSERT(!repository.isEmpty(), return);
    if (!d->m_asyncTopicRefresher)
        return;
    auto &data = d->m_topicCache[repository];
    if (data.resolving || data.upToDate || data.error)
        return;
    data.resolving = true;
    const quint64 generation = ++d->m_topicGeneration;
    data.generation = generation;
    data.runningGeneration = generation;
    const TopicRefresh refresh = std::exchange(data.forceRefresh, false)
        ? TopicRefresh::ResolveDirectories : TopicRefresh::ReuseDirectories;
    // A request may originate in model data() or a window-title update.
    QMetaObject::invokeMethod(this, [this, repository, generation, refresh] {
        d->m_asyncTopicRefresher(repository, generation, refresh);
    }, Qt::QueuedConnection);
}

void IVersionControl::fillDefaultFileActionMenu(QMenu *menu,
                                                IVersionControl *vc,
                                                const Utils::FilePath &topLevel,
                                                const Utils::FilePath &relativePath)
{
    const QString name = relativePath.isEmpty() ? topLevel.fileName() : relativePath.fileName();

    const QAction *diff = menu->addAction(Tr::tr("Diff \"%1\"").arg(name));
    connect(diff, &QAction::triggered, this, [=] {
        const FilePath path = relativePath.isEmpty() ? "." : relativePath;
        vc->vcsDiff(topLevel, path);
    });

    const QAction *log = menu->addAction(Tr::tr("Log \"%1\"").arg(name));
    connect(log, &QAction::triggered, this, [=] { vc->vcsLog(topLevel, relativePath); });

    const FilePath fullPath = topLevel.pathAppended(relativePath.path());
    if (!fullPath.isDir()) {
        const QAction *annotate = menu->addAction(Tr::tr("Annotate \"%1\"").arg(name));
        connect(annotate, &QAction::triggered, this, [=] { vc->vcsAnnotate(fullPath, 1); });
    }

    menu->addSeparator();
}

void IVersionControl::vcsFillFileActionMenu(QMenu *menu,
                                            const Utils::FilePath &topLevel,
                                            const Utils::FilePath &relativePath,
                                            VcsFileState vcsFileState)
{
    Q_UNUSED(menu);
    Q_UNUSED(topLevel);
    Q_UNUSED(relativePath);
    Q_UNUSED(vcsFileState);
}

/*!
    Provides the \a fileTracker function object for use in \c vscTopic() cache handling.

    The parameter object takes a repository as input and returns the file
    that should trigger topic refresh (e.g. .git/HEAD for Git).

    Modification of this file will invalidate the internal topic cache for the repository.
*/

void IVersionControl::setTopicFileTracker(const FileTracker &fileTracker)
{
    d->m_fileTracker = fileTracker;
}

/*!
    Provides the \a topicRefresher function object for use in synchronous \c vscTopic() cache
    handling.

    The parameter object takes a repository as input and returns its current topic.
 */

void IVersionControl::setTopicRefresher(const TopicRefresher &topicRefresher)
{
    d->m_topicRefresher = topicRefresher;
}

void IVersionControl::setAsyncTopicRefresher(const AsyncTopicRefresher &topicRefresher)
{
    d->m_asyncTopicRefresher = topicRefresher;
    if (!topicRefresher) {
        d->m_reconcileTimer.stop();
        return;
    }
    if (!d->m_watcher) {
        d->m_watcher = new QFileSystemWatcher;
        d->m_watcher->moveToThread(&d->m_watcherThread);
        connect(&d->m_watcherThread, &QThread::finished,
                d->m_watcher, &QObject::deleteLater);
        const auto metadataChanged = [this](const QString &path) {
            const FilePath file = FilePath::fromString(path);
            const auto repositories = d->m_topicCache.keys();
            for (const FilePath &repository : repositories) {
                if (!d->m_topicCache.value(repository).watchedPaths.contains(file))
                    continue;
                invalidateTopic(repository, TopicRefresh::ReuseDirectories);
                d->m_pendingRefreshes.insert(repository);
            }
            d->m_debounceTimer.start();
        };
        connect(d->m_watcher, &QFileSystemWatcher::fileChanged, this, metadataChanged);
        connect(d->m_watcher, &QFileSystemWatcher::directoryChanged, this, metadataChanged);
        d->m_watcherThread.start();
    }
    d->m_reconcileTimer.start();
}

void IVersionControl::invalidateTopic(const FilePath &repository, TopicRefresh refresh)
{
    if (repository.isEmpty())
        return;
    Internal::TopicData &data = d->m_topicCache[repository];
    ++data.generation;
    data.forceRefresh |= refresh == TopicRefresh::ResolveDirectories;
    data.upToDate = false;
    data.error = false;
}

bool IVersionControl::refreshTopic(const FilePath &repository, quint64 generation,
                                   const Result<QString> &topic, const FilePaths &watchedPaths)
{
    const auto it = d->m_topicCache.find(repository);
    if (it == d->m_topicCache.end() || !it->resolving || it->runningGeneration != generation)
        return false;
    it->resolving = false;
    it->watchedPaths = watchedPaths;
    updateTopicWatches();
    if (generation != it->generation) {
        requestTopic(repository);
        return false;
    }
    const bool changed = topic && (!it->hasValue || it->topic != *topic);
    it->error = !topic;
    it->upToDate = bool(topic);
    if (topic) {
        it->topic = *topic;
        it->hasValue = true;
    }
    if (changed)
        emit topicChanged(repository);
    return true;
}

void IVersionControl::monitorTopic(const FilePath &repository, QObject *owner)
{
    if (!d->m_asyncTopicRefresher || repository.isEmpty() || !owner)
        return;
    d->m_topicUsers[repository].insert(owner);
    if (!d->m_topicOwners.contains(owner)) {
        d->m_topicOwners.insert(owner, connect(owner, &QObject::destroyed, this, [this, owner] {
            const auto repositories = d->m_topicUsers.keys();
            for (const FilePath &repository : repositories)
                unmonitorTopic(repository, owner);
        }));
    }
    requestTopic(repository);
}

void IVersionControl::unmonitorTopic(const FilePath &repository, QObject *owner)
{
    const auto it = d->m_topicUsers.find(repository);
    if (it == d->m_topicUsers.end() || !it->remove(owner))
        return;
    if (it->isEmpty())
        d->m_topicUsers.erase(it);
    const bool stillUsed = Utils::anyOf(d->m_topicUsers, [owner](const QSet<QObject *> &users) {
        return users.contains(owner);
    });
    if (!stillUsed)
        disconnect(d->m_topicOwners.take(owner));
}

void IVersionControl::reconcileTopics()
{
    if (!d->m_asyncTopicRefresher)
        return;
    const auto repositories = d->m_topicCache.keys();
    for (const FilePath &repository : repositories) {
        const auto it = d->m_topicCache.find(repository);
        if (it->resolving)
            continue;
        const bool hasDocument = Utils::anyOf(DocumentModel::openedDocuments(),
            [&repository](IDocument *document) {
                return document->filePath().isChildOf(repository);
            });
        if (it->interest.hasExpired() && !d->m_topicUsers.contains(repository) && !hasDocument) {
            d->m_pendingRefreshes.remove(repository);
            d->m_topicCache.erase(it);
            continue;
        }
        invalidateTopic(repository);
        requestTopic(repository);
    }
    updateTopicWatches();
}

void IVersionControl::updateTopicWatches()
{
    QSet<QString> desired;
    for (const auto &data : std::as_const(d->m_topicCache)) {
        for (const FilePath &path : data.watchedPaths) {
            const QString file = path.toFSPathString();
            desired.insert(file);
        }
    }
    QFileSystemWatcher *watcher = d->m_watcher;
    if (!watcher)
        return;
    QMetaObject::invokeMethod(watcher, [this, watcher, desired] {
        const QStringList paths = watcher->files() + watcher->directories();
        const QSet<QString> watched(paths.cbegin(), paths.cend());
        for (const QString &path : watched) {
            if (!desired.contains(path))
                watcher->removePath(path);
        }
        QSet<QString> added;
        for (const QString &path : desired) {
            if (!watched.contains(path) && watcher->addPath(path))
                added.insert(path);
        }
        if (added.isEmpty())
            return;
        // Recheck owners after installing their watches to close the registration gap.
        QMetaObject::invokeMethod(this, [this, added = std::move(added)] {
            FilePaths affected;
            for (auto it = d->m_topicCache.cbegin(); it != d->m_topicCache.cend(); ++it) {
                if (Utils::anyOf(it->watchedPaths, [&added](const FilePath &path) {
                        return added.contains(path.toFSPathString());
                    })) {
                    affected.append(it.key());
                }
            }
            for (const FilePath &repository : affected) {
                invalidateTopic(repository, TopicRefresh::ReuseDirectories);
                d->m_pendingRefreshes.insert(repository);
            }
            if (!affected.isEmpty())
                d->m_debounceTimer.start();
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

FilePaths IVersionControl::unmanagedFiles(const FilePaths &filePaths) const
{
    return Utils::filtered(filePaths, [this](const FilePath &fp) {
        return !managesFile(fp.parentDir(), fp.fileName());
    });
}

IVersionControl::OpenSupportMode IVersionControl::openSupportMode(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return NoOpen;
}
void IVersionControl::fillLinkContextMenu(QMenu *, const FilePath &, const QString &)
{
}

bool IVersionControl::handleLink(const FilePath &workingDirectory, const QString &reference)
{
    QTC_ASSERT(!reference.isEmpty(), return false);
    vcsDescribe(workingDirectory, reference);
    return true;
}

} // namespace Core
