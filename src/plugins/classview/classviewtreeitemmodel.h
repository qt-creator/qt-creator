/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

#ifndef CLASSVIEWTREEITEMMODEL_H
#define CLASSVIEWTREEITEMMODEL_H

#include <QStandardItemModel>

namespace ClassView {
namespace Internal {

class TreeItemModelPrivate;

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
    TreeItemModelPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWTREEMODEL_H
