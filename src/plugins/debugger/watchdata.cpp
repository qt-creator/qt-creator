// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchdata.h"

#include "watchutils.h"
#include "debuggerprotocol.h"
#include "debuggertr.h"

#include <QDebug>
#include <QtEndian>

namespace Debugger::Internal {

const QStringView inameLocal = u"local.";
const QStringView inameWatch = u"watch.";
const QStringView inameInspect = u"inspect.";

bool isPointerType(const QStringView type)
{
    return type.endsWith('*') || type.endsWith(u"* const");
}

bool isIntType(const QStringView type)
{
    if (type.isEmpty())
        return false;
    switch (type.at(0).unicode()) {
        case 'b':
            return type == u"bool";
        case 'c':
            return type.startsWith(u"char") &&
                    (  type == u"char"
                    || type == u"char8_t"
                    || type == u"char16_t"
                    || type == u"char32_t" );
        case 'i':
            return type.startsWith(u"int") &&
                    (  type == u"int"
                    || type == u"int8_t"
                    || type == u"int16_t"
                    || type == u"int32_t"
                    || type == u"int64_t");
        case 'l':
            return type == u"long"
                || type == u"long int"
                || type == u"long unsigned int";
        case 'p':
            return type == u"ptrdiff_t";
        case 'q':
            return type == u"qint8" || type == u"quint8"
                || type == u"qint16" || type == u"quint16"
                || type == u"qint32" || type == u"quint32"
                || type == u"qint64" || type == u"quint64"
                || type == u"qlonglong" || type == u"qulonglong";
        case 's':
            return type == u"short"
                || type == u"signed"
                || type == u"size_t"
                || type == u"std::size_t"
                || type == u"std::ptrdiff_t"
                || (type.startsWith(u"signed") &&
                    (  type == u"signed char"
                    || type == u"signed short"
                    || type == u"signed short int"
                    || type == u"signed long"
                    || type == u"signed long int"
                    || type == u"signed long long"
                    || type == u"signed long long int"));
        case 'u':
            return type == u"unsigned"
                || (type.startsWith(u"unsigned") &&
                    (  type == u"unsigned char"
                    || type == u"unsigned short"
                    || type == u"unsigned short int"
                    || type == u"unsigned int"
                    || type == u"unsigned long"
                    || type == u"unsigned long int"
                    || type == u"unsigned long long"
                    || type == u"unsigned long long int"))
                || (type.startsWith(u"uint") &&
                    (  type == u"uint8_t"
                    || type == u"uint16_t"
                    || type == u"uint32_t"
                    || type == u"uint64_t"));
        default:
            return false;
    }
}

bool isFloatType(const QStringView type)
{
    return type == u"float" || type == u"double" || type == u"qreal" || type == u"number";
}

bool isIntOrFloatType(const QStringView type)
{
    return isIntType(type) || isFloatType(type);
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
    if (value == "{...}") {
        value.clear();
        wantsChildren = true; // at least one...
    }
}

QString WatchItem::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << '{';
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
    if (res.endsWith(','))
        res.truncate(res.size() - 1);
    return res + '}';
}

QString WatchItem::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    //: Display of variables shadowed by variables of the same name
    //: in nested scopes: Variable %1 is the variable name, %2 is a
    //: simple count.
    return Tr::tr("%1 <shadowed %2>").arg(name).arg(seen);
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

enum class Endian { Little, Big };

class ArrayDataDecoder
{
public:
    template <class T>
    void decodeArrayHelper(int childSize)
    {
        const QByteArray ba = QByteArray::fromHex(rawData.toUtf8());
        const auto p = (const T*)ba.data();
        for (int i = 0, n = int(ba.size() / sizeof(T)); i < n; ++i) {
            auto child = new WatchItem;
            child->arrayIndex = i;
            child->value = decodeItemHelper(endian == Endian::Big ? qFromBigEndian(p[i])
                                                                  : qFromLittleEndian(p[i]));
            child->size = childSize;
            child->type = childType;
            child->address = addrbase + i * addrstep;
            child->valueEditable = true;
            item->appendChild(child);
        }
        if (childrenElided) {
            auto child = new WatchItem;
            child->name = WatchItem::loadMoreName;
            child->iname = item->iname + "." + WatchItem::loadMoreName;
            child->wantsChildren = true;
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
                break;
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
    int childrenElided;
    quint64 addrbase;
    quint64 addrstep;
    Endian endian;
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

    // We need it for UVSC engine!
    mi = input["id"];
    if (mi.isValid())
        id = mi.toInt();

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
            if (iname.startsWith(inameLocal) && iname.count('.') == 1)
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

    mi = input["time"];
    if (mi.isValid())
        time = mi.data().toFloat();

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

    mi = input["autoderefcount"];
    if (mi.isValid()) {
        bool ok = false;
        uint derefCount = mi.data().toUInt(&ok);
        if (ok)
            autoDerefCount = derefCount;
    }

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
        decoder.childrenElided = input["childrenelided"].toInt();
        decoder.addrbase = input["addrbase"].toAddress();
        decoder.addrstep = input["addrstep"].toAddress();
        decoder.endian = input["endian"].data() == ">" ? Endian::Big : Endian::Little;
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

            int i = -1;
            for (const GdbMi &subinput : children) {
                ++i;
                auto child = new WatchItem;
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

    time = data["time"].data().toFloat();
}

// Format a tooltip row with aligned colon.
static void formatToolTipRow(QTextStream &str, const QString &category, const QString &value)
{
    QString val = value.toHtmlEscaped();
    val.replace('\n', "<br>");
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
    formatToolTipRow(str, Tr::tr("Name"), name);
    formatToolTipRow(str, Tr::tr("Expression"), expression());
    formatToolTipRow(str, Tr::tr("Internal Type"), type);
    bool ok;
    const quint64 intValue = value.toULongLong(&ok);
    if (ok && intValue) {
        formatToolTipRow(str, Tr::tr("Value"), "(dec)  " + value);
        formatToolTipRow(str, QString(), "(hex)  " + QString::number(intValue, 16));
        formatToolTipRow(str, QString(), "(oct)  " + QString::number(intValue, 8));
        formatToolTipRow(str, QString(), "(bin)  " + QString::number(intValue, 2));
    } else {
        QString val = value;
        if (val.size() > 1000) {
            val.truncate(1000);
            val += ' ';
            val += Tr::tr("... <cut off>");
        }
        formatToolTipRow(str, Tr::tr("Value"), val);
    }
    if (address)
        formatToolTipRow(str, Tr::tr("Object Address"), formatToolTipAddress(address));
    if (origaddr)
        formatToolTipRow(str, Tr::tr("Pointer Address"), formatToolTipAddress(origaddr));
    if (arrayIndex >= 0)
        formatToolTipRow(str, Tr::tr("Array Index"), QString::number(arrayIndex));
    if (size)
        formatToolTipRow(str, Tr::tr("Static Object Size"), Tr::tr("%n bytes", nullptr, size));
    formatToolTipRow(str, Tr::tr("Internal ID"), internalName());
    formatToolTipRow(str, Tr::tr("Creation Time in ms"), QString::number(int(time * 1000)));
    formatToolTipRow(str, Tr::tr("Source"), sourceExpression());
    str << "</table></body></html>";
    return res;
}

bool WatchItem::isLoadMore() const
{
    return name == loadMoreName;
}

bool WatchItem::isLocal() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parent())
            return p->isLocal();
    return iname.startsWith(inameLocal);
}

bool WatchItem::isWatcher() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parent())
            return p->isWatcher();
    return iname.startsWith(inameWatch);
}

bool WatchItem::isInspect() const
{
    if (arrayIndex >= 0)
        if (const WatchItem *p = parent())
            return p->isInspect();
    return iname.startsWith(inameInspect);
}

QString WatchItem::internalName() const
{
    if (arrayIndex >= 0) {
        if (const WatchItem *p = parent())
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
    const WatchItem *p = parent();
    if (p && !p->exp.isEmpty())
        return QString("(%1).%2").arg(p->exp, name);
    return name;
}

QString WatchItem::sourceExpression() const
{
    const WatchItem *p = parent();
    if (!p)
        return {}; // Root

    const WatchItem *pp = p->parent();
    if (!pp)
        return {}; // local

    const WatchItem *ppp = pp->parent();
    if (!ppp)
        return name; // local.x -> 'x'

    // Enforce some arbitrary, but fixed limit to avoid excessive creation
    // of very likely unused strings which are for convenience only.
    if (arrayIndex >= 0 && arrayIndex <= 16)
        return QString("%1[%2]").arg(p->sourceExpression()).arg(arrayIndex);

    if (p->name == '*')
        return QString("%1->%2").arg(pp->sourceExpression(), name);

    return QString("%1.%2").arg(p->sourceExpression(), name);
}

int WatchItem::guessSize() const
{
    if (size != 0)
        return size;
    if (type == "double")
        return 8;
    if (type == "float")
        return 4;
    if (type == "qfloat16")
        return 2;
    return 0;
}

} // Debugger::Internal

