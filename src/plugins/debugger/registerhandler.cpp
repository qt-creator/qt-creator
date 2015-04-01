/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "registerhandler.h"
#include "watchdelegatewidgets.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// Register
//
//////////////////////////////////////////////////////////////////

static struct RegisterNameAndType
{
    const char *name;
    RegisterKind kind;
    int size;
} theNameAndType[] = {
    // ARM
    { "r0", IntegerRegister, 4 },
    { "r1", IntegerRegister, 4 },
    { "r2", IntegerRegister, 4 },
    { "r3", IntegerRegister, 4 },
    { "r4", IntegerRegister, 4 },
    { "r5", IntegerRegister, 4 },
    { "r6", IntegerRegister, 4 },
    { "r7", IntegerRegister, 4 },
    { "r8", IntegerRegister, 4 },
    { "r9", IntegerRegister, 4 },
    { "r10", IntegerRegister, 4 },
    { "r11", IntegerRegister, 4 },
    { "r12", IntegerRegister, 4 },
    { "sp", IntegerRegister, 4 },
    { "lr", IntegerRegister, 4 },
    { "pc", IntegerRegister, 4 },
    { "cpsr", FlagRegister, 4 },
    { "d0", IntegerRegister, 8 },
    { "d1", IntegerRegister, 8 },
    { "d2", IntegerRegister, 8 },
    { "d3", IntegerRegister, 8 },
    { "d4", IntegerRegister, 8 },
    { "d5", IntegerRegister, 8 },
    { "d6", IntegerRegister, 8 },
    { "d7", IntegerRegister, 8 },
    { "d8", IntegerRegister, 8 },
    { "d9", IntegerRegister, 8 },
    { "d10", IntegerRegister, 8 },
    { "d11", IntegerRegister, 8 },
    { "d12", IntegerRegister, 8 },
    { "d13", IntegerRegister, 8 },
    { "d14", IntegerRegister, 8 },
    { "d15", IntegerRegister, 8 },
    { "d16", IntegerRegister, 8 },
    { "d17", IntegerRegister, 8 },
    { "d18", IntegerRegister, 8 },
    { "d19", IntegerRegister, 8 },
    { "d20", IntegerRegister, 8 },
    { "d21", IntegerRegister, 8 },
    { "d22", IntegerRegister, 8 },
    { "d23", IntegerRegister, 8 },
    { "d24", IntegerRegister, 8 },
    { "d25", IntegerRegister, 8 },
    { "d26", IntegerRegister, 8 },
    { "d27", IntegerRegister, 8 },
    { "d28", IntegerRegister, 8 },
    { "d29", IntegerRegister, 8 },
    { "d30", IntegerRegister, 8 },
    { "d31", IntegerRegister, 8 },
    { "fpscr", FlagRegister, 4 },
    { "s0", IntegerRegister, 4 },
    { "s1", IntegerRegister, 4 },
    { "s2", IntegerRegister, 4 },
    { "s3", IntegerRegister, 4 },
    { "s4", IntegerRegister, 4 },
    { "s5", IntegerRegister, 4 },
    { "s6", IntegerRegister, 4 },
    { "s7", IntegerRegister, 4 },
    { "s8", IntegerRegister, 4 },
    { "s9", IntegerRegister, 4 },
    { "s10", IntegerRegister, 4 },
    { "s11", IntegerRegister, 4 },
    { "s12", IntegerRegister, 4 },
    { "s13", IntegerRegister, 4 },
    { "s14", IntegerRegister, 4 },
    { "s15", IntegerRegister, 4 },
    { "s16", IntegerRegister, 4 },
    { "s17", IntegerRegister, 4 },
    { "s18", IntegerRegister, 4 },
    { "s19", IntegerRegister, 4 },
    { "s20", IntegerRegister, 4 },
    { "s21", IntegerRegister, 4 },
    { "s22", IntegerRegister, 4 },
    { "s23", IntegerRegister, 4 },
    { "s24", IntegerRegister, 4 },
    { "s25", IntegerRegister, 4 },
    { "s26", IntegerRegister, 4 },
    { "s27", IntegerRegister, 4 },
    { "s28", IntegerRegister, 4 },
    { "s29", IntegerRegister, 4 },
    { "s30", IntegerRegister, 4 },
    { "s31", IntegerRegister, 4 },
    { "q0", IntegerRegister, 16 },
    { "q1", IntegerRegister, 16 },
    { "q2", IntegerRegister, 16 },
    { "q3", IntegerRegister, 16 },
    { "q4", IntegerRegister, 16 },
    { "q5", IntegerRegister, 16 },
    { "q6", IntegerRegister, 16 },
    { "q7", IntegerRegister, 16 },
    { "q8", IntegerRegister, 16 },
    { "q9", IntegerRegister, 16 },
    { "q10", IntegerRegister, 16 },
    { "q11", IntegerRegister, 16 },
    { "q12", IntegerRegister, 16 },
    { "q13", IntegerRegister, 16 },
    { "q14", IntegerRegister, 16 },
    { "q15", IntegerRegister, 16 },

    // Intel
    { "eax", IntegerRegister, 4 },
    { "ecx", IntegerRegister, 4 },
    { "edx", IntegerRegister, 4 },
    { "ebx", IntegerRegister, 4 },
    { "esp", IntegerRegister, 4 },
    { "ebp", IntegerRegister, 4 },
    { "esi", IntegerRegister, 4 },
    { "edi", IntegerRegister, 4 },
    { "eip", IntegerRegister, 4 },
    { "rax", IntegerRegister, 8 },
    { "rcx", IntegerRegister, 8 },
    { "rdx", IntegerRegister, 8 },
    { "rbx", IntegerRegister, 8 },
    { "rsp", IntegerRegister, 8 },
    { "rbp", IntegerRegister, 8 },
    { "rsi", IntegerRegister, 8 },
    { "rdi", IntegerRegister, 8 },
    { "rip", IntegerRegister, 8 },
    { "eflags", FlagRegister, 4 },
    { "cs", IntegerRegister, 2 },
    { "ss", IntegerRegister, 2 },
    { "ds", IntegerRegister, 2 },
    { "es", IntegerRegister, 2 },
    { "fs", IntegerRegister, 2 },
    { "gs", IntegerRegister, 2 },
    { "st0", FloatRegister, 10 },
    { "st1", FloatRegister, 10 },
    { "st2", FloatRegister, 10 },
    { "st3", FloatRegister, 10 },
    { "st4", FloatRegister, 10 },
    { "st5", FloatRegister, 10 },
    { "st6", FloatRegister, 10 },
    { "st7", FloatRegister, 10 },
    { "fctrl", FlagRegister, 4 },
    { "fstat", FlagRegister, 4 },
    { "ftag", FlagRegister, 4 },
    { "fiseg", FlagRegister, 4 },
    { "fioff", FlagRegister, 4 },
    { "foseg", FlagRegister, 4 },
    { "fooff", FlagRegister, 4 },
    { "fop", FlagRegister, 4 },
    { "mxcsr", FlagRegister, 4 },
    { "orig_eax", IntegerRegister, 4 },
    { "al", IntegerRegister, 1 },
    { "cl", IntegerRegister, 1 },
    { "dl", IntegerRegister, 1 },
    { "bl", IntegerRegister, 1 },
    { "ah", IntegerRegister, 1 },
    { "ch", IntegerRegister, 1 },
    { "dh", IntegerRegister, 1 },
    { "bh", IntegerRegister, 1 },
    { "ax", IntegerRegister, 2 },
    { "cx", IntegerRegister, 2 },
    { "dx", IntegerRegister, 2 },
    { "bx", IntegerRegister, 2 },
    { "bp", IntegerRegister, 2 },
    { "si", IntegerRegister, 2 },
    { "di", IntegerRegister, 2 }
 };

//////////////////////////////////////////////////////////////////
//
// RegisterValue
//
//////////////////////////////////////////////////////////////////

// FIXME: This should not really be needed. Instead the guessing, if any,
// should done by the engines.
static void fixup(Register *reg, RegisterKind kind, int size)
{
    reg->kind = kind;
    if (!reg->size)
        reg->size = size;
}

void Register::guessMissingData()
{
    if (name.startsWith("xmm")) {
        fixup(this, VectorRegister, 16);
        return;
    }

    for (int i = 0; i != sizeof(theNameAndType) / sizeof(theNameAndType[0]); ++i) {
        if (theNameAndType[i].name == name) {
            fixup(this, theNameAndType[i].kind, theNameAndType[i].size);
            return;
        }
    }

    if (reportedType == "int")
        fixup(this, IntegerRegister, 4);
    else if (reportedType == "float")
        fixup(this, IntegerRegister, 8);
    else if (reportedType == "_i387_ext")
        fixup(this, IntegerRegister, 10);
    else if (reportedType == "*1" || reportedType == "long")
        fixup(this, IntegerRegister, 0);
    else if (reportedType.contains("vec"))
        fixup(this, VectorRegister, 0);
    else if (reportedType.startsWith("int"))
        fixup(this, IntegerRegister, 0);
}

static QString subTypeName(RegisterKind kind, int size, RegisterFormat format)
{
    QString name(QLatin1Char('['));

    switch (kind) {
        case IntegerRegister: name += QLatin1Char('i'); break;
        case FloatRegister: name += QLatin1Char('f'); break;
        default: break;
    }

    name += QString::number(size);

    switch (format) {
        case BinaryFormat: name += QLatin1Char('b'); break;
        case OctalFormat: name += QLatin1Char('o'); break;
        case DecimalFormat: name += QLatin1Char('d'); break;
        case HexadecimalFormat: name += QLatin1Char('x'); break;
        case CharacterFormat: name += QLatin1Char('c'); break;
    }

    name += QLatin1Char(']');

    return name;
}

static uint decodeHexChar(unsigned char c)
{
    c -= '0';
    if (c < 10)
        return c;
    c -= 'A' - '0';
    if (c < 6)
        return 10 + c;
    c -= 'a' - 'A';
    if (c < 6)
        return 10 + c;
    return uint(-1);
}

void RegisterValue::operator=(const QByteArray &ba)
{
    known = !ba.isEmpty();
    uint shift = 0;
    int j = 0;
    v.u64[1] = v.u64[0] = 0;
    for (int i = ba.size(); --i >= 0 && j < 16; ++j) {
        quint64 d = decodeHexChar(ba.at(i));
        if (d == uint(-1))
            return;
        v.u64[0] |= (d << shift);
        shift += 4;
    }
    j = 0;
    shift = 0;
    for (int i = ba.size() - 16; --i >= 0 && j < 16; ++j) {
        quint64 d = decodeHexChar(ba.at(i));
        if (d == uint(-1))
            return;
        v.u64[1] |= (d << shift);
        shift += 4;
    }
}

bool RegisterValue::operator==(const RegisterValue &other)
{
    return v.u64[0] == other.v.u64[0] && v.u64[1] == other.v.u64[1];
}

static QByteArray formatRegister(quint64 v, int size, RegisterFormat format)
{
    QByteArray result;
    if (format == HexadecimalFormat) {
        result = QByteArray::number(v, 16);
        result.prepend(QByteArray(2*size - result.size(), '0'));
    } else if (format == DecimalFormat) {
        result = QByteArray::number(v, 10);
        result.prepend(QByteArray(2*size - result.size(), ' '));
    } else if (format == CharacterFormat) {
        if (v >= 32 && v < 127) {
            result += '\'';
            result += char(v);
            result += '\'';
        } else {
            result += "   ";
        }
        result.prepend(QByteArray(2*size - result.size(), ' '));
    }
    return result;
}

QByteArray RegisterValue::toByteArray(RegisterKind kind, int size, RegisterFormat format) const
{
    if (!known)
        return "[inaccessible]";
    if (kind == FloatRegister) {
        if (size == 4)
            return QByteArray::number(v.f[0]);
        if (size == 8)
            return QByteArray::number(v.d[0]);
    }

    QByteArray result;
    if (size > 8) {
        result += formatRegister(v.u64[1], size - 8, format);
        size = 8;
        if (format != HexadecimalFormat)
            result += ',';
    }
    return result + formatRegister(v.u64[0], size, format);
}

RegisterValue RegisterValue::subValue(int size, int index) const
{
    RegisterValue value;
    value.known = known;
    switch (size) {
        case 1:
            value.v.u8[0] = v.u8[index];
            break;
        case 2:
            value.v.u16[0] = v.u16[index];
            break;
        case 4:
            value.v.u32[0] = v.u32[index];
            break;
        case 8:
            value.v.u64[0] = v.u64[index];
            break;
    }
    return value;
}

//////////////////////////////////////////////////////////////////
//
// RegisterSubItem and RegisterItem
//
//////////////////////////////////////////////////////////////////

class RegisterSubItem : public Utils::TreeItem
{
public:
    RegisterSubItem(RegisterKind subKind, int subSize, int count, RegisterFormat format)
        : m_subKind(subKind), m_subFormat(format), m_subSize(subSize), m_count(count), m_changed(false)
    {}

    QVariant data(int column, int role) const;

    Qt::ItemFlags flags(int column) const
    {
        //return column == 1 ? Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable
        //                   : Qt::ItemIsSelectable|Qt::ItemIsEnabled;
        Q_UNUSED(column);
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    }

    RegisterKind m_subKind;
    RegisterFormat m_subFormat;
    int m_subSize;
    int m_count;
    bool m_changed;
};

class RegisterItem : public Utils::TreeItem
{
public:
    explicit RegisterItem(const Register &reg);

    QVariant data(int column, int role) const;
    Qt::ItemFlags flags(int column) const;

    quint64 addressValue() const;

    Register m_reg;
    RegisterFormat m_format;
    bool m_changed;
};

RegisterItem::RegisterItem(const Register &reg) :
    m_reg(reg), m_format(HexadecimalFormat), m_changed(true)
{
    if (m_reg.kind == UnknownRegister)
        m_reg.guessMissingData();

    if (m_reg.kind == IntegerRegister || m_reg.kind == VectorRegister) {
        for (int s = m_reg.size / 2; s; s = s / 2) {
            appendChild(new RegisterSubItem(IntegerRegister, s, m_reg.size / s, HexadecimalFormat));
            appendChild(new RegisterSubItem(IntegerRegister, s, m_reg.size / s, DecimalFormat));
            if (s == 1)
                appendChild(new RegisterSubItem(IntegerRegister, s, m_reg.size / s, CharacterFormat));
        }
    }
    if (m_reg.kind == IntegerRegister || m_reg.kind == VectorRegister) {
        for (int s = m_reg.size; s >= 4; s = s / 2)
            appendChild(new RegisterSubItem(FloatRegister, s, m_reg.size / s, DecimalFormat));
    }
}

Qt::ItemFlags RegisterItem::flags(int column) const
{
    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    // Can edit registers if they are hex numbers and not arrays.
    if (column == 1) //  && IntegerWatchLineEdit::isUnsignedHexNumber(QLatin1String(m_reg.display)))
        return notEditable | Qt::ItemIsEditable;
    return notEditable;
}

quint64 RegisterItem::addressValue() const
{
    return m_reg.value.v.u64[0];
}

QVariant RegisterItem::data(int column, int role) const
{
    switch (role) {
        case RegisterNameRole:
            return m_reg.name;

        case RegisterIsBigRole:
            return m_reg.value.v.u64[1] > 0;

        case RegisterChangedRole:
            return m_changed;

        case RegisterAsAddressRole:
            return addressValue();

        case RegisterFormatRole:
            return m_format;

        case Qt::DisplayRole:
            switch (column) {
                case RegisterNameColumn: {
                    QByteArray res = m_reg.name;
                    if (!m_reg.description.isEmpty())
                        res += " (" + m_reg.description + ')';
                    return res;
                }
                case RegisterValueColumn: {
                    return m_reg.value.toByteArray(m_reg.kind, m_reg.size, m_format);
                }
            }

        case Qt::ToolTipRole:
            return QString::fromLatin1("Current Value: %1\nPreviousValue: %2")
                    .arg(QString::fromLatin1(m_reg.value.toByteArray(m_reg.kind, m_reg.size, m_format)))
                    .arg(QString::fromLatin1(m_reg.previousValue.toByteArray(m_reg.kind, m_reg.size, m_format)));

        case Qt::EditRole: // Edit: Unpadded for editing
            return m_reg.value.toByteArray(m_reg.kind, m_reg.size, m_format);

        case Qt::TextAlignmentRole:
            return column == RegisterValueColumn ? QVariant(Qt::AlignRight) : QVariant();

        default:
            break;
    }
    return QVariant();
}

QVariant RegisterSubItem::data(int column, int role) const
{
    switch (role) {
        case RegisterChangedRole:
            return m_changed;

        case RegisterFormatRole: {
            RegisterItem *registerItem = static_cast<RegisterItem *>(parent());
            return int(registerItem->m_format);
        }

        case RegisterAsAddressRole:
            return 0;

        case Qt::DisplayRole:
            switch (column) {
                case RegisterNameColumn:
                    return subTypeName(m_subKind, m_subSize, m_subFormat);
                case RegisterValueColumn: {
                    QTC_ASSERT(parent(), return QVariant());
                    RegisterItem *registerItem = static_cast<RegisterItem *>(parent());
                    RegisterValue value = registerItem->m_reg.value;
                    QByteArray ba;
                    for (int i = 0; i != m_count; ++i) {
                        int tab = 5 * (i + 1) * m_subSize;
                        QByteArray b = value.subValue(m_subSize, i).toByteArray(m_subKind, m_subSize, m_subFormat);
                        ba += QByteArray(tab - ba.size() - b.size(), ' ');
                        ba += b;
                    }
                    return ba;
                }
            }

        case Qt::ToolTipRole:
            if (m_subKind == IntegerRegister) {
                if (m_subFormat == CharacterFormat)
                    return RegisterHandler::tr("Content as ASCII Characters");
                else
                    return RegisterHandler::tr("Content as %1-bit Integer Values").arg(8 * m_subSize);
            }
            if (m_subKind == FloatRegister)
                return RegisterHandler::tr("Contents as %1-bit Floating Point Values").arg(8 * m_subSize);

        default:
            break;
    }

    return QVariant();
}

//////////////////////////////////////////////////////////////////
//
// RegisterHandler
//
//////////////////////////////////////////////////////////////////

RegisterHandler::RegisterHandler()
{
    setObjectName(QLatin1String("RegisterModel"));
    setHeader(QStringList() << tr("Name") << tr("Value"));
}

void RegisterHandler::updateRegister(const Register &r)
{
    RegisterItem *reg = m_registerByName.value(r.name, 0);
    if (!reg) {
        reg = new RegisterItem(r);
        m_registerByName[r.name] = reg;
        rootItem()->appendChild(reg);
        return;
    }

    if (r.size > 0)
        reg->m_reg.size = r.size;
    if (!r.description.isEmpty())
        reg->m_reg.description = r.description;
    if (reg->m_reg.value != r.value) {
        // Indicate red if values change, keep changed.
        reg->m_changed = true;
        reg->m_reg.previousValue = reg->m_reg.value;
        reg->m_reg.value = r.value;
        emit registerChanged(reg->m_reg.name, reg->addressValue()); // Notify attached memory views.
    } else {
        reg->m_changed = false;
    }
}

void RegisterHandler::setNumberFormat(const QByteArray &name, RegisterFormat format)
{
    RegisterItem *reg = m_registerByName.value(name, 0);
    QTC_ASSERT(reg, return);
    reg->m_format = format;
    QModelIndex index = indexFromItem(reg);
    emit dataChanged(index, index);
}

RegisterMap RegisterHandler::registerMap() const
{
    RegisterMap result;
    Utils::TreeItem *root = rootItem();
    for (int i = 0, n = root->rowCount(); i != n; ++i) {
        RegisterItem *reg = static_cast<RegisterItem *>(root->child(i));
        quint64 value = reg->addressValue();
        if (value)
            result.insert(value, reg->m_reg.name);
    }
    return result;
}

} // namespace Internal
} // namespace Debugger
