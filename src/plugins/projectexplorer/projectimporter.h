// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "toolchain.h"

#include <utils/fileutils.h>

namespace ProjectExplorer {

class BuildInfo;
class Kit;
class Project;
class Target;
class ToolChain;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectImporter : public QObject
{
    Q_OBJECT
public:
    struct ToolChainData {
        QList<ToolChain *> tcs;
        bool areTemporary = false;
    };

    ProjectImporter(const Utils::FilePath &path);
    ~ProjectImporter() override;

    const Utils::FilePath projectFilePath() const { return m_projectPath; }
    const Utils::FilePath projectDirectory() const { return m_projectPath.parentDir(); }

    virtual const QList<BuildInfo> import(const Utils::FilePath &importPath, bool silent = false);
    virtual Utils::FilePaths importCandidates() = 0;
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
    virtual QList<void *> examineDirectory(const Utils::FilePath &importPath,
                                           QString *warningMessage) const = 0;
    // will get one of the results from examineDirectory
    virtual bool matchKit(void *directoryData, const Kit *k) const = 0;
    // will get one of the results from examineDirectory
    virtual Kit *createKit(void *directoryData) const = 0;
    // will get one of the results from examineDirectory
    virtual const QList<BuildInfo> buildInfoList(void *directoryData) const = 0;

    virtual void deleteDirectoryData(void *directoryData) const = 0;

    using KitSetupFunction = std::function<void(Kit *)>;
    ProjectExplorer::Kit *createTemporaryKit(const KitSetupFunction &setup) const;

    // Handle temporary additions to Kits (Qt Versions, ToolChains, etc.)
    using CleanupFunction = std::function<void(Kit *, const QVariantList &)>;
    using PersistFunction = std::function<void(Kit *, const QVariantList &)>;
    void useTemporaryKitAspect(Utils::Id id,
                                    CleanupFunction cleanup, PersistFunction persist);
    void addTemporaryData(Utils::Id id, const QVariant &cleanupData, Kit *k) const;
    // Does *any* kit feature the requested data yet?
    bool hasKitWithTemporaryData(Utils::Id id, const QVariant &data) const;

    ToolChainData findOrCreateToolChains(const ToolChainDescription &tcd) const;

private:
    void markKitAsTemporary(Kit *k) const;
    bool findTemporaryHandler(Utils::Id id) const;

    void cleanupTemporaryToolChains(ProjectExplorer::Kit *k, const QVariantList &vl);
    void persistTemporaryToolChains(ProjectExplorer::Kit *k, const QVariantList &vl);

    const Utils::FilePath m_projectPath;
    mutable bool m_isUpdating = false;

    class TemporaryInformationHandler {
    public:
        Utils::Id id;
        CleanupFunction cleanup;
        PersistFunction persist;
    };
    QList<TemporaryInformationHandler> m_temporaryHandlers;

    friend class UpdateGuard;
};

} // namespace ProjectExplorer
