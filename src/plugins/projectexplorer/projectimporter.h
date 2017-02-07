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

#include <coreplugin/id.h>

#include <utils/fileutils.h>

#include <QVariant>

namespace ProjectExplorer {

class BuildInfo;
class Kit;
class Project;
class Target;
class ToolChain;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectImporter : public QObject
{
public:
    struct ToolChainData {
        QList<ToolChain *> tcs;
        bool areTemporary = false;
    };

    ProjectImporter(const Utils::FileName &path);
    virtual ~ProjectImporter();

    const Utils::FileName projectFilePath() const { return m_projectPath; }
    const Utils::FileName projectDirectory() const { return m_projectPath.parentDir(); }

    virtual QList<BuildInfo *> import(const Utils::FileName &importPath, bool silent = false);
    virtual QStringList importCandidates() = 0;
    virtual Target *preferredTarget(const QList<Target *> &possibleTargets);

    bool isUpdating() const { return m_isUpdating; }

    void makePersistent(Kit *k) const;
    void cleanupKit(Kit *k) const;

    bool isTemporaryKit(Kit *k) const;

    void addProject(Kit *k) const;
    void removeProject(Kit *k) const;

protected:
    class UpdateGuard
    {
    public:
        UpdateGuard(const ProjectImporter &i) : m_importer(i)
        {
            m_wasUpdating = m_importer.isUpdating();
            m_importer.m_isUpdating = true;
        }
        ~UpdateGuard() { m_importer.m_isUpdating = m_wasUpdating; }

    private:
        const ProjectImporter &m_importer;
        bool m_wasUpdating;
    };

    // importPath is an existing directory at this point!
    virtual QList<void *> examineDirectory(const Utils::FileName &importPath) const = 0;
    // will get one of the results from examineDirectory
    virtual bool matchKit(void *directoryData, const Kit *k) const = 0;
    // will get one of the results from examineDirectory
    virtual Kit *createKit(void *directoryData) const = 0;
    // will get one of the results from examineDirectory
    virtual QList<BuildInfo *> buildInfoListForKit(const Kit *k, void *directoryData) const = 0;

    virtual void deleteDirectoryData(void *directoryData) const = 0;

    using KitSetupFunction = std::function<void(Kit *)>;
    ProjectExplorer::Kit *createTemporaryKit(const KitSetupFunction &setup) const;

    // Handle temporary additions to Kits (Qt Versions, ToolChains, etc.)
    using CleanupFunction = std::function<void(Kit *, const QVariantList &)>;
    using PersistFunction = std::function<void(Kit *, const QVariantList &)>;
    void useTemporaryKitInformation(Core::Id id,
                                    CleanupFunction cleanup, PersistFunction persist);
    void addTemporaryData(Core::Id id, const QVariant &cleanupData, Kit *k) const;
    // Does *any* kit feature the requested data yet?
    bool hasKitWithTemporaryData(Core::Id id, const QVariant &data) const;

    ToolChainData findOrCreateToolChains(const Utils::FileName &toolChainPath, const Core::Id &language) const;

private:
    void markKitAsTemporary(Kit *k) const;
    bool findTemporaryHandler(Core::Id id) const;

    void cleanupTemporaryToolChains(ProjectExplorer::Kit *k, const QVariantList &vl);
    void persistTemporaryToolChains(ProjectExplorer::Kit *k, const QVariantList &vl);

    const Utils::FileName m_projectPath;
    mutable bool m_isUpdating = false;

    class TemporaryInformationHandler {
    public:
        Core::Id id;
        CleanupFunction cleanup;
        PersistFunction persist;
    };
    QList<TemporaryInformationHandler> m_temporaryHandlers;

    friend class UpdateGuard;
};

} // namespace ProjectExplorer
