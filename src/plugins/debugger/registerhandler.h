// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/treemodel.h>

#include <QHash>
#include <QSet>

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

class RegisterValue
{
public:
    RegisterValue() { known = false; v.u64[1] = v.u64[0] = 0; }
    bool operator==(const RegisterValue &other);
    bool operator!=(const RegisterValue &other) { return !operator==(other); }

    void fromString(const QString &str, RegisterFormat format);
    QString toString(RegisterKind kind, int size, RegisterFormat format,
                     bool forEdit = false) const;

    RegisterValue subValue(int size, int index) const;
    void setSubValue(int size, int index, RegisterValue subValue);

    void shiftOneDigit(uint digit, RegisterFormat format);

    union {
        quint8  u8[16];
        quint16 u16[8];
        quint32 u32[4];
        quint64 u64[2];
        float     f[4];
        double    d[2];
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
    QSet<QString> groups;
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

} // Debugger::Internal
