// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QList>
#include <QMetaType>
#include <QString>

namespace QmlDesigner {

class TokenCommand
{
    friend QDataStream &operator>>(QDataStream &in, TokenCommand &command);
    friend bool operator ==(const TokenCommand &first, const TokenCommand &second);

public:
    TokenCommand();
    explicit TokenCommand(const QString &tokenName, qint32 tokenNumber, const QList<qint32> &instances);

    QString tokenName() const;
    qint32 tokenNumber() const;
    QList<qint32> instances() const;

    void sort();

private:
    QString m_tokenName;
    qint32 m_tokenNumber;
    QList<qint32> m_instanceIdVector;
};

QDataStream &operator<<(QDataStream &out, const TokenCommand &command);
QDataStream &operator>>(QDataStream &in, TokenCommand &command);

bool operator ==(const TokenCommand &first, const TokenCommand &second);
QDebug operator <<(QDebug debug, const TokenCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::TokenCommand)
