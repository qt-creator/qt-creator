/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMOPACKAGECONTENTS_H
#define MAEMOPACKAGECONTENTS_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace Qt4ProjectManager {
namespace Internal {

struct MaemoDeployable
{
    MaemoDeployable(const QString &localFilePath, const QString &remoteFilePath)
        : localFilePath(localFilePath), remoteFilePath(remoteFilePath) {}

    bool operator==(const MaemoDeployable &other) const
    {
        return localFilePath == other.localFilePath
            && remoteFilePath == other.remoteFilePath;
    }

    QString localFilePath;
    QString remoteFilePath;
};
inline uint qHash(const MaemoDeployable &d)
{
    return qHash(qMakePair(d.localFilePath, d.remoteFilePath));
}


class MaemoPackageCreationStep;

class MaemoPackageContents : public QAbstractTableModel
{
    Q_OBJECT
public:
    MaemoPackageContents(MaemoPackageCreationStep *packageStep);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    MaemoDeployable deployableAt(int row) const;
    bool addDeployable(const MaemoDeployable &deployable);
    void removeDeployableAt(int row);
    bool isModified() const { return m_modified; }
    void setUnModified() { m_modified = false; }
    QString remoteExecutableFilePath() const;

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);

private:
    const MaemoPackageCreationStep * const m_packageStep;
    QList<MaemoDeployable> m_deployables;
    bool m_modified;
    mutable QString m_remoteExecutableFilePath;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMOPACKAGECONTENTS_H
