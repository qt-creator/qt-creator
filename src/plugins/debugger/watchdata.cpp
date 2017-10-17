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

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchdata.h"
#include "watchutils.h"
#include "debuggerprotocol.h"

#include <QDebug>

namespace Debugger {
namespace Internal {

bool isPointerType(const QString &type)
{
    return type.endsWith('*') || type.endsWith("* const");
}

bool isIntType(const QString &type)
{
    if (type.isEmpty())
        return false;
    switch (type.at(0).unicode()) {
        case 'b':
            return type == "bool";
        case 'c':
            return type == "char";
        case 'i':
            return type == "int";
        case 'l':
            return type == "long"
                || type == "long int"
                || type == "long unsigned int";
        case 'p':
            return type == "ptrdiff_t";
        case 'q':
            return type == "qint16" || type == "quint16"
                || type == "qint32" || type == "quint32"
                || type == "qint64" || type == "quint64"
                || type == "qlonglong" || type == "qulonglong";
        case 's':
            return type == "short"
                || type == "signed"
                || type == "size_t"
                || type == "std::size_t"
                || type == "std::ptrdiff_t"
                || (type.startsWith("signed") &&
                    (  type == "signed char"
                    || type == "signed short"
                    || type == "signed short int"
                    || type == "signed long"
                    || type == "signed long int"
                    || type == "signed long long"
                    || type == "signed long long int"));
        case 'u':
            return type == "unsigned"
                || (type.startsWith("unsigned") &&
                    (  type == "unsigned char"
                    || type == "unsigned short"
                    || type == "unsigned short int"
                    || type == "unsigned int"
                    || type == "unsigned long"
                    || type == "unsigned long int"
                    || type == "unsigned long long"
                    || type == "unsigned long long int"));
        default:
            return false;
    }
}

bool isFloatType(const QString &type)
{
    return type == "float" || type == "double" || type == "qreal" || type == "number";
}

bool isIntOrFloatType(const QString &type)
{
    return isIntType(type) || isFloatType(type);
}

WatchItem::WatchItem() :
    id(WatchItem::InvalidId),
    address(0),
    origaddr(0),
    size(0),
    bitpos(0),
    bitsize(0),
    elided(0),
    arrayIndex(-1),
    sortGroup(0),
    wantsChildren(false),
    valueEnabled(true),
    valueEditable(true),
    outdated(false)
{
}

bool WatchItem::isVTablePointer() const
{
    // First case: Cdb only. No user type can be named like this, this is safe.
    // Second case: Python dumper only.
    return type.startsWith("__fptr()") || (type.isEmpty() && name == "[vptr]");
}

void WatchItem::setError(const QString &msg)
{
    value = msg;
    wantsChildren = false;
    valueEnabled = false;
    valueEditable = false;
}

void WatchItem::setValue(const QString &value0)
{
    value = value0;
    if (value == QLatin1String("{...}")) {
        value.clear();
        wantsChildren = true; // at least one...
    }
}

QString WatchItem::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << QLatin1Char('{');
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
    if (!name.isEmpty() && name != iname)
        str << "name=\"" << name << doubleQuoteComma;
    if (address) {
        str.setIntegerBase(16);
        str << "addr=\"0x" << address << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (origaddr) {
        str.setIntegerBase(16);
        str << "referencingaddr=\"0x" << origaddr << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (!exp.isEmpty())
        str << "exp=\"" << exp << doubleQuoteComma;

    if (!value.isEmpty())
        str << "value=\"" << value << doubleQuoteComma;

    if (elided)
        str << "valueelided=\"" << elided << doubleQuoteComma;

    if (!editvalue.isEmpty())
        str << "editvalue=\"<...>\",";

    str << "type=\"" << type << doubleQuoteComma;

    str << "wantsChildren=\"" << (wantsChildren ? "true" : "false") << doubleQuoteComma;

    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

QString WatchItem::msgNotInScope()
{
    //: Value of variable in Debugger Locals display for variables out
    //: of scope (stopped above initialization).
    static const QString rc =
        QCoreApplication::translate("Debugger::Internal::WatchItem", "<not in scope>");
    return rc;
}

const QString &WatchItem::shadowedNameFormat()
{
    //: Display of variables shadowed by variables of the same name
    //: in nested scopes: Variable %1 is the variable name, %2 is a
    //: simple count.
    static const QString format =
        QCoreApplication::translate("Debugger::Internal::WatchItem", "%1 <shadowed %2>");
    return format;
}

QString WatchItem::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name).arg(seen);
}

QString WatchItem::hexAddress() const
{
    if (address)
        return "0x" + QString::number(address, 16);
    return QString();
}

template <class T>
QString decodeItemHelper(const T &t)
{
    return QString::number(t);
}

QString decodeItemHelper(const double &t)
{
    return QString::number(t, 'g', 16);
}

class ArrayDataDecoder
{
public:
    template <class T>
    void decodeArrayHelper(int childSize)
    {
        const QByteArray ba = QByteArray::fromHex(rawData.toUtf8());
        const T *p = (const T *) ba.data();
        for (int i = 0, n = ba.size() / sizeof(T); i < n; ++i) {
            WatchItem *child = new WatchItem;
            child->arrayIndex = i;
            child->value = decodeItemHelper(p[i]);
            child->size = childSize;
            child->type = childType;
            child->address = addrbase + i * addrstep;
            child->valueEditable = true;
            item->appendChild(child);
        }
    }

    void decode()
    {
        if (addrstep == 0)
            addrstep = encoding.size;
        switch (encoding.type) {
            case DebuggerEncoding::HexEncodedSignedInteger:
                switch (encoding.size) {
                    case 1:
                        return decodeArrayHelper<signed char>(encoding.size);
                    case 2:
                        return decodeArrayHelper<short>(encoding.size);
                    case 4:
                        return decodeArrayHelper<int>(encoding.size);
                    case 8:
                        return decodeArrayHelper<qint64>(encoding.size);
                }
            case DebuggerEncoding::HexEncodedUnsignedInteger:
                switch (encoding.size) {
                    case 1:
                        return decodeArrayHelper<uchar>(encoding.size);
                    case 2:
                        return decodeArrayHelper<ushort>(encoding.size);
                    case 4:
                        return decodeArrayHelper<uint>(encoding.size);
                    case 8:
                        return decodeArrayHelper<quint64>(encoding.size);
                }
                break;
            case DebuggerEncoding::HexEncodedFloat:
                switch (encoding.size) {
                    case 4:
                        return decodeArrayHelper<float>(encoding.size);
                    case 8:
                        return decodeArrayHelper<double>(encoding.size);
                }
            default:
                break;
        }
        qDebug() << "ENCODING ERROR: " << encoding.toString();
    }

    WatchItem *item;
    QString rawData;
    QString childType;
    DebuggerEncoding encoding;
    quint64 addrbase;
    quint64 addrstep;
};

static bool sortByName(const WatchItem *a, const WatchItem *b)
{
    if (a->sortGroup != b->sortGroup)
        return a->sortGroup > b->sortGroup;

    return a->name < b->name;
}

void WatchItem::parseHelper(const GdbMi &input, bool maySort)
{
    GdbMi mi = input["type"];
    if (mi.isValid())
        type = mi.data();

    editvalue = input["editvalue"].data();
    editformat = input["editformat"].data();
    editencoding = DebuggerEncoding(input["editencoding"].data());

    mi = input["valueelided"];
    if (mi.isValid())
        elided = mi.toInt();

    mi = input["bitpos"];
    if (mi.isValid())
        bitpos = mi.toInt();

    mi = input["bitsize"];
    if (mi.isValid())
        bitsize = mi.toInt();

    mi = input["origaddr"];
    if (mi.isValid())
        origaddr = mi.toAddress();

    mi = input["address"];
    if (mi.isValid()) {
        address = mi.toAddress();
        if (exp.isEmpty()) {
            if (iname.startsWith("local.") && iname.count('.') == 1)
                // Solve one common case of adding 'class' in
                // *(class X*)0xdeadbeef for gdb.
                exp = name;
            else
                exp = "*(" + type + "*)" + hexAddress();
        }
    }

    mi = input["value"];
    QString enc = input["valueencoded"].data();
    if (mi.isValid() || !enc.isEmpty()) {
        setValue(decodeData(mi.data(), enc));
        mi = input["valuesuffix"];
        if (mi.isValid())
            value += mi.data();
    }

    mi = input["size"];
    if (mi.isValid())
        size = mi.toInt();

    mi = input["exp"];
    if (mi.isValid())
        exp = mi.data();

    mi = input["sortgroup"];
    if (mi.isValid())
        sortGroup = mi.toInt();

    mi = input["valueenabled"];
    if (mi.data() == "true")
        valueEnabled = true;
    else if (mi.data() == "false")
        valueEnabled = false;

    mi = input["valueeditable"];
    if (mi.data() == "true")
        valueEditable = true;
    else if (mi.data() == "false")
        valueEditable = false;

    mi = input["numchild"]; // GDB/MI
    if (mi.isValid())
        setHasChildren(mi.toInt() > 0);
    mi = input["haschild"]; // native-mixed
    if (mi.isValid())
        setHasChildren(mi.toInt() > 0);

    mi = input["arraydata"];
    if (mi.isValid()) {
        ArrayDataDecoder decoder;
        decoder.item = this;
        decoder.rawData = mi.data();
        decoder.childType = input["childtype"].data();
        decoder.addrbase = input["addrbase"].toAddress();
        decoder.addrstep = input["addrstep"].toAddress();
        decoder.encoding = DebuggerEncoding(input["arrayencoding"].data());
        decoder.decode();
    } else {
        const GdbMi children = input["children"];
        if (children.isValid()) {
            bool ok = false;
            // Try not to repeat data too often.
            const GdbMi childType = input["childtype"];
            const GdbMi childNumChild = input["childnumchild"];

            qulonglong addressBase = input["addrbase"].data().toULongLong(&ok, 0);
            qulonglong addressStep = input["addrstep"].data().toULongLong(&ok, 0);

            for (int i = 0, n = int(children.children().size()); i != n; ++i) {
                const GdbMi &subinput = children.children().at(i);
                WatchItem *child = new WatchItem;
                if (childType.isValid())
                    child->type = childType.data();
                if (childNumChild.isValid())
                    child->setHasChildren(childNumChild.toInt() > 0);
                GdbMi name = subinput["name"];
                QString nn;
                if (name.isValid()) {
                    nn = name.data();
                    child->name = nn;
                } else {
                    nn.setNum(i);
                    child->name = QString("[%1]").arg(i);
                }
                GdbMi iname = subinput["iname"];
                if (iname.isValid())
                    child->iname = iname.data();
                else
                    child->iname = this->iname + '.' + nn;
                if (addressStep) {
                    child->address = addressBase + i * addressStep;
                    child->exp = "*(" + child->type + "*)0x"
                                      + QString::number(child->address, 16);
                }
                QString key = subinput["key"].data();
                if (!key.isEmpty())
                    child->name = decodeData(key, subinput["keyencoded"].data());
                child->name = subinput["keyprefix"].data() + child->name;
                child->parseHelper(subinput, maySort);
                appendChild(child);
            }

            if (maySort && input["sortable"].toInt())
                sortChildren(&sortByName);
        }
    }
}

void WatchItem::parse(const GdbMi &data, bool maySort)
{
    iname = data["iname"].data();

    GdbMi wname = data["wname"];
    if (wname.isValid()) // Happens (only) for watched expressions.
        name = fromHex(wname.data());
    else
        name = data["name"].data();

    parseHelper(data, maySort);

    if (wname.isValid())
        exp = name;
}

WatchItem *WatchItem::parentItem() const
{
    return static_cast<WatchItem *>(parent());
}

// Format a tooltip row with aligned colon.
static void formatToolTipRow(QTextStream &str, const QString &category, const QString &value)
{
    QString val = value.toHtmlEscaped();
    val.replace(QLatin1Char('\n'), QLatin1String("<br>"));
    str << "<tr><td>" << category << "</td><td>";
    if (!category.isEmpty())
        str << ':';
    str << "</td><td>" << val << "</td></tr>";
}

QString WatchItem::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, tr("Name"), name);
    formatToolTipRow(str, tr("Expression"), expression());
    formatToolTipRow(str, tr("Internal Type"), type);
    bool ok;
    const quint64 intValue = value.toULongLong(&ok);
    if (ok && intValue) {
        formatToolTipRow(str, tr("Value"), "(dec)  " + value);
        formatToolTipRow(str, QString(), "(hex)  " + QString::number(intValue, 16));
        formatToolTipRow(str, QString(), "(oct)  " + QString::number(intValue, 8));
        formatToolTipRow(str, QString(), "(bin)  " + QString::number(intValue, 2));
    } else {
        QString val = value;
        if (val.size() > 1000) {
            val.truncate(1000);
            val += QLatin1Char(' ');
            val += tr("... <cut off>");
        }
        formatToolTipRow(str, tr("Value"), val);
    }
    if (address)
        formatToolTipRow(str, tr("Object Address"), formatToolTipAddress(address));
    if (origaddr)
        formatToolTipRow(str, tr("Pointer Address"), formatToolTipAddress(origaddr));
    if (arrayIndex >= 0)
        formatToolTipRow(str, tr("Array Index"), QString::number(arrayIndex));
    if (size)
        formatToolTipRow(str, tr("Static Object Size"), tr("%n bytes", 0, size));
    formatToolTipRow(str, tr("Internal ID"), internalName());
    str << "</table></body></html>";
    return res;
}

bool WatchItem::isLocal() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isLocal();
    return iname.startsWith("local.");
}

bool WatchItem::isWatcher() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isWatcher();
    return iname.startsWith("watch.");
}

bool WatchItem::isInspect() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parentItem())
            return p->isInspect();
    return iname.startsWith("inspect.");
}

QString WatchItem::internalName() const
{
    if (arrayIndex >= 0) {
        if (const WatchItem *p = parentItem())
            return p->iname + '.' + QString::number(arrayIndex);
    }
    return iname;
}

QString WatchItem::realName() const
{
    if (arrayIndex >= 0)
        return QString::fromLatin1("[%1]").arg(arrayIndex);
    return name;
}

QString WatchItem::expression() const
{
    if (!exp.isEmpty())
         return exp;
    if (quint64 addr = address) {
        if (!type.isEmpty())
            return QString("*(%1*)0x%2").arg(type).arg(addr, 0, 16);
    }
    const WatchItem *p = parentItem();
    if (p && !p->exp.isEmpty())
        return QString("(%1).%2").arg(p->exp, name);
    return name;
}

} // namespace Internal
} // namespace Debugger

