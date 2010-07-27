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

#include "maemodeployable.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class ProFileOption;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class ProFileWrapper;
class Qt4ProFileNode;

class MaemoDeployableListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    MaemoDeployableListModel(const Qt4ProFileNode *proFileNode,
        const QSharedPointer<ProFileOption> &proFileOption, QObject *parent);
    ~MaemoDeployableListModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    MaemoDeployable deployableAt(int row) const;
    bool addDeployable(const MaemoDeployable &deployable, QString *error);
    bool removeDeployableAt(int row, QString *error);
    bool isModified() const { return m_modified; }
    void setUnModified() { m_modified = false; }
    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString projectName() const;
    QString projectDir() const;

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);

    bool buildModel();

    const Qt4ProFileNode * const m_proFileNode;
    QList<MaemoDeployable> m_deployables;
    mutable bool m_modified;
    const QScopedPointer<ProFileWrapper> m_proFileWrapper;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMOPACKAGECONTENTS_H
