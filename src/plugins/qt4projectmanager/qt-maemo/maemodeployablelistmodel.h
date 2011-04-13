/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMOPACKAGECONTENTS_H
#define MAEMOPACKAGECONTENTS_H

#include "maemodeployable.h"

#include <qt4projectmanager/qt4nodes.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

namespace Qt4ProjectManager {
class QtVersion;
namespace Internal {

class MaemoDeployableListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum ProFileUpdateSetting {
        UpdateProFile, DontUpdateProFile, AskToUpdateProFile
    };

    MaemoDeployableListModel(const Qt4ProFileNode *proFileNode,
        ProFileUpdateSetting updateSetting, QObject *parent);
    ~MaemoDeployableListModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    MaemoDeployable deployableAt(int row) const;
    bool isModified() const { return m_modified; }
    void setUnModified() { m_modified = false; }
    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString projectName() const { return m_projectName; }
    QString projectDir() const;
    QString proFilePath() const { return m_proFilePath; }
    bool isApplicationProject() const { return m_projectType == ApplicationTemplate; }
    QString applicationName() const { return m_targetInfo.target; }
    bool hasTargetPath() const { return m_hasTargetPath; }
    bool canAddDesktopFile() const { return isApplicationProject() && !hasDesktopFile(); }
    QString localDesktopFilePath() const;
    bool hasDesktopFile() const { return !localDesktopFilePath().isEmpty(); }
    bool addDesktopFile(QString &error);
    bool canAddIcon() const { return isApplicationProject() && remoteIconFilePath().isEmpty(); }
    bool addIcon(const QString &fileName, QString &error);
    QString remoteIconFilePath() const;
    ProFileUpdateSetting proFileUpdateSetting() const {
        return m_proFileUpdateSetting;
    }
    void setProFileUpdateSetting(ProFileUpdateSetting updateSetting);

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);

    bool isEditable(const QModelIndex &index) const;
    bool buildModel();
    bool addLinesToProFile(const QStringList &lines);
    const QtVersion *qtVersion() const;
    QString proFileScope() const;
    QString installPrefix() const;

    const Qt4ProjectType m_projectType;
    const QString m_proFilePath;
    const QString m_projectName;
    const TargetInformation m_targetInfo;
    const InstallsList m_installsList;
    const QStringList m_config;
    QList<MaemoDeployable> m_deployables;
    mutable bool m_modified;
    ProFileUpdateSetting m_proFileUpdateSetting;
    bool m_hasTargetPath;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMOPACKAGECONTENTS_H
