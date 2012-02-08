/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CONSOLEITEMMODEL_H
#define CONSOLEITEMMODEL_H

#include <QAbstractItemModel>
#include <QtGui/QItemSelectionModel>

namespace Debugger {
namespace Internal {

class ConsoleItem;
class ConsoleItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ItemType { InputType, LogType, WarningType, ErrorType,
                    UndefinedType //Can be used for unknown and for Return values
                  };
    enum Roles { TypeRole = Qt::UserRole };

    explicit ConsoleItemModel(QObject *parent = 0);
    ~ConsoleItemModel();

    void appendItem(ItemType,QString);

public slots:
    void clear();
    void appendEditableRow();

signals:
    void editableRowAppended(const QModelIndex &index,
                             QItemSelectionModel::SelectionFlags flags);

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());

    ConsoleItem *getItem(const QModelIndex &index) const;

private:
    ConsoleItem *m_rootItem;
};

} //Internal
} //Debugger

#endif // CONSOLEITEMMODEL_H
