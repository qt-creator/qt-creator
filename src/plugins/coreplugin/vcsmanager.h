/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSMANAGER_H
#define VCSMANAGER_H

#include "core_global.h"

#include <QString>
#include <QObject>

namespace Core {

class VcsManagerPrivate;
class IVersionControl;

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
    explicit VcsManager(QObject *parent = 0);
    virtual ~VcsManager();

    void extensionsInitialized();

    void resetVersionControlForDirectory(const QString &inputDirectory);
    IVersionControl *findVersionControlForDirectory(const QString &directory,
                                                    QString *topLevelDirectory = 0);

    QStringList repositories(const IVersionControl *) const;

    IVersionControl *checkout(const QString &versionControlType,
                              const QString &directory,
                              const QByteArray &url);
    // Used only by Trac plugin.
    bool findVersionControl(const QString &versionControl);
    // Used only by Trac plugin.
    QString repositoryUrl(const QString &directory);

    // Shows a confirmation dialog, whether the file should also be deleted
    // from revision control. Calls vcsDelete on the file. Returns false
    // if a failure occurs
    bool promptToDelete(const QString &fileName);
    bool promptToDelete(IVersionControl *versionControl, const QString &fileName);

    // Shows a confirmation dialog, whether the files in the list should be
    // added to revision control. Calls vcsAdd for each file.
    void promptToAdd(const QString &directory, const QStringList &fileNames);

    // Utility messages for adding files
    static QString msgAddToVcsTitle();
    static QString msgPromptToAddToVcs(const QStringList &files, const IVersionControl *vc);
    static QString msgAddToVcsFailedTitle();
    static QString msgToAddToVcsFailed(const QStringList &files, const IVersionControl *vc);

signals:
    void repositoryChanged(const QString &repository);

private:
    VcsManagerPrivate *d;
};

} // namespace Core

#endif // VCSMANAGER_H
