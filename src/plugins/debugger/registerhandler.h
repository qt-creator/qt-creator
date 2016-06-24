/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <utils/treemodel.h>

#include <QHash>

namespace Utils { class ItemViewEvent; }

namespace Debugger {
namespace Internal {

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
    int size = 0;
    RegisterKind kind = UnknownRegister;
};

class RegisterSubItem;
class RegisterItem;
using RegisterRootItem = Utils::TypedTreeItem<RegisterItem>;
using RegisterModel = Utils::TreeModel<RegisterRootItem, RegisterItem, RegisterSubItem>;

typedef QMap<quint64, QString> RegisterMap;

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

    QHash<QString, RegisterItem *> m_registerByName;
    DebuggerEngine * const m_engine;
};

} // namespace Internal
} // namespace Debugger
