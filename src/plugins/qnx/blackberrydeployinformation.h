/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H
#define QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H

#include <QAbstractTableModel>

namespace ProjectExplorer { class Target; }

namespace QmakeProjectManager {
class QmakeProFileNode;
class QmakeProject;
}

namespace Qnx {
namespace Internal {

class BarPackageDeployInformation {
public:
    BarPackageDeployInformation(bool enabled, const QString &proFilePath, const QString &sourceDir,
                                const QString &buildDir, const QString &targetName)
        : enabled(enabled)
        , proFilePath(proFilePath)
        , sourceDir(sourceDir)
        , buildDir(buildDir)
        , targetName(targetName)
    {
    }

    QString appDescriptorPath() const;
    QString packagePath() const;

    bool enabled;
    QString proFilePath;
    QString sourceDir;
    QString buildDir;
    QString targetName;

    QString userAppDescriptorPath;
    QString userPackagePath;
};

class BlackBerryDeployInformation : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BlackBerryDeployInformation(ProjectExplorer::Target *target);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QList<BarPackageDeployInformation> enabledPackages() const;
    QList<BarPackageDeployInformation> allPackages() const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    ProjectExplorer::Target *target() const;

private slots:
    void updateModel();

private:
    enum Columns {
        EnabledColumn = 0,
        AppDescriptorColumn,
        PackageColumn,
        ColumnCount // Always have last
    };

    QmakeProjectManager::QmakeProject *project() const;

    void initModel();
    BarPackageDeployInformation deployInformationFromNode(QmakeProjectManager::QmakeProFileNode *node) const;

    ProjectExplorer::Target *m_target;

    QList<BarPackageDeployInformation> m_deployInformation;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEPLOYINFORMATION_H
