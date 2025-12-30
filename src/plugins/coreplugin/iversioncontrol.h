// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "vcsfilestate.h"

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

    IVersionControl();
    ~IVersionControl() override;

    virtual QString displayName() const = 0;
    virtual Utils::Id id() const = 0;

    /*!
     * Returns true if \a filePath is a file or directory that is maintained by this VCS.
     *
     * It will return true only for exact matches of the name, not for e.g. files in a
     * directory owned by the version control system (e.g. .git/control).
     *
     * \note Implementations of this method must be thread safe!
     */
    virtual bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const = 0;

    /*!
     * Returns whether files in directory \a filePath are managed by this VCS.
     *
     * If \a topLevel is non-null, it returns the topmost directory, for which
     * this VCS should be used. The VcsManager assumes that all files in the
     * returned directory are managed by the same VCS.
     */
    virtual bool managesDirectory(const Utils::FilePath &filePath,
                                  Utils::FilePath *topLevel = nullptr) const = 0;

    /*!
     * Returns true if \a fileName within \a workingDirectory is managed by this VCS.
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
     * Returns true if the VCS is configured to run.
     */
    virtual bool isConfigured() const = 0;

    /*!
     * Starts or stops monitoring (depending on \a monitor) modified files inside \a path.
     */
    virtual Utils::FilePaths monitorDirectory(const Utils::FilePath &path, bool monitor);

    /*!
     * Called to query whether a VCS supports the respective \a operation.
     *
     * Return false if the VCS is not configured yet.
     */
    virtual bool supportsOperation(Operation operation) const = 0;

    /*!
     * Returns the open support mode for \a filePath.
     */
    virtual OpenSupportMode openSupportMode(const Utils::FilePath &filepath) const;

    /*!
     * Called prior to save, if the file \a filePath is read only.
     *
     * Should be implemented if the VCS requires a operation before editing
     * the file, e.g. 'p4 edit'.
     *
     * \note The EditorManager calls this for the editors.
     */
    virtual bool vcsOpen(const Utils::FilePath &filePath) = 0;

    /*!
     * Returns settings.
     */
    virtual SettingsFlags settingsFlags() const { return {}; }

    /*!
     * Called after the file \a filePath has been added to a project.
     *
     * The implementations should add the file to the version control
     * e.g. 'p4 add', 'cvs add', 'svn add'.
     */
    virtual bool vcsAdd(const Utils::FilePath &filePath) = 0;

    /*!
     * Called after the file \a filePath has been removed from the project.
     *
     * The implementations should remove the file, e.g. 'p4 delete', 'svn delete'.
     */
    virtual bool vcsDelete(const Utils::FilePath &filePath) = 0;

    /*!
     * Called to rename the file \a from to \a to.
     *
     * The implementations should do the actual on disk renaming,
     * e.g. 'git mv', 'svn move', 'p4 move'.
     */
    virtual bool vcsMove(const Utils::FilePath &from, const Utils::FilePath &to) = 0;

    /*!
     * Called to initialize the version control system in \a directory.
     */
    virtual bool vcsCreateRepository(const Utils::FilePath &directory) = 0;

    /*!
     * Returns the topic (e.g. name of the current branch).
     */
    virtual QString vcsTopic(const Utils::FilePath &topLevel);

    /*!
     * Display annotation for \a file and scroll to \a line.
     */
    virtual void vcsAnnotate(const Utils::FilePath &file, int line) = 0;

    /*!
     * Shows the log for the \a relativePath within \a toplevel.
     */
    virtual void vcsLog(const Utils::FilePath &topLevel,
                        const Utils::FilePath &relativePath) = 0;

    /*!
     * Shows the diff for the \a relativePath within \a toplevel.
     */
    virtual void vcsDiff(const Utils::FilePath &topLevel,
                         const Utils::FilePath &relativePath) = 0;

    /*!
     * Returns the display text for Open operation.
     */
    virtual QString vcsOpenText() const;

    /*!
     * Returns the display text for Make Writable.
     */
    virtual QString vcsMakeWritableText() const;

    /*!
     * Display details of \a reference within \a workingDirectory.
     */
    virtual void vcsDescribe(const Utils::FilePath &workingDirectory, const QString &reference) = 0;

    /*!
     * Return a list of paths where tools that came with the VCS may be installed.
     *
     * This is helpful on windows where e.g. git comes with a lot of nice unix tools.
     */
    virtual Utils::FilePaths additionalToolsPath() const;

    /*!
     * Add context \a menu actions for the \a reference within \a workingDirectory.
     *
     * The context menu is requested when the user right clicks in the Version Control pane.
     */
    virtual void fillLinkContextMenu(QMenu *menu,
                                     const Utils::FilePath &workingDirectory,
                                     const QString &reference);

    /*!
     * Handle the \a reference within \a workingDirectory.
     *
     * Typical implementation would be showing the reference.
     */
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
    void setTopicRefresher(const TopicRefresher &topicRefresher);

    static QColor vcStateToColor(const Core::VcsFileState &state);
    static QString modificationToText(const Core::VcsFileState &state);

signals:
    void repositoryChanged(const Utils::FilePath &repository);
    void filesChanged(const Utils::FilePaths &files);
    void configurationChanged();

private:
    Internal::IVersionControlPrivate *d;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IVersionControl::SettingsFlags)
