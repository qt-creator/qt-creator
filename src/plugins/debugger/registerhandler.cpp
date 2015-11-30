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

#include "debuggerengine.h"
#include "watchdelegatewidgets.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// Register
//
//////////////////////////////////////////////////////////////////

void Register::guessMissingData()
{
    if (reportedType == "int")
        kind = IntegerRegister;
    else if (reportedType == "float")
        kind = FloatRegister;
    else if (reportedType == "_i387_ext")
        kind = FloatRegister;
    else if (reportedType == "*1" || reportedType == "long")
        kind = IntegerRegister;
    else if (reportedType.contains("vec"))
        kind = VectorRegister;
    else if (reportedType.startsWith("int"))
        kind = IntegerRegister;
    else if (name.startsWith("xmm") || name.startsWith("ymm"))
        kind = VectorRegister;
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
        case DecimalFormat: name += QLatin1Char('u'); break;
        case SignedDecimalFormat: name += QLatin1Char('s'); break;
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

void RegisterValue::fromByteArray(const QByteArray &ba, RegisterFormat format)
{
    known = !ba.isEmpty();
    v.u64[1] = v.u64[0] = 0;

    const int n = ba.size();
    int pos = 0;
    if (ba.startsWith("0x"))
        pos += 2;

    bool negative = pos < n && ba.at(pos) == '-';
    if (negative)
        ++pos;

    while (pos < n) {
        uint c = ba.at(pos);
        if (format != CharacterFormat) {
            c = decodeHexChar(c);
            if (c == uint(-1))
                break;
        }
        shiftOneDigit(c, format);
        ++pos;
    }

    if (negative) {
        v.u64[1] = ~v.u64[1];
        v.u64[0] = ~v.u64[0];
        ++v.u64[0];
        if (v.u64[0] == 0)
            ++v.u64[1];
    }
}

bool RegisterValue::operator==(const RegisterValue &other)
{
    return v.u64[0] == other.v.u64[0] && v.u64[1] == other.v.u64[1];
}

static QByteArray formatRegister(quint64 v, int size, RegisterFormat format, bool forEdit)
{
    QByteArray result;
    if (format == HexadecimalFormat) {
        result = QByteArray::number(v, 16);
        result.prepend(QByteArray(2*size - result.size(), '0'));
    } else if (format == DecimalFormat) {
        result = QByteArray::number(v, 10);
        result.prepend(QByteArray(2*size - result.size(), ' '));
    } else if (format == SignedDecimalFormat) {
        qint64 sv;
        if (size >= 8)
            sv = qint64(v);
        else if (size >= 4)
            sv = qint32(v);
        else if (size >= 2)
            sv = qint16(v);
        else
            sv = qint8(v);
        result = QByteArray::number(sv, 10);
        result.prepend(QByteArray(2*size - result.size(), ' '));
    } else if (format == CharacterFormat) {
        bool spacesOnly = true;
        if (v >= 32 && v < 127) {
            spacesOnly = false;
            if (!forEdit)
                result += '\'';
            result += char(v);
            if (!forEdit)
                result += '\'';
        } else {
            result += "   ";
        }
        if (spacesOnly && forEdit)
            result.clear();
        else
            result.prepend(QByteArray(2*size - result.size(), ' '));
    }
    return result;
}

QByteArray RegisterValue::toByteArray(RegisterKind kind, int size, RegisterFormat format, bool forEdit) const
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
        result += formatRegister(v.u64[1], size - 8, format, forEdit);
        size = 8;
        if (format != HexadecimalFormat)
            result += ',';
    }
    return result + formatRegister(v.u64[0], size, format, forEdit);
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

void RegisterValue::setSubValue(int size, int index, RegisterValue subValue)
{
    switch (size) {
        case 1:
            v.u8[index] = subValue.v.u8[0];
            break;
        case 2:
            v.u16[index] = subValue.v.u16[0];
            break;
        case 4:
            v.u32[index] = subValue.v.u32[0];
            break;
        case 8:
            v.u64[index] = subValue.v.u64[0];
            break;
    }
}

static inline void shiftBitsLeft(RegisterValue *val, int amount)
{
    val->v.u64[1] <<= amount;
    val->v.u64[1] |= val->v.u64[0] >> (64 - amount);
    val->v.u64[0] <<= amount;
}

void RegisterValue::shiftOneDigit(uint digit, RegisterFormat format)
{
    switch (format) {
    case HexadecimalFormat:
        shiftBitsLeft(this, 4);
        v.u64[0] |= digit;
        break;
    case OctalFormat:
        shiftBitsLeft(this, 3);
        v.u64[0] |= digit;
        break;
    case BinaryFormat:
        shiftBitsLeft(this, 1);
        v.u64[0] |= digit;
        break;
    case DecimalFormat:
    case SignedDecimalFormat: {
        shiftBitsLeft(this, 1);
        quint64 tmp0 = v.u64[0];
        quint64 tmp1 = v.u64[1];
        shiftBitsLeft(this, 2);
        v.u64[1] += tmp1;
        v.u64[0] += tmp0;
        if (v.u64[0] < tmp0)
            ++v.u64[1];
        v.u64[0] += digit;
        if (v.u64[0] < digit)
            ++v.u64[1];
        break;
    }
    case CharacterFormat:
        shiftBitsLeft(this, 8);
        v.u64[0] |= digit;
    }
}

//////////////////////////////////////////////////////////////////
//
// RegisterSubItem and RegisterItem
//
//////////////////////////////////////////////////////////////////

class RegisterSubItem;

class RegisterEditItem : public Utils::TreeItem
{
public:
    RegisterEditItem(int pos, RegisterKind subKind, int subSize, RegisterFormat format)
        : m_index(pos), m_subKind(subKind), m_subSize(subSize), m_subFormat(format)
    {}

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &value, int role) override;
    Qt::ItemFlags flags(int column) const override;

    int m_index;
    RegisterKind m_subKind;
    int m_subSize;
    RegisterFormat m_subFormat;
};


class RegisterSubItem : public Utils::TreeItem
{
public:
    RegisterSubItem(RegisterKind subKind, int subSize, int count, RegisterFormat format)
        : m_subKind(subKind), m_subFormat(format), m_subSize(subSize), m_count(count), m_changed(false)
    {
        for (int i = 0; i != count; ++i)
            appendChild(new RegisterEditItem(i, subKind, subSize, format));
    }

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

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &value, int role) override;
    Qt::ItemFlags flags(int column) const override;

    quint64 addressValue() const;
    void triggerChange();

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
        if (m_reg.size <= 8) {
            appendChild(new RegisterSubItem(IntegerRegister, m_reg.size, 1, SignedDecimalFormat));
            appendChild(new RegisterSubItem(IntegerRegister, m_reg.size, 1, DecimalFormat));
        }
        for (int s = m_reg.size / 2; s; s = s / 2) {
            appendChild(new RegisterSubItem(IntegerRegister, s, m_reg.size / s, HexadecimalFormat));
            appendChild(new RegisterSubItem(IntegerRegister, s, m_reg.size / s, SignedDecimalFormat));
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

void RegisterItem::triggerChange()
{
    QByteArray ba = "0x" + m_reg.value.toByteArray(m_reg.kind, m_reg.size, HexadecimalFormat);
    DebuggerEngine *engine = static_cast<RegisterHandler *>(model())->engine();
    engine->setRegisterValue(m_reg.name, QString::fromLatin1(ba));
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
            return QString::fromLatin1(m_reg.value.toByteArray(m_reg.kind, m_reg.size, m_format));

        case Qt::TextAlignmentRole:
            return column == RegisterValueColumn ? QVariant(Qt::AlignRight) : QVariant();

        default:
            break;
    }
    return QVariant();
}

bool RegisterItem::setData(int column, const QVariant &value, int role)
{
    if (column == RegisterValueColumn && role == Qt::EditRole) {
        m_reg.value.fromByteArray(value.toString().toLatin1(), m_format);
        triggerChange();
        return true;
    }
    return false;
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
                if (m_subFormat == SignedDecimalFormat)
                    return RegisterHandler::tr("Content as %1-bit Signed Decimal Values").arg(8 * m_subSize);
                if (m_subFormat == DecimalFormat)
                    return RegisterHandler::tr("Content as %1-bit Unsigned Decimal Values").arg(8 * m_subSize);
                if (m_subFormat == HexadecimalFormat)
                    return RegisterHandler::tr("Content as %1-bit Hexadecimal Values").arg(8 * m_subSize);
                if (m_subFormat == OctalFormat)
                    return RegisterHandler::tr("Content as %1-bit Octal Values").arg(8 * m_subSize);
                if (m_subFormat == BinaryFormat)
                    return RegisterHandler::tr("Content as %1-bit Binary Values").arg(8 * m_subSize);
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

RegisterHandler::RegisterHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    setObjectName(QLatin1String("RegisterModel"));
    setHeader({tr("Name"), tr("Value")});
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
    QModelIndex index = indexForItem(reg);
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

QVariant RegisterEditItem::data(int column, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (column) {
                case RegisterNameColumn: {
                    return QString::fromLatin1("[%1]").arg(m_index);
                }
                case RegisterValueColumn: {
                    RegisterItem *registerItem = static_cast<RegisterItem *>(parent()->parent());
                    RegisterValue value = registerItem->m_reg.value;
                    return value.subValue(m_subSize, m_index)
                            .toByteArray(m_subKind, m_subSize, m_subFormat, role == Qt::EditRole);
                }
            }
        case Qt::ToolTipRole: {
                RegisterItem *registerItem = static_cast<RegisterItem *>(parent()->parent());
                return RegisterHandler::tr("Edit bits %1...%2 of register %3")
                        .arg(m_index * 8).arg(m_index * 8 + 7)
                        .arg(QString::fromLatin1(registerItem->m_reg.name));
            }
        default:
            break;
    }
    return QVariant();
}

bool RegisterEditItem::setData(int column, const QVariant &value, int role)
{
    if (column == RegisterValueColumn && role == Qt::EditRole) {
        QTC_ASSERT(parent(), return false);
        QTC_ASSERT(parent()->parent(), return false);
        RegisterItem *registerItem = static_cast<RegisterItem *>(parent()->parent());
        Register &reg = registerItem->m_reg;
        RegisterValue vv;
        vv.fromByteArray(value.toString().toLatin1(), m_subFormat);
        reg.value.setSubValue(m_subSize, m_index, vv);
        registerItem->triggerChange();
        return true;
    }
    return false;
}

Qt::ItemFlags RegisterEditItem::flags(int column) const
{
    QTC_ASSERT(parent(), return Qt::ItemFlags());
    RegisterSubItem *registerSubItem = static_cast<RegisterSubItem *>(parent());
    Qt::ItemFlags f = registerSubItem->flags(column);
    if (column == RegisterValueColumn)
        f |= Qt::ItemIsEditable;
    return f;
}

} // namespace Internal
} // namespace Debugger
