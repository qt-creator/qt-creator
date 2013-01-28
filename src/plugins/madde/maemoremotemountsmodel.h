/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MAEMOREMOTEMOUNTSMODEL_H
#define MAEMOREMOTEMOUNTSMODEL_H

#include "maemomountspecification.h"

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QVariantMap>

namespace Madde {
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
} // namespace Madde

#endif // MAEMOREMOTEMOUNTSMODEL_H
