/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEPLOYMENTINFO_H
#define DEPLOYMENTINFO_H

#include "remotelinux_export.h"

#include <QAbstractListModel>

namespace ProjectExplorer { class Target; }
namespace Qt4ProjectManager {
class Qt4ProFileNode;
class Qt4Project;
} // namespace Qt4ProjectManager

namespace RemoteLinux {
class DeployableFile;
class DeployableFilesPerProFile;

namespace Internal {
class DeploymentInfoPrivate;
}

class REMOTELINUX_EXPORT DeploymentInfo : public QAbstractListModel
{
    Q_OBJECT
public:
    DeploymentInfo(Qt4ProjectManager::Qt4Project *project, const QString &installPrefix = QString());
    ~DeploymentInfo();

    void setUnmodified();
    bool isModified() const;
    void setInstallPrefix(const QString &installPrefix);
    int deployableCount() const;
    DeployableFile deployableAt(int i) const;
    QString remoteExecutableFilePath(const QString &localExecutableFilePath) const;
    int modelCount() const;
    DeployableFilesPerProFile *modelAt(int i) const;

private slots:
    void createModels();

private:
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

    void createModels(const Qt4ProjectManager::Qt4ProFileNode *proFileNode);

    Internal::DeploymentInfoPrivate * const d;
};

} // namespace RemoteLinux

#endif // DEPLOYMENTINFO_H
