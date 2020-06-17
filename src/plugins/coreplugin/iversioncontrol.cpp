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

#include "iversioncontrol.h"
#include "vcsmanager.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
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
    \fn Core::IVersionControl::TopicCache::trackFile(const QString &repository)
    Returns the path to the file that invalidates the cache for \a repository when
    the file is modified.

    \fn Core::IVersionControl::TopicCache::refreshTopic(const QString &repository)
    Returns the current topic for \a repository.
 */
namespace Core {

QString IVersionControl::vcsOpenText() const
{
    return tr("Open with VCS (%1)").arg(displayName());
}

QString IVersionControl::vcsMakeWritableText() const
{
    return QString();
}

QStringList IVersionControl::additionalToolsPath() const
{
    return QStringList();
}

ShellCommand *IVersionControl::createInitialCheckoutCommand(const QString &url,
                                                            const Utils::FilePath &baseDirectory,
                                                            const QString &localName,
                                                            const QStringList &extraArgs)
{
    Q_UNUSED(url)
    Q_UNUSED(baseDirectory)
    Q_UNUSED(localName)
    Q_UNUSED(extraArgs)
    return nullptr;
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

QString IVersionControl::vcsTopic(const QString &topLevel)
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

QStringList IVersionControl::unmanagedFiles(const QString &workingDir,
                                            const QStringList &filePaths) const
{
    return Utils::filtered(filePaths, [wd = QDir(workingDir), this](const QString &f) {
        return !managesFile(wd.path(), wd.relativeFilePath(f));
    });
}

IVersionControl::OpenSupportMode IVersionControl::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName)
    return NoOpen;
}

IVersionControl::TopicCache::~TopicCache() = default;

/*!
   Returns the topic for repository under \a topLevel.

   If the cache for \a topLevel is valid, it will be used. Otherwise it will be refreshed.
 */
QString IVersionControl::TopicCache::topic(const QString &topLevel)
{
    QTC_ASSERT(!topLevel.isEmpty(), return QString());
    TopicData &data = m_cache[topLevel];
    QString file = trackFile(topLevel);

    if (file.isEmpty())
        return QString();
    const QDateTime lastModified = QFileInfo(file).lastModified();
    if (lastModified == data.timeStamp)
        return data.topic;
    data.timeStamp = lastModified;
    return data.topic = refreshTopic(topLevel);
}

void IVersionControl::fillLinkContextMenu(QMenu *, const QString &, const QString &)
{
}

bool IVersionControl::handleLink(const QString &workingDirectory, const QString &reference)
{
    QTC_ASSERT(!reference.isEmpty(), return false);
    vcsDescribe(workingDirectory, reference);
    return true;
}

} // namespace Core

#if defined(WITH_TESTS)

#include <QFileInfo>

namespace Core {

TestVersionControl::~TestVersionControl()
{
    VcsManager::clearVersionControlCache();
}

void TestVersionControl::setManagedDirectories(const QHash<QString, QString> &dirs)
{
    m_managedDirs = dirs;
    m_dirCount = 0;
    VcsManager::clearVersionControlCache();
}

void TestVersionControl::setManagedFiles(const QSet<QString> &files)
{
    m_managedFiles = files;
    m_fileCount = 0;
    VcsManager::clearVersionControlCache();
}

bool TestVersionControl::managesDirectory(const QString &filename, QString *topLevel) const
{
    ++m_dirCount;

    if (m_managedDirs.contains(filename)) {
        if (topLevel)
            *topLevel = m_managedDirs.value(filename);
        return true;
    }
    return false;
}

bool TestVersionControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    ++m_fileCount;

    QFileInfo fi(workingDirectory + QLatin1Char('/') + fileName);
    QString dir = fi.absolutePath();
    if (!managesDirectory(dir, nullptr))
        return false;
    QString file = fi.absoluteFilePath();
    return m_managedFiles.contains(file);
}

} // namespace Core
#endif
