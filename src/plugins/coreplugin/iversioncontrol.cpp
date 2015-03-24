/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iversioncontrol.h"
#include "vcsmanager.h"

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QStringList>

/*!
    \class Core::IVersionControl::TopicCache
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

QString IVersionControl::vcsTopic(const QString &topLevel)
{
    return m_topicCache ? m_topicCache->topic(topLevel) : QString();
}

IVersionControl::~IVersionControl()
{
    delete m_topicCache;
}

IVersionControl::OpenSupportMode IVersionControl::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName);
    return NoOpen;
}

IVersionControl::TopicCache::~TopicCache()
{
}

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

} // namespace Core

#if defined(WITH_TESTS)

#include <QFileInfo>

namespace Core {

TestVersionControl::~TestVersionControl()
{
    VcsManager::instance()->clearVersionControlCache();
}

void TestVersionControl::setManagedDirectories(const QHash<QString, QString> &dirs)
{
    m_managedDirs = dirs;
    m_dirCount = 0;
    VcsManager::instance()->clearVersionControlCache();
}

void TestVersionControl::setManagedFiles(const QSet<QString> &files)
{
    m_managedFiles = files;
    m_fileCount = 0;
    VcsManager::instance()->clearVersionControlCache();
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
    if (!managesDirectory(dir, 0))
        return false;
    QString file = fi.absoluteFilePath();
    return m_managedFiles.contains(file);
}

} // namespace Core
#endif
