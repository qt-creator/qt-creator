// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QString>
#include <QObject>

namespace Core {

class IVersionControl;

namespace Internal { class ICorePrivate; }

/* VcsManager:
 * 1) Provides functionality for finding the IVersionControl * for a given
 *    filename (findVersionControlForDirectory). Note that the VcsManager assumes
 *    that if a IVersionControl * manages a directory, then it also manages
 *    all the files and all the subdirectories.
 *    It works by asking all IVersionControl * if they manage the file, and ask
 *    for the topmost directory it manages. This information is cached and
 *    VCSManager thus knows pretty fast which IVersionControl * is responsible.
 * 2) Passes on the changes from the version controls caused by updating or
 *    branching repositories and routes them to its signals (repositoryChanged,
 *    filesChanged). */

class CORE_EXPORT VcsManager : public QObject
{
    Q_OBJECT

public:
    static VcsManager *instance();

    static void extensionsInitialized();

    static const QList<IVersionControl *> versionControls();
    static IVersionControl *versionControl(Utils::Id id);

    static void resetVersionControlForDirectory(const Utils::FilePath &inputDirectory);
    static IVersionControl *findVersionControlForDirectory(const Utils::FilePath &directory,
                                                           Utils::FilePath *topLevelDirectory = nullptr);
    static Utils::FilePath findTopLevelForDirectory(const Utils::FilePath &directory);

    static Utils::FilePaths repositories(const IVersionControl *versionControl);

    // Shows a confirmation dialog, whether the files should also be deleted
    // from revision control. Calls vcsDelete on the files. Returns the list
    // of files that failed.
    static Utils::FilePaths promptToDelete(const Utils::FilePaths &filePaths);
    static Utils::FilePaths promptToDelete(IVersionControl *versionControl,
                                           const Utils::FilePaths &filePaths);
    static bool promptToDelete(IVersionControl *versionControl, const Utils::FilePath &filePath);

    // Shows a confirmation dialog, whether the files in the list should be
    // added to revision control. Calls vcsAdd for each file.
    static void promptToAdd(const Utils::FilePath &directory, const Utils::FilePaths &filePaths);

    static void emitRepositoryChanged(const Utils::FilePath &repository);

    // Utility messages for adding files
    static QString msgAddToVcsTitle();
    static QString msgPromptToAddToVcs(const QStringList &files, const IVersionControl *vc);
    static QString msgAddToVcsFailedTitle();
    static QString msgToAddToVcsFailed(const QStringList &files, const IVersionControl *vc);

    /*!
     * Return a list of paths where tools that came with the VCS may be installed.
     * This is helpful on windows where e.g. git comes with a lot of nice unix tools.
     */
    static Utils::FilePaths additionalToolsPath();

    static void clearVersionControlCache();

signals:
    void repositoryChanged(const Utils::FilePath &repository);
    void configurationChanged(const IVersionControl *vcs);

private:
    explicit VcsManager(QObject *parent = nullptr);
    ~VcsManager() override;

    void handleConfigurationChanges(IVersionControl *vc);
    static void addVersionControl(IVersionControl *vc);

    friend class Internal::ICorePrivate;
    friend class IVersionControl;
};

} // namespace Core
