/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchutils.h"
#include "watchdata.h"
#include "debuggerprotocol.h"

#include <QDebug>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };

namespace Debugger {
namespace Internal {

QString removeObviousSideEffects(const QString &expIn)
{
    QString exp = expIn.trimmed();
    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')) || !hasLetterOrNumber(exp) || isKeyWord(exp))
        return QString();

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"')))
        return QString();

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.truncate(exp.size() - 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return QString();

    if (hasSideEffects(exp) || exp.isEmpty())
        return QString();
    return exp;
}

bool isSkippableFunction(const QString &funcName, const QString &fileName)
{
    if (fileName.endsWith(QLatin1String("/qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/moc_qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/qmetaobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/qmetaobject_p.h")))
        return true;
    if (fileName.endsWith(QLatin1String(".moc")))
        return true;

    if (funcName.endsWith(QLatin1String("::qt_metacall")))
        return true;
    if (funcName.endsWith(QLatin1String("::d_func")))
        return true;
    if (funcName.endsWith(QLatin1String("::q_func")))
        return true;

    return false;
}

bool isLeavableFunction(const QString &funcName, const QString &fileName)
{
    if (funcName.endsWith(QLatin1String("QObjectPrivate::setCurrentSender")))
        return true;
    if (funcName.endsWith(QLatin1String("QMutexPool::get")))
        return true;

    if (fileName.endsWith(QLatin1String(".cpp"))) {
        if (fileName.endsWith(QLatin1String("/qmetaobject.cpp"))
                && funcName.endsWith(QLatin1String("QMetaObject::methodOffset")))
            return true;
        if (fileName.endsWith(QLatin1String("/qobject.cpp"))
                && (funcName.endsWith(QLatin1String("QObjectConnectionListVector::at"))
                    || funcName.endsWith(QLatin1String("~QObject"))))
            return true;
        if (fileName.endsWith(QLatin1String("/qmutex.cpp")))
            return true;
        if (fileName.endsWith(QLatin1String("/qthread.cpp")))
            return true;
        if (fileName.endsWith(QLatin1String("/qthread_unix.cpp")))
            return true;
    } else if (fileName.endsWith(QLatin1String(".h"))) {

        if (fileName.endsWith(QLatin1String("/qobject.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qmutex.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qvector.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qlist.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qhash.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qmap.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qshareddata.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qstring.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qglobal.h")))
            return true;

    } else {

        if (fileName.contains(QLatin1String("/qbasicatomic")))
            return true;
        if (fileName.contains(QLatin1String("/qorderedmutexlocker_p")))
            return true;
        if (fileName.contains(QLatin1String("/qatomic")))
            return true;
    }

    return false;
}

bool isLetterOrNumber(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

bool hasLetterOrNumber(const QString &exp)
{
    const QChar underscore = QLatin1Char('_');
    for (int i = exp.size(); --i >= 0; )
        if (exp.at(i).isLetterOrNumber() || exp.at(i) == underscore)
            return true;
    return false;
}

bool hasSideEffects(const QString &exp)
{
    // FIXME: complete?
    return exp.contains(QLatin1String("-="))
        || exp.contains(QLatin1String("+="))
        || exp.contains(QLatin1String("/="))
        || exp.contains(QLatin1String("%="))
        || exp.contains(QLatin1String("*="))
        || exp.contains(QLatin1String("&="))
        || exp.contains(QLatin1String("|="))
        || exp.contains(QLatin1String("^="))
        || exp.contains(QLatin1String("--"))
        || exp.contains(QLatin1String("++"));
}

bool isKeyWord(const QString &exp)
{
    // FIXME: incomplete.
    if (!exp.isEmpty())
        return false;
    switch (exp.at(0).toLatin1()) {
    case 'a':
        return exp == QLatin1String("auto");
    case 'b':
        return exp == QLatin1String("break");
    case 'c':
        return exp == QLatin1String("case") || exp == QLatin1String("class")
               || exp == QLatin1String("const") || exp == QLatin1String("constexpr")
               || exp == QLatin1String("catch") || exp == QLatin1String("continue")
               || exp == QLatin1String("const_cast");
    case 'd':
        return exp == QLatin1String("do") || exp == QLatin1String("default")
               || exp == QLatin1String("delete") || exp == QLatin1String("decltype")
               || exp == QLatin1String("dynamic_cast");
    case 'e':
        return exp == QLatin1String("else") || exp == QLatin1String("extern")
               || exp == QLatin1String("enum") || exp == QLatin1String("explicit");
    case 'f':
        return exp == QLatin1String("for") || exp == QLatin1String("friend");  // 'final'?
    case 'g':
        return exp == QLatin1String("goto");
    case 'i':
        return exp == QLatin1String("if") || exp == QLatin1String("inline");
    case 'n':
        return exp == QLatin1String("new") || exp == QLatin1String("namespace")
               || exp == QLatin1String("noexcept");
    case 'm':
        return exp == QLatin1String("mutable");
    case 'o':
        return exp == QLatin1String("operator"); // 'override'?
    case 'p':
        return exp == QLatin1String("public") || exp == QLatin1String("protected")
               || exp == QLatin1String("private");
    case 'r':
        return exp == QLatin1String("return") || exp == QLatin1String("register")
               || exp == QLatin1String("reinterpret_cast");
    case 's':
        return exp == QLatin1String("struct") || exp == QLatin1String("switch")
               || exp == QLatin1String("static_cast");
    case 't':
        return exp == QLatin1String("template") || exp == QLatin1String("typename")
               || exp == QLatin1String("try") || exp == QLatin1String("throw")
               || exp == QLatin1String("typedef");
    case 'u':
        return exp == QLatin1String("union") || exp == QLatin1String("using");
    case 'v':
        return exp == QLatin1String("void") || exp == QLatin1String("volatile")
               || exp == QLatin1String("virtual");
    case 'w':
        return exp == QLatin1String("while");
    }
    return false;
}

bool startsWithDigit(const QString &str)
{
    return !str.isEmpty() && str.at(0).isDigit();
}

QByteArray stripPointerType(QByteArray type)
{
    if (type.endsWith('*'))
        type.chop(1);
    if (type.endsWith("* const"))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

// Format a hex address with colons as in the memory editor.
QString formatToolTipAddress(quint64 a)
{
    QString rc = QString::number(a, 16);
    if (a) {
        if (const int remainder = rc.size() % 4)
            rc.prepend(QString(4 - remainder, QLatin1Char('0')));
        const QChar colon = QLatin1Char(':');
        switch (rc.size()) {
        case 16:
            rc.insert(12, colon);
        case 12:
            rc.insert(8, colon);
        case 8:
            rc.insert(4, colon);
        }
    }
    return QLatin1String("0x") + rc;
}

QByteArray gdbQuoteTypes(const QByteArray &type)
{
    // gdb does not understand sizeof(Core::IDocument*).
    // "sizeof('Core::IDocument*')" is also not acceptable,
    // it needs to be "sizeof('Core::IDocument'*)"
    //
    // We never will have a perfect solution here (even if we had a full blown
    // C++ parser as we do not have information on what is a type and what is
    // a variable name. So "a<b>::c" could either be two comparisons of values
    // 'a', 'b' and '::c', or a nested type 'c' in a template 'a<b>'. We
    // assume here it is the latter.
    //return type;

    // (*('myns::QPointer<myns::QObject>*'*)0x684060)" is not acceptable
    // (*('myns::QPointer<myns::QObject>'**)0x684060)" is acceptable
    if (isPointerType(type))
        return gdbQuoteTypes(stripPointerType(type)) + '*';

    QByteArray accu;
    QByteArray result;
    int templateLevel = 0;

    const char colon = ':';
    const char singleQuote = '\'';
    const char lessThan = '<';
    const char greaterThan = '>';
    for (int i = 0; i != type.size(); ++i) {
        const char c = type.at(i);
        if (isLetterOrNumber(c) || c == '_' || c == colon || c == ' ') {
            accu += c;
        } else if (c == lessThan) {
            ++templateLevel;
            accu += c;
        } else if (c == greaterThan) {
            --templateLevel;
            accu += c;
        } else if (templateLevel > 0) {
            accu += c;
        } else {
            if (accu.contains(colon) || accu.contains(lessThan))
                result += singleQuote + accu + singleQuote;
            else
                result += accu;
            accu.clear();
            result += c;
        }
    }
    if (accu.contains(colon) || accu.contains(lessThan))
        result += singleQuote + accu + singleQuote;
    else
        result += accu;
    //qDebug() << "GDB_QUOTING" << type << " TO " << result;

    return result;
}

// Utilities to decode string data returned by the dumper helpers.


template <class T>
void decodeArrayHelper(QList<WatchData> *list, const WatchData &tmplate,
    const QByteArray &rawData)
{
    const QByteArray ba = QByteArray::fromHex(rawData);
    const T *p = (const T *) ba.data();
    WatchData data;
    const QByteArray exp = "*(" + gdbQuoteTypes(tmplate.type) + "*)0x";
    for (int i = 0, n = ba.size() / sizeof(T); i < n; ++i) {
        data = tmplate;
        data.sortId = i;
        data.iname += QByteArray::number(i);
        data.name = QString::fromLatin1("[%1]").arg(i);
        data.value = QString::number(p[i]);
        data.address += i * sizeof(T);
        data.exp = exp + QByteArray::number(data.address, 16);
        data.setAllUnneeded();
        list->append(data);
    }
}

void decodeArray(QList<WatchData> *list, const WatchData &tmplate,
    const QByteArray &rawData, int encoding)
{
    switch (encoding) {
        case Hex2EncodedInt1:
            decodeArrayHelper<signed char>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt2:
            decodeArrayHelper<short>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt4:
            decodeArrayHelper<int>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt8:
            decodeArrayHelper<qint64>(list, tmplate, rawData);
            break;
        case Hex2EncodedUInt1:
            decodeArrayHelper<uchar>(list, tmplate, rawData);
            break;
        case Hex2EncodedUInt2:
            decodeArrayHelper<ushort>(list, tmplate, rawData);
            break;
        case Hex2EncodedUInt4:
            decodeArrayHelper<uint>(list, tmplate, rawData);
            break;
        case Hex2EncodedUInt8:
            decodeArrayHelper<quint64>(list, tmplate, rawData);
            break;
        case Hex2EncodedFloat4:
            decodeArrayHelper<float>(list, tmplate, rawData);
            break;
        case Hex2EncodedFloat8:
            decodeArrayHelper<double>(list, tmplate, rawData);
            break;
        default:
            qDebug() << "ENCODING ERROR: " << encoding;
    }
}

//////////////////////////////////////////////////////////////////////
//
// GdbMi interaction
//
//////////////////////////////////////////////////////////////////////

void setWatchDataValue(WatchData &data, const GdbMi &item)
{
    GdbMi value = item.findChild("value");
    if (value.isValid()) {
        int encoding = item.findChild("valueencoded").data().toInt();
        data.setValue(decodeData(value.data(), encoding));
    } else {
        data.setValueNeeded();
    }
}

void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
    int encoding)
{
    if (mi.isValid())
        data.setValueToolTip(decodeData(mi.data(), encoding));
}

void setWatchDataChildCount(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.setHasChildren(mi.data().toInt() > 0);
}

void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEnabled = true;
    else if (mi.data() == "false")
        data.valueEnabled = false;
}

static void setWatchDataValueEditable(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEditable = true;
    else if (mi.data() == "false")
        data.valueEditable = false;
}

static void setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = mi.data();
}

static void setWatchDataAddress(WatchData &data, quint64 address, quint64 origAddress = 0)
{
    if (origAddress) { // Gdb dumpers reports the dereferenced address as origAddress
        data.address = origAddress;
        data.referencingAddress = address;
    } else {
        data.address = address;
    }
    if (data.exp.isEmpty() && !data.dumperFlags.startsWith('$')) {
        if (data.iname.startsWith("local.") && data.iname.count('.') == 1)
            // Solve one common case of adding 'class' in
            // *(class X*)0xdeadbeef for gdb.
            data.exp = data.name.toLatin1();
        else
            data.exp = "*(" + gdbQuoteTypes(data.type) + "*)" + data.hexAddress();
    }
}

void setWatchDataAddress(WatchData &data, const GdbMi &addressMi, const GdbMi &origAddressMi)
{
    if (!addressMi.isValid())
        return;
    const QByteArray addressBA = addressMi.data();
    if (!addressBA.startsWith("0x")) { // Item model dumpers pull tricks.
        data.dumperFlags = addressBA;
        return;
    }
    const quint64 address = addressMi.toAddress();
    const quint64 origAddress = origAddressMi.toAddress();
    setWatchDataAddress(data, address, origAddress);
}

static void setWatchDataSize(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        bool ok = false;
        const unsigned size = mi.data().toUInt(&ok);
        if (ok)
            data.size = size;
    }
}

// Find the "type" and "displayedtype" children of root and set up type.
void setWatchDataType(WatchData &data, const GdbMi &item)
{
    if (item.isValid())
        data.setType(item.data());
    else if (data.type.isEmpty())
        data.setTypeNeeded();
}

void setWatchDataDisplayedType(WatchData &data, const GdbMi &item)
{
    if (item.isValid())
        data.displayedType = QString::fromLatin1(item.data());
}

void parseWatchData(const QSet<QByteArray> &expandedINames,
    const WatchData &data0, const GdbMi &item,
    QList<WatchData> *list)
{
    //qDebug() << "HANDLE CHILDREN: " << data0.toString() << item.toString();
    WatchData data = data0;
    bool isExpanded = expandedINames.contains(data.iname);
    if (!isExpanded)
        data.setChildrenUnneeded();

    GdbMi children = item.findChild("children");
    if (children.isValid() || !isExpanded)
        data.setChildrenUnneeded();

    setWatchDataType(data, item.findChild("type"));
    GdbMi mi = item.findChild("editvalue");
    if (mi.isValid())
        data.editvalue = mi.data();
    mi = item.findChild("editformat");
    if (mi.isValid())
        data.editformat = mi.data().toInt();
    mi = item.findChild("typeformats");
    if (mi.isValid())
        data.typeFormats = QString::fromUtf8(mi.data());
    mi = item.findChild("bitpos");
    if (mi.isValid())
        data.bitpos = mi.data().toInt();
    mi = item.findChild("bitsize");
    if (mi.isValid())
        data.bitsize = mi.data().toInt();

    setWatchDataValue(data, item);
    setWatchDataAddress(data, item.findChild("addr"), item.findChild("origaddr"));
    setWatchDataSize(data, item.findChild("size"));
    setWatchDataExpression(data, item.findChild("exp"));
    setWatchDataValueEnabled(data, item.findChild("valueenabled"));
    setWatchDataValueEditable(data, item.findChild("valueeditable"));
    setWatchDataChildCount(data, item.findChild("numchild"));
    //qDebug() << "\nAPPEND TO LIST: " << data.toString() << "\n";
    list->append(data);

    bool ok = false;
    qulonglong addressBase = item.findChild("addrbase").data().toULongLong(&ok, 0);
    qulonglong addressStep = item.findChild("addrstep").data().toULongLong(&ok, 0);

    // Try not to repeat data too often.
    WatchData childtemplate;
    setWatchDataType(childtemplate, item.findChild("childtype"));
    setWatchDataChildCount(childtemplate, item.findChild("childnumchild"));
    //qDebug() << "CHILD TEMPLATE:" << childtemplate.toString();

    mi = item.findChild("arraydata");
    if (mi.isValid()) {
        int encoding = item.findChild("arrayencoding").data().toInt();
        childtemplate.iname = data.iname + '.';
        childtemplate.address = addressBase;
        decodeArray(list, childtemplate, mi.data(), encoding);
    } else {
        for (int i = 0, n = children.children().size(); i != n; ++i) {
            const GdbMi &child = children.children().at(i);
            WatchData data1 = childtemplate;
            data1.sortId = i;
            GdbMi name = child.findChild("name");
            if (name.isValid())
                data1.name = QString::fromLatin1(name.data());
            else
                data1.name = QString::number(i);
            GdbMi iname = child.findChild("iname");
            if (iname.isValid()) {
                data1.iname = iname.data();
            } else {
                data1.iname = data.iname;
                data1.iname += '.';
                data1.iname += data1.name.toLatin1();
            }
            if (!data1.name.isEmpty() && data1.name.at(0).isDigit())
                data1.name = QLatin1Char('[') + data1.name + QLatin1Char(']');
            if (addressStep) {
                setWatchDataAddress(data1, addressBase);
                addressBase += addressStep;
            }
            QByteArray key = child.findChild("key").data();
            if (!key.isEmpty()) {
                int encoding = child.findChild("keyencoded").data().toInt();
                QString skey = decodeData(key, encoding);
                if (skey.size() > 13) {
                    skey = skey.left(12);
                    skey += QLatin1String("...");
                }
                //data1.name += " (" + skey + ")";
                data1.name = skey;
            }
            parseWatchData(expandedINames, data1, child, list);
        }
    }
}

} // namespace Internal
} // namespace Debugger
