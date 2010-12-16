/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMODEPLOYABLES_H
#define MAEMODEPLOYABLES_H

#include "maemodeployable.h"
#include "maemodeployablelistmodel.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace ProjectExplorer { class BuildStep; }

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;

namespace Internal {
class Qt4ProFileNode;

class MaemoDeployables : public QAbstractListModel
{
    Q_OBJECT
public:
    MaemoDeployables(const ProjectExplorer::BuildStep *buildStep);
    ~MaemoDeployables();
    void setUnmodified();
    bool isModified() const;
    int deployableCount() const;
    MaemoDeployable deployableAt(int i) const;
    QString remoteExecutableFilePath(const QString &localExecutableFilePath) const;
    int modelCount() const { return m_listModels.count(); }
    MaemoDeployableListModel *modelAt(int i) const { return m_listModels.at(i); }
    const ProjectExplorer::BuildStep *buildStep() const { return m_buildStep; }

private:
    typedef QHash<QString, MaemoDeployableListModel::ProFileUpdateSetting> UpdateSettingsMap;

    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

    Q_SLOT void createModels();
    Q_SLOT void init();
    void createModels(const Qt4ProFileNode *proFileNode);
    const Qt4BuildConfiguration *qt4BuildConfiguration() const;

    QList<MaemoDeployableListModel *> m_listModels;
    UpdateSettingsMap m_updateSettings;
    const ProjectExplorer::BuildStep * const m_buildStep;
    QTimer *const m_updateTimer;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMODEPLOYABLES_H
