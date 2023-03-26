// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerengine.h"

#include <utils/treemodel.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Utils { class ItemViewEvent; }

namespace Debugger::Internal {

class DebuggerEngine;

enum class PeripheralRegisterAccess
{
    Unknown,
    ReadOnly,
    WriteOnly,
    ReadWrite
};

enum class PeripheralRegisterFormat
{
    Hexadecimal,
    Decimal,
    Octal,
    Binary
};

// PeripheralRegisterField

class PeripheralRegisterField final
{
public:
    QString bitRangeString() const;
    QString bitValueString(quint64 regValue) const;
    quint64 bitValue(quint64 regValue) const;
    quint64 bitMask() const;

    QString name;
    QString description;
    int bitOffset = 0;
    int bitWidth = 0;
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    PeripheralRegisterFormat format = PeripheralRegisterFormat::Hexadecimal;
};

// PeripheralRegisterValue

class PeripheralRegisterValue final
{
public:
    PeripheralRegisterValue(quint64 v = 0) : v(v) {}
    bool operator==(const PeripheralRegisterValue &other) const { return v == other.v; }
    bool operator!=(const PeripheralRegisterValue &other) const { return !operator==(other); }

    bool fromString(const QString &string, PeripheralRegisterFormat fmt);
    QString toString(int size, PeripheralRegisterFormat fmt) const;

    quint64 v = 0;
};

// PeripheralRegister

class PeripheralRegister final
{
public:
    QString currentValueString() const;
    QString previousValueString() const;
    QString resetValueString() const;

    QString addressString(quint64 baseAddress) const;
    quint64 address(quint64 baseAddress) const;

    QString name;
    QString displayName;
    QString description;
    quint64 addressOffset = 0;
    int size = 0; // in bits
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    PeripheralRegisterFormat format = PeripheralRegisterFormat::Hexadecimal;

    QVector<PeripheralRegisterField> fields;

    PeripheralRegisterValue currentValue;
    PeripheralRegisterValue previousValue;
    PeripheralRegisterValue resetValue;
};

// PeripheralRegisterGroup

class PeripheralRegisterGroup final
{
public:
    QString name;
    QString displayName;
    QString description;
    quint64 baseAddress = 0;
    int size = 0; // in bits
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    bool active = false;
    QVector<PeripheralRegister> registers;
};

// PeripheralRegisterGroups

using PeripheralRegisterGroups = QVector<PeripheralRegisterGroup>;

// PeripheralRegisterItem's

enum { PeripheralRegisterLevel = 1, PeripheralRegisterFieldLevel = 2 };

class PeripheralRegisterFieldItem;
class PeripheralRegisterItem;
using PeripheralRegisterRootItem = Utils::TypedTreeItem<PeripheralRegisterItem>;
using PeripheralRegisterModel = Utils::TreeModel<PeripheralRegisterRootItem,
                                                 PeripheralRegisterItem,
                                                 PeripheralRegisterFieldItem>;

// PeripheralRegisterHandler

class PeripheralRegisterHandler final : public PeripheralRegisterModel
{
public:
    explicit PeripheralRegisterHandler(DebuggerEngine *engine);

    QAbstractItemModel *model() { return this; }

    void updateRegisterGroups();
    void updateRegister(quint64 address, quint64 value);
    void commitUpdates() { emit layoutChanged(); }
    QList<quint64> activeRegisters() const;

private:
    QVariant data(const QModelIndex &idx, int role) const final;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) final;

    bool contextMenuEvent(const Utils::ItemViewEvent &ev);
    QMenu *createRegisterGroupsMenu(DebuggerState state);
    QMenu *createRegisterFormatMenu(DebuggerState state,
                                    PeripheralRegisterItem *item) const;
    QMenu *createRegisterFieldFormatMenu(DebuggerState state,
                                         PeripheralRegisterFieldItem *item) const;
    void setActiveGroup(const QString &groupName);
    void deactivateGroups();

    PeripheralRegisterGroups m_peripheralRegisterGroups;
    QHash<quint64, PeripheralRegisterItem *> m_activeRegisters;
    DebuggerEngine * const m_engine;
};

} // Debugger::Internal
