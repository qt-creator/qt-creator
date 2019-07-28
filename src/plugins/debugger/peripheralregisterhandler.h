/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debuggerengine.h"

#include <utils/treemodel.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Utils { class ItemViewEvent; }

namespace Debugger {
namespace Internal {

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
    bool operator==(const PeripheralRegisterValue &other) { return v == other.v; }
    bool operator!=(const PeripheralRegisterValue &other) { return !operator==(other); }

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
    Q_OBJECT
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
    QMenu *createRegisterGroupsMenu(DebuggerState state) const;
    QMenu *createRegisterFormatMenu(DebuggerState state,
                                    PeripheralRegisterItem *item) const;
    QMenu *createRegisterFieldFormatMenu(DebuggerState state,
                                         PeripheralRegisterFieldItem *item) const;
    void setActiveGroup(bool checked);
    void deactivateGroups();

    PeripheralRegisterGroups m_peripheralRegisterGroups;
    QHash<quint64, PeripheralRegisterItem *> m_activeRegisters;
    DebuggerEngine * const m_engine;
};

} // namespace Internal
} // namespace Debugger
