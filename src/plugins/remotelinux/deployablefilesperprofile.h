/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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
**
**************************************************************************/

#ifndef DEPLOYABLEFILESPERPROFILE_H
#define DEPLOYABLEFILESPERPROFILE_H

#include "remotelinux_export.h"

#include <qt4projectmanager/qt4nodes.h>

#include <QAbstractTableModel>
#include <QList>
#include <QString>

namespace RemoteLinux {
class DeployableFile;

namespace Internal {
class DeployableFilesPerProFilePrivate;
}

class REMOTELINUX_EXPORT DeployableFilesPerProFile : public QAbstractTableModel
{
    Q_OBJECT
public:
    DeployableFilesPerProFile(const Qt4ProjectManager::Qt4ProFileNode *proFileNode,
        const QString &installPrefix, QObject *parent);
    ~DeployableFilesPerProFile();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    DeployableFile deployableAt(int row) const;
    bool isModified() const;
    void setUnModified();
    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString projectName() const;
    QString projectDir() const;
    QString proFilePath() const;
    Qt4ProjectManager::Qt4ProjectType projectType() const;
    bool isApplicationProject() const { return projectType() == Qt4ProjectManager::ApplicationTemplate; }
    QString applicationName() const;
    bool hasTargetPath() const;

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    QStringList localLibraryFilePaths() const;

    Internal::DeployableFilesPerProFilePrivate * const d;
};

} // namespace RemoteLinux

#endif // DEPLOYABLEFILESPERPROFILE_H
