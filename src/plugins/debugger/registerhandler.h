// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/treemodel.h>

#include <QHash>
#include <QSet>

QT_BEGIN_NAMESPACE
class QAbstractItemDelegate;
QT_END_NAMESPACE

namespace Utils { class ItemViewEvent; }

namespace Debugger::Internal {

class DebuggerEngine;

enum RegisterKind
{
    UnknownRegister,
    IntegerRegister,
    FloatRegister,
    VectorRegister,
    FlagRegister,
    OtherRegister
};

enum RegisterFormat
{
    CharacterFormat,
    HexadecimalFormat,
    DecimalFormat,
    SignedDecimalFormat,
    OctalFormat,
    BinaryFormat
};

// A minimal unsigned 128-bit integer synthesized from two quint64. It provides
// only the operations needed to format and edit register values, so that
// 256-bit registers (held as two of these) can be shown without depending on
// the compiler's non-portable __int128.
class Quint128
{
public:
    Quint128() = default;
    Quint128(quint64 value) : lo(value), hi(0) {}

    explicit operator bool() const { return lo || hi; }

    friend bool operator==(Quint128 a, Quint128 b) { return a.lo == b.lo && a.hi == b.hi; }
    friend bool operator<(Quint128 a, Quint128 b)
    {
        return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
    }

    Quint128 operator~() const
    {
        Quint128 result;
        result.lo = ~lo;
        result.hi = ~hi;
        return result;
    }

    Quint128 &operator++()
    {
        if (++lo == 0)
            ++hi;
        return *this;
    }

    Quint128 &operator+=(Quint128 other)
    {
        lo += other.lo;
        hi += other.hi;
        if (lo < other.lo)
            ++hi;
        return *this;
    }

    Quint128 &operator|=(Quint128 other)
    {
        lo |= other.lo;
        hi |= other.hi;
        return *this;
    }

    Quint128 &operator<<=(int n)
    {
        hi = (hi << n) | (lo >> (64 - n));
        lo <<= n;
        return *this;
    }

    Quint128 operator>>(int n) const
    {
        Quint128 result;
        if (n >= 64) {
            result.lo = hi >> (n - 64);
            result.hi = 0;
        } else {
            result.lo = (lo >> n) | (hi << (64 - n));
            result.hi = hi >> n;
        }
        return result;
    }

    uint operator&(uint mask) const { return uint(lo) & mask; }

    Quint128 operator/(uint divisor) const { uint rem; return divided(divisor, rem); }
    uint operator%(uint divisor) const { uint rem; divided(divisor, rem); return rem; }

    quint64 lo;
    quint64 hi;

private:
    // Long division by a small divisor (used with 10 and 16), processing the
    // value in 32-bit limbs so that (remainder << 32 | limb) cannot overflow.
    Quint128 divided(uint divisor, uint &remainder) const
    {
        const quint32 limbs[4] = {quint32(hi >> 32), quint32(hi),
                                  quint32(lo >> 32), quint32(lo)};
        quint64 rem = 0;
        quint32 quotient[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; ++i) {
            const quint64 cur = (rem << 32) | limbs[i];
            quotient[i] = quint32(cur / divisor);
            rem = cur % divisor;
        }
        remainder = uint(rem);
        Quint128 result;
        result.hi = (quint64(quotient[0]) << 32) | quotient[1];
        result.lo = (quint64(quotient[2]) << 32) | quotient[3];
        return result;
    }
};

class RegisterValue
{
public:
    RegisterValue() { known = false; v.u128[1] = v.u128[0] = 0; }
    bool operator==(const RegisterValue &other);

    void fromString(const QString &str, RegisterFormat format);
    QString toString(RegisterKind kind, int size, RegisterFormat format,
                     bool forEdit = false) const;

    RegisterValue subValue(int size, int index) const;
    void setSubValue(int size, int index, RegisterValue subValue);

    void shiftOneDigit(uint digit, RegisterFormat format);

    union {
        quint8    u8[32];
        quint16  u16[16];
        quint32   u32[8];
        quint64   u64[4];
        Quint128 u128[2];
        float       f[8];
        double      d[4];
    } v;
    bool known;
};

class Register
{
public:
    Register() {}
    void guessMissingData();

    QString name;
    QString reportedType;
    RegisterValue value;
    RegisterValue previousValue;
    QString description;
    QStringList groups;
    int size = 0;
    RegisterKind kind = UnknownRegister;
};

class RegisterSubItem;
class RegisterItem;
class RegisterGroup;
using RegisterRootItem = Utils::TypedTreeItem<RegisterGroup>;
using RegisterModel = Utils::TreeModel<RegisterRootItem, RegisterGroup, RegisterItem, RegisterSubItem>;

using RegisterMap = QMap<quint64, QString>;

class RegisterHandler : public RegisterModel
{
    Q_OBJECT

public:
    explicit RegisterHandler(DebuggerEngine *engine);

    QAbstractItemModel *model() { return this; }

    void updateRegister(const Register &reg);
    void commitUpdates() { emit layoutChanged(); }
    RegisterMap registerMap() const;

signals:
    void registerChanged(const QString &name, quint64 value); // For memory views

private:
    QVariant data(const QModelIndex &idx, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;

    bool contextMenuEvent(const Utils::ItemViewEvent &ev);
    RegisterGroup *allRegisters() const;
    QHash<QString, RegisterGroup *> m_registerGroups;
    DebuggerEngine * const m_engine;
};

QAbstractItemDelegate *createRegisterDelegate(int column);

} // Debugger::Internal
