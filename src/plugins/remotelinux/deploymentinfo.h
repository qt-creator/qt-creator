/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef DEPLOYMENTINFO_H
#define DEPLOYMENTINFO_H

#include "deployablefilesperprofile.h"
#include "remotelinux_export.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4BaseTarget;
class Qt4ProFileNode;
} // namespace Qt4ProjectManager

namespace RemoteLinux {
class DeployableFile;

class REMOTELINUX_EXPORT DeploymentInfo : public QAbstractListModel
{
    Q_OBJECT
public:
    DeploymentInfo(const Qt4ProjectManager::Qt4BaseTarget *target);
    ~DeploymentInfo();
    void setUnmodified();
    bool isModified() const;
    int deployableCount() const;
    DeployableFile deployableAt(int i) const;
    QString remoteExecutableFilePath(const QString &localExecutableFilePath) const;
    int modelCount() const { return m_listModels.count(); }
    DeployableFilesPerProFile *modelAt(int i) const { return m_listModels.at(i); }

private slots:
    void startTimer(Qt4ProjectManager::Qt4ProFileNode *, bool success, bool parseInProgress);

private:
    typedef QHash<QString, DeployableFilesPerProFile::ProFileUpdateSetting> UpdateSettingsMap;

    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

    Q_SLOT void createModels();
    void createModels(const Qt4ProjectManager::Qt4ProFileNode *proFileNode);

    QList<DeployableFilesPerProFile *> m_listModels;
    UpdateSettingsMap m_updateSettings;
    const Qt4ProjectManager::Qt4BaseTarget * const m_target;
    QTimer *const m_updateTimer;
};

} // namespace RemoteLinux

#endif // DEPLOYMENTINFO_H
