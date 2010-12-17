/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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

#ifndef CLASSVIEWTREEITEMMODEL_H
#define CLASSVIEWTREEITEMMODEL_H

#include <QtGui/QStandardItemModel>
#include <QtCore/QModelIndex>
#include <QtCore/QScopedPointer>
#include <QtCore/QSet>

namespace ClassView {
namespace Internal {

/*!
   \class TreeItemModel
   \brief Model for Class View Tree
 */

class TreeItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit TreeItemModel(QObject *parent=0);
    virtual ~TreeItemModel();

    /*!
       \brief move root item to the target
       \param target Target item
     */
    void moveRootToTarget(const QStandardItem *target);

    //! \implements QStandardItemModel::data
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    //! \implements QStandardItemModel::canFetchMore
    virtual bool canFetchMore(const QModelIndex &parent) const;

    //! \implements QStandardItemModel::fetchMore
    virtual void fetchMore(const QModelIndex &parent);

private:
    //! private class data pointer
    QScopedPointer<struct TreeItemModelPrivate> d_ptr;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWTREEMODEL_H
