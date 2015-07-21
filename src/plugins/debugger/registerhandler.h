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

#ifndef DEBUGGER_REGISTERHANDLER_H
#define DEBUGGER_REGISTERHANDLER_H

#include <utils/treemodel.h>

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

namespace Debugger {
namespace Internal {

enum RegisterColumns
{
    RegisterNameColumn,
    RegisterValueColumn,
    RegisterColumnCount
};

enum RegisterDataRole
{
    RegisterNameRole = Qt::UserRole,
    RegisterIsBigRole,
    RegisterChangedRole,
    RegisterFormatRole,
    RegisterAsAddressRole
};

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
    void operator=(const QByteArray &ba);
    bool operator==(const RegisterValue &other);
    bool operator!=(const RegisterValue &other) { return !operator==(other); }
    QByteArray toByteArray(RegisterKind kind, int size, RegisterFormat format) const;
    RegisterValue subValue(int size, int index) const;

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
    Register() { size = 0; kind = UnknownRegister; }
    void guessMissingData();

    QByteArray name;
    QByteArray reportedType;
    RegisterValue value;
    RegisterValue previousValue;
    QByteArray description;
    int size;
    RegisterKind kind;
};

class RegisterItem;
typedef QMap<quint64, QByteArray> RegisterMap;

class RegisterHandler : public Utils::TreeModel
{
    Q_OBJECT

public:
    RegisterHandler();

    QAbstractItemModel *model() { return this; }

    void updateRegister(const Register &reg);

    void setNumberFormat(const QByteArray &name, RegisterFormat format);
    void commitUpdates() { emit layoutChanged(); }
    RegisterMap registerMap() const;

signals:
    void registerChanged(const QByteArray &name, quint64 value); // For memory views

private:
    QHash<QByteArray, RegisterItem *> m_registerByName;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_REGISTERHANDLER_H
