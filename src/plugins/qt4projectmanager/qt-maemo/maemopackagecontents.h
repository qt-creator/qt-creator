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
#include <QtCore/QList>
#include <QtCore/QString>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoPackageCreationStep;

class MaemoPackageContents : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Deployable
    {
        Deployable(const QString &localFilePath, const QString &remoteFilePath)
            : localFilePath(localFilePath), remoteFilePath(remoteFilePath) {}
        QString localFilePath;
        QString remoteFilePath;
    };

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    MaemoPackageContents(MaemoPackageCreationStep *packageStep);

    Deployable deployableAt(int row) const;
    bool isModified() const { return m_modified; }
    void setUnModified() { m_modified = false; }

    // TODO: to/from map

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    // TODO: setData


private:
    const MaemoPackageCreationStep * const m_packageStep;
    QList<Deployable> m_deployables;
    bool m_modified;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMOPACKAGECONTENTS_H
