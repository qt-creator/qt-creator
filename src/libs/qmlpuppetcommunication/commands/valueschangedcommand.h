// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ValuesChangedCommand
{
    friend QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command);
    friend QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);
    friend bool operator ==(const ValuesChangedCommand &first, const ValuesChangedCommand &second);

public:
    enum TransactionOption { Start = 1, End = 2, None = 0 };
    ValuesChangedCommand();
    explicit ValuesChangedCommand(const QVector<PropertyValueContainer> &valueChangeVector);

    const QVector<PropertyValueContainer> valueChanges() const;
    quint32 keyNumber() const;

    static void removeSharedMemorys(const QVector<qint32> &keyNumberVector);

    void sort();
    TransactionOption transactionOption = TransactionOption::None;

private:
    QVector<PropertyValueContainer> m_valueChangeVector;
    mutable quint32 m_keyNumber;
};

QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);

bool operator ==(const ValuesChangedCommand &first, const ValuesChangedCommand &second);
QDebug operator <<(QDebug debug, const ValuesChangedCommand &instance);

/* ValuesChangedCommand is used to notify that the values of a specific instatiated
 * QObject changed.
 * The ValuesModifiedCommand is used to notify that a user changed a QML property and
 * that this property should be changed in the data model.
 */

class ValuesModifiedCommand : public ValuesChangedCommand
{
public:
    ValuesModifiedCommand() = default;

    explicit ValuesModifiedCommand(const QVector<PropertyValueContainer> &valueChangeVector)
        : ValuesChangedCommand(valueChangeVector)
    {}
};

QDataStream &operator<<(QDataStream &out, const ValuesModifiedCommand &command);
QDataStream &operator>>(QDataStream &in, ValuesModifiedCommand &command);

} // namespace QmlDesigner


Q_DECLARE_METATYPE(QmlDesigner::ValuesModifiedCommand)
Q_DECLARE_METATYPE(QmlDesigner::ValuesChangedCommand)
