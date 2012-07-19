/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QTMESSAGELOGHANDLER_H
#define QTMESSAGELOGHANDLER_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QFont>

namespace Debugger {
namespace Internal {

class QtMessageLogItem;
class QtMessageLogHandler : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ItemType
    {
    InputType = 0x01,
    DebugType = 0x02,
    WarningType = 0x04,
    ErrorType = 0x08,
    UndefinedType = 0x10, //Can be used for unknown and for Return values
    DefaultTypes = InputType | UndefinedType
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    enum Roles { TypeRole = Qt::UserRole, FileRole, LineRole };

    explicit QtMessageLogHandler(QObject *parent = 0);
    ~QtMessageLogHandler();

    void setHasEditableRow(bool hasEditableRow);
    bool hasEditableRow() const;
    void appendEditableRow();
    void removeEditableRow();

    bool appendItem(QtMessageLogItem *item, int position = -1);
    bool appendMessage(QtMessageLogHandler::ItemType itemType,
                       const QString &message, int position = -1);

    QAbstractItemModel *model() { return this; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);

    QtMessageLogItem *root() const { return m_rootItem; }

public slots:
    void clear();

signals:
    void selectEditableRow(const QModelIndex &index,
                             QItemSelectionModel::SelectionFlags flags);
    void rowInserted(const QModelIndex &index);

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;


    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());

    QtMessageLogItem *getItem(const QModelIndex &index) const;

private:
    bool m_hasEditableRow;
    QtMessageLogItem *m_rootItem;
    int m_maxSizeOfFileName;
};

class QtMessageLogItem
{
public:
    QtMessageLogItem(QtMessageLogItem *parent,
                     QtMessageLogHandler::ItemType type = QtMessageLogHandler::UndefinedType,
                     const QString &data = QString());
    ~QtMessageLogItem();

    QtMessageLogItem *child(int number);
    int childCount() const;
    bool insertChildren(int position, int count);
    void insertChild(QtMessageLogItem *item);
    bool insertChild(int position, QtMessageLogItem *item);
    QtMessageLogItem *parent();
    bool removeChildren(int position, int count);
    bool detachChild(int position);
    int childNumber() const;
    void setText(const QString &text);
    const QString &text() const;

private:
    QtMessageLogItem *m_parentItem;
    QList<QtMessageLogItem *> m_childItems;
    QString m_text;

public:
    QtMessageLogHandler::ItemType itemType;
    QString file;
    int line;
};

} //Internal
} //Debugger

#endif // QTMESSAGELOGHANDLER_H
