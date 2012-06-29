/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H
#define QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H

#include <QAbstractTableModel>

namespace Qt4ProjectManager {
class Qt4Project;
}

namespace Qnx {
namespace Internal {

class BarPackageDeployInformation {
public:
    BarPackageDeployInformation(bool enabled, QString appDescriptorPath, QString packagePath, QString proFilePath)
        : enabled(enabled)
        , appDescriptorPath(appDescriptorPath)
        , packagePath(packagePath)
        , proFilePath(proFilePath)
    {
    }

    bool enabled;
    QString appDescriptorPath;
    QString packagePath;
    QString proFilePath;
};

class BlackBerryDeployInformation : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BlackBerryDeployInformation(Qt4ProjectManager::Qt4Project *project);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QList<BarPackageDeployInformation> enabledPackages() const;

private slots:
    void initModel();

private:
    enum Columns {
        EnabledColumn = 0,
        AppDescriptorColumn,
        PackageColumn,
        ColumnCount // Always have last
    };

    Qt4ProjectManager::Qt4Project *m_project;

    QList<BarPackageDeployInformation> m_deployInformation;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H
