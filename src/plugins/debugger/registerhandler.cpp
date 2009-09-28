/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "registerhandler.h"

#include "debuggerconstants.h"

#include <utils/qtcassert.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>

#include <QtGui/QColor>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;


//////////////////////////////////////////////////////////////////
//
// RegisterHandler
//
//////////////////////////////////////////////////////////////////

RegisterHandler::RegisterHandler(QObject *parent)
    : QAbstractTableModel(parent)
{
    setNumberBase(16);
}

int RegisterHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_registers.size();
}

int RegisterHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant RegisterHandler::data(const QModelIndex &index, int role) const
{
    if (role == RegisterNumberBaseRole)
        return m_base;

    if (!index.isValid() || index.row() >= m_registers.size())
        return QVariant();

    const Register &reg = m_registers.at(index.row());

    if (role == RegisterAddressRole) {
        // Return some address associated with the register.
        bool ok = true;
        qulonglong value = reg.value.toULongLong(&ok, 0);
        return ok ? QString::fromLatin1("0x") + QString::number(value, 16) : QVariant();
    }

    const QString padding = "  ";
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return padding + reg.name + padding;
            case 1: {
                bool ok = true;
                qulonglong value = reg.value.toULongLong(&ok, 0);
                QString res = ok ? QString::number(value, m_base) : reg.value;
                return QString(m_strlen - res.size(), QLatin1Char(' ')) + res;
            }
        }
    }

    if (role == Qt::TextAlignmentRole && index.column() == 1)
        return Qt::AlignRight;

    if (role == RegisterChangedRole)
        return reg.changed;
    
    return QVariant();
}

QVariant RegisterHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name");
            case 1: return tr("Value (base %1)").arg(m_base);
        };
    }
    return QVariant();
}

Qt::ItemFlags RegisterHandler::flags(const QModelIndex &idx) const
{
    using namespace Qt;

    if (!idx.isValid())
        return ItemFlags();

    static const ItemFlags notEditable =
          ItemIsSelectable
        | ItemIsDragEnabled
        | ItemIsDropEnabled
        | ItemIsEnabled;

    static const ItemFlags editable = notEditable | ItemIsEditable;

    if (idx.column() == 1)
        return editable; // locals and watcher values are editable
    return  notEditable;
}

void RegisterHandler::removeAll()
{
    m_registers.clear();
    reset();
}

bool RegisterHandler::isEmpty() const
{
    return m_registers.isEmpty();
}

void RegisterHandler::setRegisters(const QList<Register> &registers)
{
    m_registers = registers;
    reset();
}

QList<Register> RegisterHandler::registers() const
{
    return m_registers;
}

void RegisterHandler::setNumberBase(int base)
{
    m_base = base;
    m_strlen = (base == 2 ? 64 : base == 8 ? 32 : base == 10 ? 26 : 16);
    emit reset();
}
