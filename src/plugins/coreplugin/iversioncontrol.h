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

#include "core_global.h"
#include "id.h"

#include <utils/fileutils.h>

#include <QDateTime>
#include <QFlags>
#include <QHash>
#include <QObject>
#include <QString>

namespace Core {

class ShellCommand;

class CORE_EXPORT IVersionControl : public QObject
{
    Q_OBJECT
    Q_ENUMS(SettingsFlag Operation)
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

    enum OpenSupportMode {
        NoOpen,        /*!< Files can be edited without noticing the VCS */
        OpenOptional,  /*!< Files can be opened by the VCS, or hijacked */
        OpenMandatory  /*!< Files must always be opened by the VCS */
    };

    class CORE_EXPORT TopicCache
    {
    public:
        virtual ~TopicCache();
        QString topic(const QString &topLevel);

    protected:
        virtual QString trackFile(const QString &repository) = 0;
        virtual QString refreshTopic(const QString &repository) = 0;

    private:
        class TopicData
        {
        public:
            QDateTime timeStamp;
            QString topic;
        };

        QHash<QString, TopicData> m_cache;

    };

    explicit IVersionControl(TopicCache *topicCache = 0) : m_topicCache(topicCache) {}
    virtual ~IVersionControl();

    virtual QString displayName() const = 0;
    virtual Id id() const = 0;

    /*!
     * \brief isVcsFileOrDirectory
     * \param fileName
     * \return True if filename is a file or directory that is maintained by the
     * version control system.
     *
     * It will return true only for exact matches of the name, not for e.g. files in a
     * directory owned by the version control system (e.g. .git/control).
     *
     * This method needs to be thread safe!
     */
    virtual bool isVcsFileOrDirectory(const Utils::FileName &fileName) const = 0;

    /*!
     * Returns whether files in this directory should be managed with this
     * version control.
     * If \a topLevel is non-null, it should return the topmost directory,
     * for which this IVersionControl should be used. The VcsManager assumes
     * that all files in the returned directory are managed by the same IVersionControl.
     */

    virtual bool managesDirectory(const QString &filename, QString *topLevel = 0) const = 0;

    /*!
     * Returns whether \a fileName is managed by this version control.
     *
     * \a workingDirectory is assumed to be part of a valid repository (not necessarily its
     * top level). \a fileName is expected to be relative to workingDirectory.
     */
    virtual bool managesFile(const QString &workingDirectory, const QString &fileName) const = 0;

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
     * Returns the open support mode for \a fileName.
     */
    virtual OpenSupportMode openSupportMode(const QString &fileName) const;

    /*!
     * Called prior to save, if the file is read only. Should be implemented if
     * the scc requires a operation before editing the file, e.g. 'p4 edit'
     *
     * \note The EditorManager calls this for the editors.
     */
    virtual bool vcsOpen(const QString &fileName) = 0;

    /*!
     * Returns settings.
     */

    virtual SettingsFlags settingsFlags() const { return 0; }

    /*!
     * Called after a file has been added to a project If the version control
     * needs to know which files it needs to track you should reimplement this
     * function, e.g. 'p4 add', 'cvs add', 'svn add'.
     *
     * \note This function should be called from IProject subclasses after
     *       files are added to the project.
     */
    virtual bool vcsAdd(const QString &filename) = 0;

    /*!
     * Called after a file has been removed from the project (if the user
     * wants), e.g. 'p4 delete', 'svn delete'.
     */
    virtual bool vcsDelete(const QString &filename) = 0;

    /*!
     * Called to rename a file, should do the actual on disk renaming
     * (e.g. git mv, svn move, p4 move)
     */
    virtual bool vcsMove(const QString &from, const QString &to) = 0;

    /*!
     * Called to initialize the version control system in a directory.
     */
    virtual bool vcsCreateRepository(const QString &directory) = 0;

    /*!
     * Topic (e.g. name of the current branch)
     */
    virtual QString vcsTopic(const QString &topLevel);

    /*!
     * Display annotation for a file and scroll to line
     */
    virtual bool vcsAnnotate(const QString &file, int line) = 0;

    /*!
     * Display text for Open operation
     */
    virtual QString vcsOpenText() const;

    /*!
     * Display text for Make Writable
     */
    virtual QString vcsMakeWritableText() const;

    /*!
     * Return a list of paths where tools that came with the VCS may be installed.
     * This is helpful on windows where e.g. git comes with a lot of nice unix tools.
     */
    virtual QStringList additionalToolsPath() const;

    /*!
     * Return a ShellCommand capable of checking out \a url into \a baseDirectory, where
     * a new subdirectory with \a localName will be created.
     *
     * \a extraArgs are passed on to the command being run.
     */
    virtual ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                       const Utils::FileName &baseDirectory,
                                                       const QString &localName,
                                                       const QStringList &extraArgs);

signals:
    void repositoryChanged(const QString &repository);
    void filesChanged(const QStringList &files);
    void configurationChanged();

private:
    TopicCache *m_topicCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IVersionControl::SettingsFlags)

} // namespace Core

#if defined(WITH_TESTS)

#include <QSet>

namespace Core {

class CORE_EXPORT TestVersionControl : public IVersionControl
{
    Q_OBJECT
public:
    TestVersionControl(Id id, const QString &name) :
        m_id(id), m_displayName(name), m_dirCount(0), m_fileCount(0)
    { }
    ~TestVersionControl();

    bool isVcsFileOrDirectory(const Utils::FileName &fileName) const final
    { Q_UNUSED(fileName); return false; }

    void setManagedDirectories(const QHash<QString, QString> &dirs);
    void setManagedFiles(const QSet<QString> &files);

    int dirCount() const { return m_dirCount; }
    int fileCount() const { return m_fileCount; }

    // IVersionControl interface
    QString displayName() const override { return m_displayName; }
    Id id() const override { return m_id; }
    bool managesDirectory(const QString &filename, QString *topLevel) const override;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const override;
    bool isConfigured() const override { return true; }
    bool supportsOperation(Operation) const override { return false; }
    bool vcsOpen(const QString &) override { return false; }
    bool vcsAdd(const QString &) override { return false; }
    bool vcsDelete(const QString &) override { return false; }
    bool vcsMove(const QString &, const QString &) override { return false; }
    bool vcsCreateRepository(const QString &) override { return false; }
    bool vcsAnnotate(const QString &, int) override { return false; }

private:
    Id m_id;
    QString m_displayName;
    QHash<QString, QString> m_managedDirs;
    QSet<QString> m_managedFiles;
    mutable int m_dirCount;
    mutable int m_fileCount;
};

} // namespace Core
#endif
