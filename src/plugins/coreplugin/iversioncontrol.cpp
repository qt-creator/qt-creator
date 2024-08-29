// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iversioncontrol.h"

#include "coreplugintr.h"
#include "vcsmanager.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QStringList>

using namespace Utils;

namespace Core {

namespace Internal {

class TopicData
{
public:
    QDateTime timeStamp;
    QString topic;
};

class IVersionControlPrivate
{
public:
    IVersionControl::FileTracker m_fileTracker;
    IVersionControl::TopicRefresher m_topicRefresher;
    QHash<FilePath, TopicData> m_topicCache;
};

} // Internal

IVersionControl::IVersionControl()
    : d(new Internal::IVersionControlPrivate)
{
    Core::VcsManager::addVersionControl(this);
}

IVersionControl::~IVersionControl()
{
    delete d;
}

QString IVersionControl::vcsOpenText() const
{
    return Tr::tr("Open with VCS (%1)").arg(displayName());
}

QString IVersionControl::vcsMakeWritableText() const
{
    return QString();
}

FilePaths IVersionControl::additionalToolsPath() const
{
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

QString IVersionControl::refreshTopic(const FilePath &repository)
{
    QTC_ASSERT(d->m_topicRefresher, return {});
    return d->m_topicRefresher(repository);
}

/*!
    Returns the topic for repository under \a topLevel.

    A VCS topic is typically the current active branch name, but it can also have other
    values (for example the latest tag) when there is no active branch.

    It is displayed:
    \list
    \li In the project tree, next to each root project - corresponding to the project.
    \li In the main window title - corresponding to the current active editor.
    \endlist

    In order to enable topic display, an IVersionControl subclass needs to create
    an instance of the TopicCache subclass with appropriate overrides for its
    pure virtual functions, and pass this instance to IVersionControl's constructor.

    The cache tracks a file in the repository, which is expected to change when the
    topic changes. When the file is modified, the cache is refreshed.
    For example: for Git this file is typically <repository>/.git/HEAD

    The base implementation features a cache. If the cache for \a topLevel is valid,
    it will be used. Otherwise it will be refreshed using the items provided by
    \c setTopicFileTracker() and \c setTopicRefresher().

    \sa setTopicFileTracker(), setTopicRefresher()
 */

QString IVersionControl::vcsTopic(const FilePath &topLevel)
{
    QTC_ASSERT(!topLevel.isEmpty(), return QString());
    Internal::TopicData &data = d->m_topicCache[topLevel];
    const FilePath file = trackFile(topLevel);

    if (file.isEmpty())
        return QString();
    const QDateTime lastModified = file.lastModified();
    if (lastModified == data.timeStamp)
        return data.topic;
    data.timeStamp = lastModified;
    return data.topic = refreshTopic(topLevel);
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
    Provides the \a topicRefresher function object for use in \c vscTopic() cache handling.

    The parameter object takes a repository as input and returns its current topic.
 */

void IVersionControl::setTopicRefresher(const TopicRefresher &topicRefresher)
{
    d->m_topicRefresher = topicRefresher;
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
