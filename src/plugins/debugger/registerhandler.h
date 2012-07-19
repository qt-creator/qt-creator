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

#ifndef DEBUGGER_REGISTERHANDLER_H
#define DEBUGGER_REGISTERHANDLER_H

#include <QAbstractTableModel>
#include <QVector>

namespace Debugger {
namespace Internal {

class Register
{
public:
    Register() : type(0), changed(true) {}
    Register(const QByteArray &name_);

    QVariant editValue() const;
    QString displayValue(int base, int strlen) const;

public:
    QByteArray name;
    /* Value should be an integer for which autodetection by passing
     * base=0 to QString::toULongLong() should work (C-language conventions).
     * Values that cannot be converted (such as 128bit MMX-registers) are
     * passed through. */
    QByteArray value;
    int type;
    bool changed;
};

typedef QVector<Register> Registers;

class RegisterHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    RegisterHandler();

    QAbstractItemModel *model() { return this; }

    bool isEmpty() const; // nothing known so far?
    // Set up register names (gdb)
    void setRegisters(const Registers &registers);
    // Set register values
    void setAndMarkRegisters(const Registers &registers);
    Registers registers() const;
    Register registerAt(int i) const { return m_registers.at(i); }
    void removeAll();
    Q_SLOT void setNumberBase(int base);
    int numberBase() const { return m_base; }

signals:
    void registerSet(const QModelIndex &r); // Register was set, for memory views

private:
    void calculateWidth();
    int rowCount(const QModelIndex &idx = QModelIndex()) const;
    int columnCount(const QModelIndex &idx = QModelIndex()) const;
    QModelIndex index(int row, int col, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &idx) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;

    Registers m_registers;
    int m_base;
    int m_strlen; // approximate width of a value in chars.
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_REGISTERHANDLER_H
