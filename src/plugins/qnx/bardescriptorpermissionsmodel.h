/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BARDESCRIPTORPERMISSIONSMODEL_H
#define QNX_INTERNAL_BARDESCRIPTORPERMISSIONSMODEL_H

#include <QAbstractTableModel>

namespace Qnx {
namespace Internal {

class BarDescriptorPermission {
public:
    BarDescriptorPermission(const QString &perm, const QString &ident, const QString &desc)
        : checked(false)
        , permission(perm)
        , identifier(ident)
        , description(desc)
    {
    }

    bool checked;
    QString permission;
    QString identifier;
    QString description;
};

class BarDescriptorPermissionsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BarDescriptorPermissionsModel(QObject *parent = 0);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void checkPermission(const QString &identifier);
    QStringList checkedIdentifiers() const;

public slots:
    void uncheckAll();
    void checkAll();

private:
    enum DataRole {
        IdentifierRole = Qt::UserRole
    };

    void setCheckStateAll(Qt::CheckState checkState);

    void initModel();

    QList<BarDescriptorPermission> m_permissions;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BARDESCRIPTORPERMISSIONSMODEL_H
