// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>
#include <utils/filepath.h>

#include <QFlags>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu);

namespace Core {

namespace Internal { class IVersionControlPrivate; }

class CORE_EXPORT IVersionControl : public QObject
{
    Q_OBJECT
public:
    enum SettingsFlag {
        AutoOpen = 0x1
    };
    Q_DECLARE_FLAGS(SettingsFlags, SettingsFlag)

    enum Operation {
        AddOperation, DeleteOperation, MoveOperation,
        CreateRepositoryOperation,
        SnapshotOperations,
        AnnotateOperation,
        InitialCheckoutOperation
    };
    Q_ENUM(SettingsFlag)
    Q_ENUM(Operation)

    enum OpenSupportMode {
        NoOpen,        /*!< Files can be edited without noticing the VCS */
        OpenOptional,  /*!< Files can be opened by the VCS, or hijacked */
        OpenMandatory  /*!< Files must always be opened by the VCS */
    };

    enum class FileState : quint8 {
        Unknown = 0x00,
        Untracked,
        Added,
        Modified,
        Deleted,
        Renamed,
        Unmerged,
    };
    Q_ENUM(FileState)

    IVersionControl();
    ~IVersionControl() override;

    virtual QString displayName() const = 0;
    virtual Utils::Id id() const = 0;

    /*!
     * \brief isVcsFileOrDirectory
     * \param filePath
     * \return True if filePath is a file or directory that is maintained by the
     * version control system.
     *
     * It will return true only for exact matches of the name, not for e.g. files in a
     * directory owned by the version control system (e.g. .git/control).
     *
     * This method needs to be thread safe!
     */
    virtual bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const = 0;

    /*!
     * Returns whether files in this directory should be managed with this
     * version control.
     * If \a topLevel is non-null, it should return the topmost directory,
     * for which this IVersionControl should be used. The VcsManager assumes
     * that all files in the returned directory are managed by the same IVersionControl.
     */

    virtual bool managesDirectory(const Utils::FilePath &filePath,
                                  Utils::FilePath *topLevel = nullptr) const = 0;

    /*!
     * Returns whether \a relativeFilePath is managed by this version control.
     *
     * \a workingDirectory is assumed to be part of a valid repository (not necessarily its
     * top level). \a fileName is expected to be relative to workingDirectory.
     */
    virtual bool managesFile(const Utils::FilePath &workingDirectory,
                             const QString &fileName) const = 0;

    /*!
     * Returns the subset of \a filePaths that is not managed by this version control.
     *
     * The \a filePaths are expected to be absolute paths.
     */
    virtual Utils::FilePaths unmanagedFiles(const Utils::FilePaths &filePaths) const;

    /*!
     * Returns true is the VCS is configured to run.
     */
    virtual bool isConfigured() const = 0;

    /*!
     * Returns true if the file has modification compared to version control
     */
    virtual Core::IVersionControl::FileState modificationState(const Utils::FilePath &path) const;

    /*!
     * Starts monitoring modified files inside path
     */
    virtual void monitorDirectory(const Utils::FilePath &path);

    /*!
     * Stops monitoring modified files inside path
     */
    virtual void stopMonitoringDirectory(const Utils::FilePath &path);

    /*!
     * Called to query whether a VCS supports the respective operations.
     *
     * Return false if the VCS is not configured yet.
     */
    virtual bool supportsOperation(Operation operation) const = 0;

    /*!
     * Returns the open support mode for \a filePath.
     */
    virtual OpenSupportMode openSupportMode(const Utils::FilePath &filepath) const;

    /*!
     * Called prior to save, if the file is read only. Should be implemented if
     * the scc requires a operation before editing the file, e.g. 'p4 edit'
     *
     * \note The EditorManager calls this for the editors.
     */
    virtual bool vcsOpen(const Utils::FilePath &filePath) = 0;

    /*!
     * Returns settings.
     */

    virtual SettingsFlags settingsFlags() const { return {}; }

    /*!
     * Called after a file has been added to a project If the version control
     * needs to know which files it needs to track you should reimplement this
     * function, e.g. 'p4 add', 'cvs add', 'svn add'.
     *
     * \note This function should be called from IProject subclasses after
     *       files are added to the project.
     */
    virtual bool vcsAdd(const Utils::FilePath &filePath) = 0;

    /*!
     * Called after a file has been removed from the project (if the user
     * wants), e.g. 'p4 delete', 'svn delete'.
     */
    virtual bool vcsDelete(const Utils::FilePath &filePath) = 0;

    /*!
     * Called to rename a file, should do the actual on disk renaming
     * (e.g. git mv, svn move, p4 move)
     */
    virtual bool vcsMove(const Utils::FilePath &from, const Utils::FilePath &to) = 0;

    /*!
     * Called to initialize the version control system in a directory.
     */
    virtual bool vcsCreateRepository(const Utils::FilePath &directory) = 0;

    /*!
     * Topic (e.g. name of the current branch)
     */
    virtual QString vcsTopic(const Utils::FilePath &topLevel);

    /*!
     * Display annotation for a file and scroll to line
     */
    virtual void vcsAnnotate(const Utils::FilePath &file, int line) = 0;

    /*!
     * Shows the log for the \a relativeDirectory within \a toplevel.
     */
    virtual void vcsLog(const Utils::FilePath &topLevel,
                        const Utils::FilePath &relativeDirectory) = 0;

    /*!
     * Display text for Open operation
     */
    virtual QString vcsOpenText() const;

    /*!
     * Display text for Make Writable
     */
    virtual QString vcsMakeWritableText() const;

    /*!
     * Display details of reference
     */
    virtual void vcsDescribe(const Utils::FilePath &workingDirectory, const QString &reference) = 0;

    /*!
     * Return a list of paths where tools that came with the VCS may be installed.
     * This is helpful on windows where e.g. git comes with a lot of nice unix tools.
     */
    virtual Utils::FilePaths additionalToolsPath() const;

    virtual void fillLinkContextMenu(QMenu *menu,
                                     const Utils::FilePath &workingDirectory,
                                     const QString &reference);

    virtual bool handleLink(const Utils::FilePath &workingDirectory, const QString &reference);

    class CORE_EXPORT RepoUrl {
    public:
        RepoUrl(const QString &location);

        QString protocol;
        QString userName;
        QString host;
        QString path;
        quint16 port = 0;
        bool isValid = false;
    };
    virtual RepoUrl getRepoUrl(const QString &location) const;

    // Topic cache
    using FileTracker = std::function<Utils::FilePath(const Utils::FilePath &)>;
    Utils::FilePath trackFile(const Utils::FilePath &repository);
    void setTopicFileTracker(const FileTracker &fileTracker);

    using TopicRefresher = std::function<QString(const Utils::FilePath &)>;
    QString refreshTopic(const Utils::FilePath &repository);
    void setTopicRefresher(const TopicRefresher &topicRefresher);

    static QColor vcStateToColor(const IVersionControl::FileState &state);
    static QString modificationToText(const IVersionControl::FileState &state);

signals:
    void repositoryChanged(const Utils::FilePath &repository);
    void filesChanged(const Utils::FilePaths &files);
    void updateFileStatus(const Utils::FilePath &repository, const QStringList &files);
    void clearFileStatus(const Utils::FilePath &repository);
    void configurationChanged();

private:
    Internal::IVersionControlPrivate *d;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IVersionControl::SettingsFlags)
