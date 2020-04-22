/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvscfunctions.h"

#include <debugger/debuggerprotocol.h>

namespace Debugger {
namespace Internal {
namespace UvscUtils {

enum RegisterType {
    // General purpose registers (R0-R15).
    MinimumGeneralPurposeRegister = 0,
    MaximumGeneralPurposeRegister = 15,

    // Banked registers.
    MinimumBankedRegister = 17,
    MaximumBankedRegister = 18,

    // System registers.
    MinimumSystemRegister = 32,
    MaximumSystemRegister = 35,

    // Program status register.
    ProgramStatusRegister = 256,
};

SSTR encodeSstr(const QString &value);
QString decodeSstr(const SSTR &sstr);
QString decodeAscii(const qint8 *ascii);
QByteArray encodeProjectData(const QStringList &someNames);
QByteArray encodeBreakPoint(BKTYPE type, const QString &exp, const QString &cmd = QString());
QByteArray encodeAmem(quint64 address, quint32 bytesCount);
QByteArray encodeAmem(quint64 address, const QByteArray &data);
EXECCMD encodeCommand(const QString &cmd);
TVAL encodeVoidTval();
TVAL encodeIntTval(int value);
TVAL encodeU64Tval(quint64 value);
VSET encodeVoidVset(const QString &value);
VSET encodeIntVset(int index, const QString &value);
VSET encodeU64Vset(quint64 index, const QString &value);
bool isKnownRegister(int type);
QString adjustHexValue(QString hex, const QString &type);

QByteArray encodeU32(quint32 value);
quint32 decodeU32(const QByteArray &data);

QString buildLocalId(const VARINFO &varinfo);
QString buildLocalEditable(const VARINFO &varinfo);
QString buildLocalNumchild(const VARINFO &varinfo);
QString buildLocalName(const VARINFO &varinfo);
QString buildLocalIName(const QString &parentIName, const QString &name = QString());
QString buildLocalWName(const QString &exp);
QString buildLocalType(const VARINFO &varinfo);
QString buildLocalValue(const VARINFO &varinfo, const QString &type);

GdbMi buildEntry(const QString &name, const QString &data, GdbMi::Type type);
GdbMi buildChildrenEntry(const std::vector<GdbMi> &locals);
GdbMi buildResultTemplateEntry(bool partial);

} // namespace UvscUtils
} // namespace Internal
} // namespace Debugger
