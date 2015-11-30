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

#include "vcsprojectcache.h"

#include "vcsbasesubmiteditor.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <limits>

namespace {

class PathMatcher
{
public:
    PathMatcher() : m_count(std::numeric_limits<int>::max()), m_project(0) { }
    ProjectExplorer::Project *project() { return m_project; }

    void match(ProjectExplorer::Project *project,
               const Utils::FileName &base, const Utils::FileName &child) {
        int count = std::numeric_limits<int>::max();
        if (child.isChildOf(base)) {
            const QString relative = child.toString().mid(base.count() + 1);
            count = relative.count(QLatin1Char('/'));
        }
        if (count < m_count) {
            m_count = count;
            m_project = project;
        }
    }

private:
    int m_count;
    ProjectExplorer::Project *m_project;
};

} // namespace

namespace VcsBase {
namespace Internal {

VcsProjectCache *VcsProjectCache::m_instance = 0;

VcsProjectCache::VcsProjectCache()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::projectAdded,
            this, [this]() { VcsProjectCache::invalidate(); });
    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::projectRemoved,
            this, [this]() { VcsProjectCache::invalidate(); });
}

VcsProjectCache::~VcsProjectCache()
{
    m_instance = 0;
}

ProjectExplorer::Project *VcsProjectCache::projectFor(const QString &repo)
{
    ProjectExplorer::Project *project;

    const int pos = Utils::indexOf(m_instance->m_cache,
                                   [repo](const CacheNode &n) { return n.repository == repo; });
    if (pos >= 0) {
        if (pos > 0) {
            m_instance->m_cache.prepend(m_instance->m_cache.at(pos));
            m_instance->m_cache.removeAt(pos + 1);
        }
        return m_instance->m_cache.at(0).project;
    }

    project = projectForToplevel(Utils::FileName::fromString(repo));
    m_instance->m_cache.prepend(CacheNode(repo, project));
    while (m_instance->m_cache.count() > 10)
        m_instance->m_cache.removeLast();

    return project;
}

void VcsProjectCache::invalidate()
{
    m_instance->m_cache.clear();
}

void VcsProjectCache::create()
{
    new VcsProjectCache;
}

void VcsProjectCache::destroy()
{
    delete m_instance;
}

ProjectExplorer::Project *VcsProjectCache::projectForToplevel(const Utils::FileName &vcsTopLevel)
{
    PathMatcher parentMatcher;
    PathMatcher childMatcher;
    foreach (ProjectExplorer::Project *project, ProjectExplorer::SessionManager::projects()) {
        const Utils::FileName projectDir = project->projectDirectory();
        if (projectDir == vcsTopLevel)
            return project;
        parentMatcher.match(project, vcsTopLevel, projectDir);
        childMatcher.match(project, projectDir, vcsTopLevel);
    }

    if (parentMatcher.project())
        return parentMatcher.project();

    return childMatcher.project();
}

} // Internal
} // namespace VcsBase
