/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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
