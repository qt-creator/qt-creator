/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "registerhandler.h"
#include "watchdelegatewidgets.h"

#if USE_REGISTER_MODEL_TEST
#include "modeltest.h"
#endif

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// Register
//
//////////////////////////////////////////////////////////////////

enum RegisterType
{
    RegisterUnknown,
    //RegisterDummy,  // like AH if EAX is present.
    RegisterI8,
    RegisterI16,
    RegisterI32,
    RegisterI64,
    RegisterF32,
    RegisterF64,
    RegisterF80,
    RegisterXMM,
    RegisterMMX,
    RegisterNeon,
    RegisterFlags32
};

static struct RegisterNameAndType
{
    const char *name;
    RegisterType type;
} theNameAndType[] = {
    { "eax", RegisterI32 },
    { "ecx", RegisterI32 },
    { "edx", RegisterI32 },
    { "ebx", RegisterI32 },
    { "esp", RegisterI32 },
    { "ebp", RegisterI32 },
    { "esi", RegisterI32 },
    { "edi", RegisterI32 },
    { "eip", RegisterI32 },
    { "eflags", RegisterFlags32 },
    { "cs", RegisterI32 },
    { "ss", RegisterI32 },
    { "ds", RegisterI32 },
    { "es", RegisterI32 },
    { "fs", RegisterI32 },
    { "gs", RegisterI32 },
    { "st0", RegisterF80 },
    { "st1", RegisterF80 },
    { "st2", RegisterF80 },
    { "st3", RegisterF80 },
    { "st4", RegisterF80 },
    { "st5", RegisterF80 },
    { "st6", RegisterF80 },
    { "st7", RegisterF80 },
    { "fctrl", RegisterFlags32 },
    { "fstat", RegisterFlags32 },
    { "ftag", RegisterFlags32 },
    { "fiseg", RegisterFlags32 },
    { "fioff", RegisterFlags32 },
    { "foseg", RegisterFlags32 },
    { "fooff", RegisterFlags32 },
    { "fop", RegisterFlags32 },
    { "xmm0", RegisterXMM },
    { "xmm1", RegisterXMM },
    { "xmm2", RegisterXMM },
    { "xmm3", RegisterXMM },
    { "xmm4", RegisterXMM },
    { "xmm5", RegisterXMM },
    { "xmm6", RegisterXMM },
    { "xmm7", RegisterXMM },
    { "mxcsr", RegisterFlags32 },
    { "orig_eax", RegisterI32 },
    { "al", RegisterI8 },
    { "cl", RegisterI8 },
    { "dl", RegisterI8 },
    { "bl", RegisterI8 },
    { "ah", RegisterI8 },
    { "ch", RegisterI8 },
    { "dh", RegisterI8 },
    { "bh", RegisterI8 },
    { "ax", RegisterI16 },
    { "cx", RegisterI16 },
    { "dx", RegisterI16 },
    { "bx", RegisterI16 },
    { "bp", RegisterI16 },
    { "si", RegisterI16 },
    { "di", RegisterI16 },
    { "mm0", RegisterMMX },
    { "mm1", RegisterMMX },
    { "mm2", RegisterMMX },
    { "mm3", RegisterMMX },
    { "mm4", RegisterMMX },
    { "mm5", RegisterMMX },
    { "mm6", RegisterMMX },
    { "mm7", RegisterMMX }
 };

static RegisterType guessType(const QByteArray &name)
{
    static QHash<QByteArray, RegisterType> theTypes;
    if (theTypes.isEmpty()) {
        for (int i = 0; i != sizeof(theNameAndType) / sizeof(theNameAndType[0]); ++i)
            theTypes[theNameAndType[i].name] = theNameAndType[i].type;
    }
    return theTypes.value(name, RegisterUnknown);
}

static int childCountFromType(int type)
{
    switch (type) {
        case RegisterUnknown: return 0;
        case RegisterI8: return 0;
        case RegisterI16: return 1;
        case RegisterI32: return 2;
        case RegisterI64: return 3;
        case RegisterF32: return 0;
        case RegisterF64: return 0;
        case RegisterF80: return 0;
        case RegisterXMM: return 3;
        case RegisterMMX: return 3;
        case RegisterNeon: return 3;
        case RegisterFlags32: return 0;
    }
    QTC_ASSERT(false, /**/);
    return 0;
}

static int bitWidthFromType(int type, int subType)
{
    const uint integer[] = { 8, 16, 32, 64 };
    const uint xmm[] = { 8, 16, 32, 64 };
    const uint mmx[] = { 8, 16, 32, 64 };
    const uint neon[] = { 8, 16, 32, 64 };

    switch (type) {
        case RegisterUnknown: return 0;
        case RegisterI8: return 8;
        case RegisterI16: return integer[subType];
        case RegisterI32: return integer[subType];
        case RegisterI64: return integer[subType];
        case RegisterF32: return 0;
        case RegisterF64: return 0;
        case RegisterF80: return 0;
        case RegisterXMM: return xmm[subType];
        case RegisterMMX: return mmx[subType];
        case RegisterNeon: return neon[subType];
        case RegisterFlags32: return 0;
    }
    QTC_ASSERT(false, /**/);
    return 0;
}

Register::Register(const QByteArray &name_)
    : name(name_), changed(true)
{
    type = guessType(name);
}


//////////////////////////////////////////////////////////////////
//
// RegisterHandler
//
//////////////////////////////////////////////////////////////////

RegisterHandler::RegisterHandler()
{
    m_base = 16;
    calculateWidth();
#if USE_REGISTER_MODEL_TEST
    new ModelTest(this, 0);
#endif
}

int RegisterHandler::rowCount(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return 0;
    if (!idx.isValid())
        return m_registers.size(); // Top level.
    if (idx.internalId() >= 0)
        return 0; // Sub-Items don't have children.
    if (idx.row() >= m_registers.size())
        return 0;
    return childCountFromType(m_registers[idx.row()].type);
}

int RegisterHandler::columnCount(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return 0;
    if (!idx.isValid())
        return 2;
    if (idx.internalId() >= 0)
        return 0; // Sub-Items don't have children.
    return 2;
}

QModelIndex RegisterHandler::index(int row, int col, const QModelIndex &parent) const
{
    if (row < 0 || col < 0 || col >= 2)
        return QModelIndex();
    if (!parent.isValid()) // Top level.
        return createIndex(row, col, -1);
    if (parent.internalId() >= 0) // Sub-Item has no children.
        return QModelIndex();
    if (parent.column() > 0)
        return QModelIndex();
    return createIndex(row, col, parent.row());
}

QModelIndex RegisterHandler::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();
    if (idx.internalId() >= 0)
        return createIndex(idx.internalId(), 0, -1);
    return QModelIndex();
}

// Editor value: Preferably number, else string.
QVariant Register::editValue() const
{
    bool ok = true;
    // Try to convert to number?
    const qulonglong v = value.toULongLong(&ok, 0); // Autodetect format
    if (ok)
        return QVariant(v);
    return QVariant(value);
}

// Editor value: Preferably padded number, else padded string.
QString Register::displayValue(int base, int strlen) const
{
    const QVariant editV = editValue();
    if (editV.type() == QVariant::ULongLong)
        return QString::fromAscii("%1").arg(editV.toULongLong(), strlen, base);
    const QString stringValue = editV.toString();
    if (stringValue.size() < strlen)
        return QString(strlen - stringValue.size(), QLatin1Char(' ')) + value;
    return stringValue;
}

QVariant RegisterHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QModelIndex topLevel = index.parent();
    const int mainRow = topLevel.isValid() ? topLevel.row() : index.row();

    if (mainRow >= m_registers.size())
        return QVariant();


    const Register &reg = m_registers.at(mainRow);

    if (topLevel.isValid()) {
        //
        // Nested
        //
        int subType = index.row();
        int bitWidth = bitWidthFromType(reg.type, subType);

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0: {
                switch (bitWidth) {
                    case 8:  return "[Bytes]";
                    case 16: return "[Words]";
                    case 32: return "[DWords]";
                    case 64: return "[QWords]";
                    case -32: return "[Single]";
                    case -64: return "[Double]";
                    return QVariant(bitWidth);
                }
            }
        }
        default:
            break;
        }

    } else {
        //
        // Toplevel
        //

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0: {
                const QString padding = QLatin1String("  ");
                return QVariant(padding + reg.name + padding);
                //return QVariant(reg.name);
            }
            case 1: // Display: Pad value for alignment
                return reg.displayValue(m_base, m_strlen);
            } // switch column
        case Qt::EditRole: // Edit: Unpadded for editing
            return reg.editValue();
        case Qt::TextAlignmentRole:
            return index.column() == 1 ? QVariant(Qt::AlignRight) : QVariant();
        default:
            break;
        }
    }
    return QVariant();
}

QVariant RegisterHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Value (Base %1)").arg(m_base);
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
    if (idx.column() == 1
            && IntegerWatchLineEdit::isUnsignedHexNumber(m_registers.at(idx.row()).value))
        return notEditable | Qt::ItemIsEditable;
    return notEditable;
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

// Compare register sets by name
static inline bool compareRegisterSet(const Registers &r1, const Registers &r2)
{
    if (r1.size() != r2.size())
        return false;
    const int size = r1.size();
    for (int r = 0; r < size; r++)
        if (r1.at(r).name != r2.at(r).name)
            return false;
    return true;
}

void RegisterHandler::setRegisters(const Registers &registers)
{
    m_registers = registers;
    const int size = m_registers.size();
    for (int r = 0; r < size; r++)
        m_registers[r].changed = false;
    calculateWidth();
    reset();
}

void RegisterHandler::setAndMarkRegisters(const Registers &registers)
{
    if (!compareRegisterSet(m_registers, registers)) {
        setRegisters(registers);
        return;
    }
    const int size = m_registers.size();
    for (int r = 0; r != size; ++r) {
        const QModelIndex regIndex = index(r, 1, QModelIndex());
        if (m_registers.at(r).value != registers.at(r).value) {
            // Indicate red if values change, keep changed.
            m_registers[r].changed = m_registers.at(r).changed
                || !m_registers.at(r).value.isEmpty();
            m_registers[r].value = registers.at(r).value;
            emit dataChanged(regIndex, regIndex);
        }
        emit registerSet(regIndex); // Notify attached memory views.
    }
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

} // namespace Internal
} // namespace Debugger
