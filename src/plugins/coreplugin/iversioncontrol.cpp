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

/*!
    \class Core::IVersionControl::TopicCache
    \inheaderfile coreplugin/iversioncontrol.h
    \inmodule QtCreator

    \brief The TopicCache class stores a cache which maps a directory to a topic.

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
 */

/*!
    \fn Utils::FilePath Core::IVersionControl::TopicCache::trackFile(const Utils::FilePath &repository)
    Returns the path to the file that invalidates the cache for \a repository when
    the file is modified.

    \fn QString Core::IVersionControl::TopicCache::refreshTopic(const Utils::FilePath &repository)
    Returns the current topic for \a repository.
 */

using namespace Utils;

namespace Core {

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

void IVersionControl::setTopicCache(TopicCache *topicCache)
{
    m_topicCache = topicCache;
}

QString IVersionControl::vcsTopic(const FilePath &topLevel)
{
    return m_topicCache ? m_topicCache->topic(topLevel) : QString();
}

IVersionControl::IVersionControl()
{
    Core::VcsManager::addVersionControl(this);
}

IVersionControl::~IVersionControl()
{
    delete m_topicCache;
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

IVersionControl::TopicCache::~TopicCache() = default;

/*!
   Returns the topic for repository under \a topLevel.

   If the cache for \a topLevel is valid, it will be used. Otherwise it will be refreshed.
 */
QString IVersionControl::TopicCache::topic(const FilePath &topLevel)
{
    QTC_ASSERT(!topLevel.isEmpty(), return QString());
    TopicData &data = m_cache[topLevel];
    const FilePath file = trackFile(topLevel);

    if (file.isEmpty())
        return QString();
    const QDateTime lastModified = file.lastModified();
    if (lastModified == data.timeStamp)
        return data.topic;
    data.timeStamp = lastModified;
    return data.topic = refreshTopic(topLevel);
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

#if defined(WITH_TESTS)

namespace Core {

TestVersionControl::~TestVersionControl()
{
    VcsManager::clearVersionControlCache();
}

void TestVersionControl::setManagedDirectories(const QHash<FilePath, FilePath> &dirs)
{
    m_managedDirs = dirs;
    m_dirCount = 0;
    VcsManager::clearVersionControlCache();
}

void TestVersionControl::setManagedFiles(const QSet<FilePath> &files)
{
    m_managedFiles = files;
    m_fileCount = 0;
    VcsManager::clearVersionControlCache();
}

bool TestVersionControl::managesDirectory(const FilePath &filePath, FilePath *topLevel) const
{
    ++m_dirCount;

    if (m_managedDirs.contains(filePath)) {
        if (topLevel)
            *topLevel = m_managedDirs.value(filePath);
        return true;
    }
    return false;
}

bool TestVersionControl::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    ++m_fileCount;

    FilePath full = workingDirectory.pathAppended(fileName);
    if (!managesDirectory(full.parentDir(), nullptr))
        return false;
    return m_managedFiles.contains(full.absoluteFilePath());
}

} // namespace Core
#endif
