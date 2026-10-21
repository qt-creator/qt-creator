// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerengine.h"

#include <utils/treemodel.h>

#include <QHash>
#include <memory>
#include <vector>
#include <utility>

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

// PeripheralFieldEnumerant

class PeripheralFieldEnumerant final
{
public:
    QString name;
    QString description;
    quint64 value = 0;
};

// PeripheralFieldEnum

class PeripheralFieldEnum final
{
public:
    QString name;

    QVector<PeripheralFieldEnumerant> enumerants;

    bool isValid() const { return !enumerants.empty(); }

    std::pair<QString, QString> enumerant(quint64 fieldValue) const;
};

// PeripheralRegisterField

enum class PeripheralFieldFormat
{
    Normal,
    ToolTip
};

class PeripheralRegisterField final
{
public:
    QString bitRangeString() const;
    QString bitValueString(quint64 regValue, PeripheralFieldFormat fieldFormat = PeripheralFieldFormat::Normal) const;
    quint64 bitValue(quint64 regValue) const;
    quint64 bitMask() const;

    QString name;
    QString description;
    int bitOffset = 0;
    int bitWidth = 0;
    PeripheralFieldEnum fieldEnum;
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    PeripheralRegisterFormat format = PeripheralRegisterFormat::Hexadecimal;
};

// PeripheralRegisterValue

class PeripheralRegisterValue final
{
public:
    PeripheralRegisterValue(quint64 v = 0) : v(v) {}
    bool operator==(const PeripheralRegisterValue &other) const { return v == other.v; }

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
    int dimensions = 0;
    int dimensionIncrement = 0;
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    PeripheralRegisterFormat format = PeripheralRegisterFormat::Hexadecimal;

    QList<PeripheralRegisterField> fields;

    PeripheralRegisterValue currentValue;
    PeripheralRegisterValue previousValue;
    PeripheralRegisterValue resetValue;

    int instances() const { return dimensions == 0 ? 1 : dimensions; }
};

// PeripheralRegisterGroup

class PeripheralRegisterGroup final
{
public:
    QString name;
    QString displayName;
    QString description;
    quint64 addressOffset = 0;
    int dimensions = 0;
    int dimensionIncrement = 0;
    int size = 0; // in bits
    PeripheralRegisterAccess access = PeripheralRegisterAccess::Unknown;
    bool active = false;
    PeripheralRegisterGroup *parentGroup = nullptr;
    QVector<PeripheralRegister> registers;
    std::vector<std::unique_ptr<PeripheralRegisterGroup>> clusters;
    quint64 baseAddress() const;
    void setBaseAddress(quint64 address);

    PeripheralRegisterGroup() = default;
    PeripheralRegisterGroup(const PeripheralRegisterGroup &other);
    PeripheralRegisterGroup &operator=(const PeripheralRegisterGroup &) = delete;

private:
    quint64 m_baseAddress = 0;
};

// PeripheralRegisterGroups

using PeripheralRegisterGroups = std::vector<std::unique_ptr<PeripheralRegisterGroup>>;

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
    int registerSize(quint64 address) const;

private:
    QVariant data(const QModelIndex &idx, int role) const final;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) final;

    bool contextMenuEvent(const Utils::ItemViewEvent &ev);
    QMenu *createRegisterGroupsMenu(DebuggerState state);
    QMenu *createRegisterFormatMenu(DebuggerState state,
                                    PeripheralRegisterItem *item) const;
    QMenu *createRegisterFieldFormatMenu(DebuggerState state,
                                         PeripheralRegisterFieldItem *item) const;
    void setActiveGroup(const QString &groupName, const QStringList &clusterPath = {});
    void deactivateGroups();

    PeripheralRegisterGroups m_peripheralRegisterGroups;
    QHash<quint64, PeripheralRegisterItem *> m_activeRegisters;
    DebuggerEngine * const m_engine;
};

#ifdef WITH_TESTS
QObject *createPeripheralRegisterTest();
#endif

} // Debugger::Internal
