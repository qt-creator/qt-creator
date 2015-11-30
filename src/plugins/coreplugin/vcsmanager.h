/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSMANAGER_H
#define VCSMANAGER_H

#include "core_global.h"

#include <QString>
#include <QObject>

namespace Core {

class Id;
class IVersionControl;

namespace Internal { class MainWindow; }

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

    static QList<IVersionControl *> versionControls();
    static IVersionControl *versionControl(Id id);

    static void resetVersionControlForDirectory(const QString &inputDirectory);
    static IVersionControl *findVersionControlForDirectory(const QString &directory,
                                                           QString *topLevelDirectory = 0);
    static QString findTopLevelForDirectory(const QString &directory);

    static QStringList repositories(const IVersionControl *);

    // Shows a confirmation dialog, whether the file should also be deleted
    // from revision control. Calls vcsDelete on the file. Returns false
    // if a failure occurs
    static bool promptToDelete(const QString &fileName);
    static bool promptToDelete(IVersionControl *versionControl, const QString &fileName);

    // Shows a confirmation dialog, whether the files in the list should be
    // added to revision control. Calls vcsAdd for each file.
    static void promptToAdd(const QString &directory, const QStringList &fileNames);

    static void emitRepositoryChanged(const QString &repository);

    // Utility messages for adding files
    static QString msgAddToVcsTitle();
    static QString msgPromptToAddToVcs(const QStringList &files, const IVersionControl *vc);
    static QString msgAddToVcsFailedTitle();
    static QString msgToAddToVcsFailed(const QStringList &files, const IVersionControl *vc);

    /*!
     * Return a list of paths where tools that came with the VCS may be installed.
     * This is helpful on windows where e.g. git comes with a lot of nice unix tools.
     */
    static QStringList additionalToolsPath();

signals:
    void repositoryChanged(const QString &repository);
    void configurationChanged(const IVersionControl *vcs);

public slots:
    static void clearVersionControlCache();

private slots:
    void handleConfigurationChanges();

private:
    explicit VcsManager(QObject *parent = 0);
    ~VcsManager();

    friend class Core::Internal::MainWindow;
};

} // namespace Core

#endif // VCSMANAGER_H
