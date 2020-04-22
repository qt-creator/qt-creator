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

#include "uvscutils.h"

#include <QDataStream>
#include <QVariant>

namespace Debugger {
namespace Internal {
namespace UvscUtils {

// Utils

SSTR encodeSstr(const QString &value)
{
    SSTR sstr = {};
    // Note: UVSC API support only ASCII!
    const QByteArray data = value.toLocal8Bit();
    if (sizeof(sstr.data) < size_t(data.size()))
        return sstr;
    sstr.length = data.size();
    ::memcpy(sstr.data, data.constData(), sstr.length);
    return sstr;
}

QString decodeSstr(const SSTR &sstr)
{
    // Note: UVSC API support only ASCII!
    return QString::fromLocal8Bit(reinterpret_cast<const char *>(sstr.data),
                                  sstr.length);
}

QString decodeAscii(const qint8 *ascii)
{
    // Note: UVSC API support only ASCII!
    return QString::fromLocal8Bit(reinterpret_cast<const char *>(ascii));
}

QByteArray encodeProjectData(const QStringList &someNames)
{
    QByteArray buffer(sizeof(PRJDATA) - 1, 0);

    // Note: UVSC API support only ASCII!
    quint32 namesLength = 0;
    for (const QString &someName : someNames) {
        const QByteArray asciiName = someName.toLocal8Bit();
        buffer.append(asciiName);
        buffer.append('\0');
        namesLength += asciiName.size() + 1;
    }
    buffer.append('\0');
    namesLength += 1;

    const auto dataPtr = reinterpret_cast<PRJDATA *>(buffer.data());
    dataPtr->code = 0;
    dataPtr->length = namesLength;
    return buffer;
}

QByteArray encodeBreakPoint(BKTYPE type, const QString &exp, const QString &cmd)
{
    QByteArray buffer(sizeof(BKPARM) - 1, 0);
    // Note: UVSC API support only ASCII!
    const QByteArray asciiExp = exp.toLocal8Bit();
    buffer.append(asciiExp);
    buffer.append('\0');
    const QByteArray asciiCmd = cmd.toLocal8Bit();
    buffer.append(asciiCmd);
    buffer.append('\0');

    const auto bkPtr = reinterpret_cast<BKPARM *>(buffer.data());
    bkPtr->type = type;
    bkPtr->count = 1;
    bkPtr->accessSize = 0;
    bkPtr->expressionLength = asciiExp.count() + 1;
    bkPtr->commandLength = asciiCmd.count() + 1;
    return buffer;
}

QByteArray encodeAmem(quint64 address, quint32 bytesCount)
{
    QByteArray buffer(sizeof(AMEM) - 1, 0);
    buffer.resize(buffer.size() + bytesCount);
    const auto amem = reinterpret_cast<AMEM *>(buffer.data());
    amem->address = address;
    amem->bytesCount = bytesCount;
    return buffer;
}

QByteArray encodeAmem(quint64 address, const QByteArray &data)
{
    QByteArray buffer(sizeof(AMEM) - 1, 0);
    buffer.append(data);
    const auto amem = reinterpret_cast<AMEM *>(buffer.data());
    amem->address = address;
    amem->bytesCount = data.size();
    return buffer;
}

EXECCMD encodeCommand(const QString &cmd)
{
    EXECCMD exeCmd = {};
    exeCmd.useEcho = false;
    exeCmd.command = encodeSstr(cmd);
    return exeCmd;
}

TVAL encodeVoidTval()
{
    TVAL tval = {};
    tval.type = VTT_void;
    return tval;
}

TVAL encodeIntTval(int value)
{
    TVAL tval = {};
    tval.type = VTT_int;
    tval.v.i = value;
    return tval;
}

TVAL encodeU64Tval(quint64 value)
{
    TVAL tval = {};
    tval.type = VTT_uint64;
    tval.v.u64 = value;
    return tval;
}

VSET encodeVoidVset(const QString &value)
{
    VSET vset = {};
    vset.value = encodeVoidTval();
    vset.name = encodeSstr(value);
    return vset;
}

VSET encodeIntVset(int index, const QString &value)
{
    VSET vset = {};
    vset.value = encodeIntTval(index);
    vset.name = encodeSstr(value);
    return vset;
}

VSET encodeU64Vset(quint64 index, const QString &value)
{
    VSET vset = {};
    vset.value = encodeU64Tval(index);
    vset.name = encodeSstr(value);
    return vset;
}

bool isKnownRegister(int type)
{
    if (type >= MinimumGeneralPurposeRegister && type <= MaximumGeneralPurposeRegister)
        return true;
    else if (type >= MinimumBankedRegister && type <= MaximumBankedRegister)
        return true;
    else if (type >= MinimumSystemRegister && type <= MaximumSystemRegister)
        return true;
    else if (type == ProgramStatusRegister)
        return true;
    return false;
}

QString adjustHexValue(QString hex, const QString &type)
{
    if (!hex.startsWith("0x"))
        return hex;

    hex.remove(0, 2);
    const QByteArray data = QByteArray::fromHex(hex.toLatin1());
    QDataStream in(data);

    if (type == "float") {
        float v = 0;
        in >> v;
        return QString::number(v);
    } else if (type == "double") {
        double v = 0;
        in >> v;
        return QString::number(v);
    } else {
        const bool isUnsigned = type.startsWith("unsigned");
        switch (data.count()) {
        case 1:
            if (isUnsigned) {
                quint8 v = 0;
                in >> v;
                return QString::number(v);
            } else {
                qint8 v = 0;
                in >> v;
                return QString::number(v);
            }
        case 2:
            if (isUnsigned) {
                quint16 v = 0;
                in >> v;
                return QString::number(v);
            } else {
                qint16 v = 0;
                in >> v;
                return QString::number(v);
            }
        case 4:
            if (isUnsigned) {
                quint32 v = 0;
                in >> v;
                return QString::number(v);
            } else {
                qint32 v = 0;
                in >> v;
                return QString::number(v);
            }
        case 8:
            if (isUnsigned) {
                quint64 v = 0;
                in >> v;
                return QString::number(v);
            } else {
                qint64 v = 0;
                in >> v;
                return QString::number(v);
            }
        default:
            break;
        }
    }

    return {};
}

QByteArray encodeU32(quint32 value)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out << value;
    return data;
}

quint32 decodeU32(const QByteArray &data)
{
    QDataStream in(data);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 value = 0;
    in >> value;
    return value;
}

QString buildLocalId(const VARINFO &varinfo)
{
    return QString::number(varinfo.id);
}

QString buildLocalEditable(const VARINFO &varinfo)
{
    return QVariant(bool(varinfo.isEditable)).toString();
}

QString buildLocalNumchild(const VARINFO &varinfo)
{
    return QString::number(varinfo.count);
}

QString buildLocalName(const VARINFO &varinfo)
{
    return decodeSstr(varinfo.name);
}

QString buildLocalIName(const QString &parentIName, const QString &name)
{
    if (name.isEmpty())
        return parentIName;
    return parentIName + '.' + name;
}

QString buildLocalWName(const QString &exp)
{
    return QString::fromLatin1(exp.toLatin1().toHex());
}

QString buildLocalType(const VARINFO &varinfo)
{
    QString type = decodeSstr(varinfo.type);
    // Remove the 'auto - ' and 'param - ' prefixes.
    if (type.startsWith("auto - "))
        type.remove(0, 7);
    else if (type.startsWith("param - "))
        type.remove(0, 8);
    return type;
}

QString buildLocalValue(const VARINFO &varinfo, const QString &type)
{
    QString value = decodeSstr(varinfo.value);
    // Adjust value to the desired form.
    if (value.startsWith("0x")) {
        const int spaceIndex = value.indexOf(" ");
        const QString hex = value.mid(0, spaceIndex);
        if (type == "char") {
            value = adjustHexValue(hex, type);
        } else if (type.startsWith("enum") && spaceIndex != -1) {
            const QString name = value.mid(spaceIndex + 1);
            value = QStringLiteral("%1 (%2)").arg(name).arg(hex.toInt(nullptr, 16));
        } else if (type.startsWith("struct")) {
            value = QStringLiteral("@%1").arg(hex);
        } else {
            value = adjustHexValue(hex, type);
        }
    }
    return value;
}

GdbMi buildEntry(const QString &name, const QString &data, GdbMi::Type type)
{
    GdbMi result;
    result.m_name = name;
    result.m_data = data;
    result.m_type = type;
    return result;
}

GdbMi buildChildrenEntry(const std::vector<GdbMi> &locals)
{
    GdbMi children = buildEntry("children", "", GdbMi::List);
    for (const GdbMi &local : locals)
        children.addChild(local);
    return children;
}

GdbMi buildResultTemplateEntry(bool partial)
{
    GdbMi all = UvscUtils::buildEntry("result", "", GdbMi::Tuple);

    // FIXME: Do we need in 'token', 'typeinfo', 'counts' and 'timings'
    // entries?

    // Build token entry.
    all.addChild(UvscUtils::buildEntry("token", "0", GdbMi::Const));
    // Build typeinfo entry.
    all.addChild(UvscUtils::buildEntry("typeinfo", "", GdbMi::List));
    // Build counts entry.
    all.addChild(UvscUtils::buildEntry("counts", "", GdbMi::Tuple));
    // Build timings entry.
    all.addChild(UvscUtils::buildEntry("timings", "", GdbMi::List));
    // Build partial entry.
    all.addChild(UvscUtils::buildEntry("partial", QString::number(partial),
                                       GdbMi::Const));
    return all;
}

} // namespace UvscUtils
} // namespace Internal
} // namespace Debugger
