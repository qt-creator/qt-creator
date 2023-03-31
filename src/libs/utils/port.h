// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QMetaType>
#include <QList>
#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Port
{
public:
    Port() = default;
    explicit Port(quint16 port) : m_port(port) {}
    explicit Port(int port);
    explicit Port(uint port);

    quint16 number() const;
    bool isValid() const { return m_port != -1; }

    QString toString() const { return QString::number(m_port); }

    static QList<Port> parseFromSedOutput(const QByteArray &output);
    static QList<Port> parseFromCatOutput(const QByteArray &output);
    static QList<Port> parseFromNetstatOutput(const QByteArray &output);

    friend bool operator<(const Port &p1, const Port &p2) { return p1.number() < p2.number(); }
    friend bool operator<=(const Port &p1, const Port &p2) { return p1.number() <= p2.number(); }
    friend bool operator>(const Port &p1, const Port &p2) { return p1.number() > p2.number(); }
    friend bool operator>=(const Port &p1, const Port &p2) { return p1.number() >= p2.number(); }

    friend bool operator==(const Port &p1, const Port &p2)
    {
        return p1.isValid() ? (p2.isValid() && p1.number() == p2.number()) : !p2.isValid();
    }

    friend bool operator!=(const Port &p1, const Port &p2)
    {
        return p1.isValid() ? (!p2.isValid() || p1.number() != p2.number()) : p2.isValid();
    }

private:
    int m_port = -1;
};

} // Utils

Q_DECLARE_METATYPE(Utils::Port)
