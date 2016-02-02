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

#pragma once

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

namespace ProjectExplorer {

class BuildInfo;
class Kit;
class Project;
class Target;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectImporter
{
public:
    ProjectImporter(const QString &path);
    virtual ~ProjectImporter();

    const QString projectFilePath() const { return m_projectPath; }

    virtual QList<BuildInfo *> import(const Utils::FileName &importPath, bool silent = false) = 0;
    virtual QStringList importCandidates(const Utils::FileName &projectFilePath) = 0;
    virtual Target *preferredTarget(const QList<Target *> &possibleTargets) = 0;

    bool isUpdating() const { return m_isUpdating; }

    virtual void markTemporary(Kit *k);
    virtual void makePermanent(Kit *k);

    // Additional cleanup that has to happen when kits are removed
    virtual void cleanupKit(Kit *k);

    void addProject(Kit *k);
    void removeProject(Kit *k, const QString &path);

    bool isTemporaryKit(Kit *k);

protected:
    bool setIsUpdating(bool b) {
        bool old = m_isUpdating;
        m_isUpdating = b;
        return old;
    }

private:
    const QString m_projectPath;
    bool m_isUpdating;
};

} // namespace ProjectExplorer
