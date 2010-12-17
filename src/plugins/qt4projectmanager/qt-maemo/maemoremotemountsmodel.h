/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MAEMOREMOTEMOUNTSMODEL_H
#define MAEMOREMOTEMOUNTSMODEL_H

#include "maemomountspecification.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoRemoteMountsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MaemoRemoteMountsModel(QObject *parent = 0);
    int mountSpecificationCount() const { return m_mountSpecs.count(); }
    int validMountSpecificationCount() const;
    MaemoMountSpecification mountSpecificationAt(int pos) const { return m_mountSpecs.at(pos); }
    bool hasValidMountSpecifications() const;
    const QList<MaemoMountSpecification> &mountSpecs() const { return m_mountSpecs; }

    void addMountSpecification(const QString &localDir);
    void removeMountSpecificationAt(int pos);
    void setLocalDir(int pos, const QString &localDir);

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    static const int LocalDirRow = 0;
    static const int RemoteMountPointRow = 1;

private:
    virtual int columnCount(const QModelIndex& = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& = QModelIndex()) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
        int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
        int role);

    QList<MaemoMountSpecification> m_mountSpecs;
};

inline int MaemoRemoteMountsModel::columnCount(const QModelIndex &) const
{
    return 2;
}

inline int MaemoRemoteMountsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mountSpecificationCount();
}

inline QModelIndex MaemoRemoteMountsModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEMOUNTSMODEL_H
