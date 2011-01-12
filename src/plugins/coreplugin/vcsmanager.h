/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef VCSMANAGER_H
#define VCSMANAGER_H

#include "core_global.h"

#include <QtCore/QString>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Core {

struct VCSManagerPrivate;
class IVersionControl;

/* VCSManager:
 * 1) Provides functionality for finding the IVersionControl * for a given
 *    filename (findVersionControlForDirectory). Note that the VCSManager assumes
 *    that if a IVersionControl * manages a directory, then it also manages
 *    all the files and all the subdirectories.
 *    It works by asking all IVersionControl * if they manage the file, and ask
 *    for the topmost directory it manages. This information is cached and
 *    VCSManager thus knows pretty fast which IVersionControl * is responsible.
 * 2) Passes on the changes from the version controls caused by updating or
 *    branching repositories and routes them to its signals (repositoryChanged,
 *    filesChanged). */

class CORE_EXPORT VCSManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VCSManager)
public:
    explicit VCSManager(QObject *parent = 0);
    virtual ~VCSManager();

    void extensionsInitialized();

    IVersionControl *findVersionControlForDirectory(const QString &directory,
                                                    QString *topLevelDirectory = 0);
    IVersionControl *checkout(const QString &versionControlType,
                              const QString &directory,
                              const QByteArray &url);
    bool findVersionControl(const QString &versionControl);
    QString getRepositoryURL(const QString &directory);

    // Shows a confirmation dialog, whether the file should also be deleted
    // from revision control Calls sccDelete on the file. Returns false
    // if a failure occurs
    bool promptToDelete(const QString &fileName);
    bool promptToDelete(IVersionControl *versionControl, const QString &fileName);

    friend CORE_EXPORT QDebug operator<<(QDebug in, const VCSManager &);

signals:
    void repositoryChanged(const QString &repository);

private:
    VCSManagerPrivate *m_d;
};

CORE_EXPORT QDebug operator<<(QDebug in, const VCSManager &);

} // namespace Core

#endif // VCSMANAGER_H
