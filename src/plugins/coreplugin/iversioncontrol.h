// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>
#include <utils/filepath.h>

#include <QDateTime>
#include <QFlags>
#include <QHash>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QMenu);

namespace Core {

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

    class CORE_EXPORT TopicCache
    {
    public:
        virtual ~TopicCache();
        QString topic(const Utils::FilePath &topLevel);

    protected:
        virtual Utils::FilePath trackFile(const Utils::FilePath &repository) = 0;
        virtual QString refreshTopic(const Utils::FilePath &repository) = 0;

    private:
        class TopicData
        {
        public:
            QDateTime timeStamp;
            QString topic;
        };

        QHash<Utils::FilePath, TopicData> m_cache;

    };

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

    void setTopicCache(TopicCache *topicCache);

signals:
    void repositoryChanged(const Utils::FilePath &repository);
    void filesChanged(const QStringList &files);
    void configurationChanged();

private:
    TopicCache *m_topicCache = nullptr;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IVersionControl::SettingsFlags)

#if defined(WITH_TESTS)

#include <QSet>

namespace Core {

class CORE_EXPORT TestVersionControl : public IVersionControl
{
    Q_OBJECT
public:
    TestVersionControl(Utils::Id id, const QString &name) :
        m_id(id), m_displayName(name)
    { }
    ~TestVersionControl() override;

    bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const final
    { Q_UNUSED(filePath) return false; }

    void setManagedDirectories(const QHash<Utils::FilePath, Utils::FilePath> &dirs);
    void setManagedFiles(const QSet<Utils::FilePath> &files);

    int dirCount() const { return m_dirCount; }
    int fileCount() const { return m_fileCount; }

    // IVersionControl interface
    QString displayName() const override { return m_displayName; }
    Utils::Id id() const override { return m_id; }
    bool managesDirectory(const Utils::FilePath &filePath, Utils::FilePath *topLevel) const override;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const override;
    bool isConfigured() const override { return true; }
    bool supportsOperation(Operation) const override { return false; }
    bool vcsOpen(const Utils::FilePath &) override { return false; }
    bool vcsAdd(const Utils::FilePath &) override { return false; }
    bool vcsDelete(const Utils::FilePath &) override { return false; }
    bool vcsMove(const Utils::FilePath &, const Utils::FilePath &) override { return false; }
    bool vcsCreateRepository(const Utils::FilePath &) override { return false; }
    void vcsAnnotate(const Utils::FilePath &, int) override {}
    void vcsDescribe(const Utils::FilePath &, const QString &) override {}

private:
    Utils::Id m_id;
    QString m_displayName;
    QHash<Utils::FilePath, Utils::FilePath> m_managedDirs;
    QSet<Utils::FilePath> m_managedFiles;
    mutable int m_dirCount = 0;
    mutable int m_fileCount = 0;
};

} // namespace Core

#endif
