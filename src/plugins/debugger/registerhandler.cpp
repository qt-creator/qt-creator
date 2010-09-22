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

#include "registerhandler.h"

#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"
#include "watchdelegatewidgets.h"

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

RegisterHandler::RegisterHandler(DebuggerEngine *engine)
  : m_engine(engine)
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

// Editor value: Preferably number, else string.
QVariant RegisterHandler::editValue(const Register &reg)
{
    bool ok = true;
    // Try to convert to number?
    const qulonglong value = reg.value.toULongLong(&ok, 0); // Autodetect format
    if (ok)
        return QVariant(value);
    return QVariant(reg.value);
}

// Editor value: Preferably padded number, else padded string.
inline QString RegisterHandler::displayValue(const Register &reg) const
{
    const QVariant editV = RegisterHandler::editValue(reg);
    if (editV.type() == QVariant::ULongLong)
        return QString::fromAscii("%1").arg(editV.toULongLong(), m_strlen, m_base);
    const QString stringValue = editV.toString();
    if (stringValue.size() < m_strlen)
        return QString(m_strlen - stringValue.size(), QLatin1Char(' ')) + reg.value;
    return stringValue;
}

QVariant RegisterHandler::data(const QModelIndex &index, int role) const
{
    switch (role) {
        case EngineStateRole:
            return m_engine->state();

        case EngineCapabilitiesRole:
            return m_engine->debuggerCapabilities();

        case EngineActionsEnabledRole:
            return m_engine->debuggerActionsEnabled();
    }

    if (!index.isValid() || index.row() >= m_registers.size())
        return QVariant();

    const Register &reg = m_registers.at(index.row());

    switch (role) {
    case RegisterAddressRole: {
        // Return some address associated with the register. Autodetect format
        const QVariant editV = RegisterHandler::editValue(reg);
        return editV.type() == QVariant::ULongLong ? editV : QVariant();
    }

    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: {
            const QString padding = QLatin1String("  ");
            return QVariant(padding + reg.name + padding);
        }
        case 1: // Display: Pad value for alignment
            return displayValue(reg);
        } // switch column
    case Qt::EditRole: // Edit: Unpadded for editing
        return RegisterHandler::editValue(reg);
    case Qt::TextAlignmentRole:
        return index.column() == 1 ? QVariant(Qt::AlignRight) : QVariant();
    case RegisterChangedRole:
        return QVariant(reg.changed);
    case RegisterBigNumberRole: // Editor: Can it be handled as quint64?
        return editValue(reg).type() != QVariant::ULongLong;
    case RegisterNumberBaseRole: // Big integers are assumed to be hexadecimal
        return editValue(reg).type() == QVariant::ULongLong ? m_base : 16;
    default:
        break;
    }
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
    if (!idx.isValid())
        return Qt::ItemFlags();

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    // Can edit registers if they are hex numbers and not arrays.
    if (idx.column() == 1 && IntegerWatchLineEdit::isUnsignedHexNumber(m_registers.at(idx.row()).value))
        return notEditable | Qt::ItemIsEditable;
    return notEditable;
}

bool RegisterHandler::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role) {
        case RequestSetRegisterRole:
            m_engine->setRegisterValue(index.row(), value.toString());
            return true;

        case RequestReloadRegistersRole:
            m_engine->reloadRegisters();
            return true;

        case RequestShowMemoryRole:
            (void) new MemoryViewAgent(m_engine, value.toString());
            return true;

        default:
            return QAbstractTableModel::setData(index, value, role);
    }
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

void RegisterHandler::setRegisters(const Registers &registers)
{
    m_registers = registers;
    calculateWidth();
    reset();
}

void RegisterHandler::setAndMarkRegisters(const Registers &registers)
{
    const Registers old = m_registers;
    m_registers = registers;
    for (int i = qMin(m_registers.size(), old.size()); --i >= 0; )
        m_registers[i].changed = m_registers[i].value != old[i].value;
    calculateWidth();
    reset();
}

Registers RegisterHandler::registers() const
{
    return m_registers;
}

void RegisterHandler::calculateWidth()
{
    m_strlen = (m_base == 2 ? 64 : m_base == 8 ? 32 : m_base == 10 ? 26 : 16);
}

void RegisterHandler::setNumberBase(int base)
{
    if (m_base != base) {
        m_base = base;
        calculateWidth();
        emit reset();
    }
}
