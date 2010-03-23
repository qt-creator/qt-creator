/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef IVERSIONCONTROL_H
#define IVERSIONCONTROL_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QString>

namespace Core {

class CORE_EXPORT IVersionControl : public QObject
{
    Q_OBJECT
public:
    enum Operation {
        AddOperation, DeleteOperation, OpenOperation,
        CreateRepositoryOperation,
        SnapshotOperations,
        AnnotateOperation
        };

    explicit IVersionControl(QObject *parent = 0) : QObject(parent) {}
    virtual ~IVersionControl() {}

    virtual QString displayName() const = 0;

    /*!
     * Returns whether files in this directory should be managed with this
     * version control.
     */
    virtual bool managesDirectory(const QString &filename) const = 0;

    /*!
     * This function should return the topmost directory, for which this
     * IVersionControl should be used. The VCSManager assumes that all files in
     * the returned directory are managed by the same IVersionControl.
     *
     * Note that this is used as an optimization, so that the VCSManager
     * doesn't need to call managesDirectory(..) for each directory.
     *
     * This function is called after finding out that the directory is managed
     * by a specific version control.
     */
    virtual QString findTopLevelForDirectory(const QString &directory) const = 0;

    /*!
     * Called to query whether a VCS supports the respective operations.
     */
    virtual bool supportsOperation(Operation operation) const = 0;

    /*!
     * Called prior to save, if the file is read only. Should be implemented if
     * the scc requires a operation before editing the file, e.g. 'p4 edit'
     *
     * \note The EditorManager calls this for the editors.
     */
    virtual bool vcsOpen(const QString &fileName) = 0;

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
     * Called to initialize the version control system in a directory.
     */
    virtual bool vcsCreateRepository(const QString &directory) = 0;

    /*!
     * Create a snapshot of the current state and return an identifier or
     * an empty string in case of failure.
     */
    virtual QString vcsCreateSnapshot(const QString &topLevel) = 0;

    /*!
     * List snapshots.
     */
    virtual QStringList vcsSnapshots(const QString &topLevel) = 0;

    /*!
     * Restore a snapshot.
     */
    virtual bool vcsRestoreSnapshot(const QString &topLevel, const QString &name) = 0;

    /*!
     * Remove a snapshot.
     */
    virtual bool vcsRemoveSnapshot(const QString &topLevel, const QString &name) = 0;

    /*!
     * Display annotation for a file and scroll to line
     */
    virtual bool vcsAnnotate(const QString &file, int line) = 0;

signals:
    void repositoryChanged(const QString &repository);
    void filesChanged(const QStringList &files);

    // TODO: ADD A WAY TO DETECT WHETHER A FILE IS MANAGED, e.g
    // virtual bool sccManaged(const QString &filename) = 0;
};

} // namespace Core

#endif // IVERSIONCONTROL_H
