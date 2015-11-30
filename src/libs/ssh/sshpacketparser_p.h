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

#ifndef SSHPACKETPARSER_P_H
#define SSHPACKETPARSER_P_H

#include <botan/botan.h>

#include <QByteArray>
#include <QList>
#include <QString>

namespace QSsh {
namespace Internal {

struct SshNameList
{
    SshNameList() : originalLength(0) {}
    SshNameList(quint32 originalLength) : originalLength(originalLength) {}
    quint32 originalLength;
    QList<QByteArray> names;
};

class SshPacketParseException { };

// This class's functions try to read a byte array at a certain offset
// as the respective chunk of data as specified in the SSH RFCs.
// If they succeed, they update the offset, so they can easily
// be called in succession by client code.
// For convenience, some have also versions that don't update the offset,
// so they can be called with rvalues if the new value is not needed.
// If they fail, they throw an SshPacketParseException.
class SshPacketParser
{
public:
    static bool asBool(const QByteArray &data, quint32 offset);
    static bool asBool(const QByteArray &data, quint32 *offset);
    static quint16 asUint16(const QByteArray &data, quint32 offset);
    static quint16 asUint16(const QByteArray &data, quint32 *offset);
    static quint64 asUint64(const QByteArray &data, quint32 offset);
    static quint64 asUint64(const QByteArray &data, quint32 *offset);
    static quint32 asUint32(const QByteArray &data, quint32 offset);
    static quint32 asUint32(const QByteArray &data, quint32 *offset);
    static QByteArray asString(const QByteArray &data, quint32 *offset);
    static QString asUserString(const QByteArray &data, quint32 *offset);
    static SshNameList asNameList(const QByteArray &data, quint32 *offset);
    static Botan::BigInt asBigInt(const QByteArray &data, quint32 *offset);

    static QString asUserString(const QByteArray &rawString);
};

} // namespace Internal
} // namespace QSsh

#endif // SSHPACKETPARSER_P_H
