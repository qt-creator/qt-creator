/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "symbolgroupvalue.h"
#include "symbolgroup.h"
#include "stringutils.h"

#include <iomanip>

SymbolGroupValue::SymbolGroupValue(SymbolGroupNode *node,
                                   const SymbolGroupValueContext &ctx) :
    m_node(node), m_context(ctx)
{
}

SymbolGroupValue::SymbolGroupValue() :
    m_node(0), m_errorMessage("Invalid")
{
}

bool SymbolGroupValue::isValid() const
{
    return m_node != 0 && m_context.dataspaces != 0;
}

SymbolGroupValue SymbolGroupValue::operator[](unsigned index) const
{
    if (ensureExpanded())
        if (index < m_node->children().size())
            return SymbolGroupValue(m_node->children().at(index), m_context);
    return SymbolGroupValue();
}

bool SymbolGroupValue::ensureExpanded() const
{
    if (!isValid() || !m_node->canExpand())
        return false;

    if (m_node->isExpanded())
        return true;

    // Set a flag indicating the node was expanded by SymbolGroupValue
    // and not by an explicit request from the watch model.
    if (m_node->expand(&m_errorMessage)) {
        m_node->addFlags(SymbolGroupNode::ExpandedByDumper);
        return true;
    }
    return false;
}

SymbolGroupValue SymbolGroupValue::operator[](const char *name) const
{
    if (ensureExpanded())
        if (SymbolGroupNode *child = m_node->childByIName(name))
            return SymbolGroupValue(child, m_context);
    return SymbolGroupValue();
}

std::string SymbolGroupValue::type() const
{
    return isValid() ? m_node->type() : std::string();
}

unsigned SymbolGroupValue::size() const
{
    return isValid() ? m_node->size() : 0;
}

std::wstring SymbolGroupValue::value() const
{
    return isValid() ? m_node->symbolGroupFixedValue() : std::wstring();
}

double SymbolGroupValue::floatValue(double defaultValue) const
{
    double f = defaultValue;
    if (isValid()) {
        std::wistringstream str(value());
        str >> f;
        if (str.fail())
            f = defaultValue;
    }
    return f;
}

int SymbolGroupValue::intValue(int defaultValue) const
{
    if (isValid()) {
        int rc = 0;
        // Is this an enumeration "EnumValue (0n12)", -> convert to integer
        std::wstring v = value();
        const std::wstring::size_type enPos = v.find(L"(0n");
        if (enPos != std::wstring::npos && v.at(v.size() - 1) == L')')
            v = v.substr(enPos + 3, v.size() - 4);
        if (integerFromString(wStringToString(v), &rc))
            return rc;
    }
    return defaultValue;
}

ULONG64 SymbolGroupValue::pointerValue(ULONG64 defaultValue) const
{
    if (isValid()) {
        ULONG64 rc = 0;
        if (integerFromString(wStringToString(value()), &rc))
            return rc;
    }
    return defaultValue;
}

// Return allocated array of data
unsigned char *SymbolGroupValue::pointerData(unsigned length) const
{
    if (isValid()) {
        if (const ULONG64 ptr = pointerValue()) {
            unsigned char *data = new unsigned char[length];
            std::fill(data, data + length, 0);
            const HRESULT hr = m_context.dataspaces->ReadVirtual(ptr, data, length, NULL);
            if (FAILED(hr)) {
                delete [] data;
                return 0;
            }
            return data;
        }
    }
    return 0;
}

ULONG64 SymbolGroupValue::address() const
{
    if (isValid())
        return m_node->address();
    return 0;
}

// Temporary iname
static inline std::string additionalSymbolIname(const SymbolGroup *g)
{
    std::ostringstream str;
    str << "__additional" << g->root()->children().size();
    return str.str();
}

SymbolGroupValue SymbolGroupValue::typeCast(const char *type) const
{
    return typeCastedValue(address(), type);
}

SymbolGroupValue SymbolGroupValue::pointerTypeCast(const char *type) const
{
    return typeCastedValue(pointerValue(), type);
}

SymbolGroupValue SymbolGroupValue::typeCastedValue(ULONG64 address, const char *type) const
{
    if (address) {
        SymbolGroup *sg = m_node->symbolGroup();
        std::ostringstream str;
        str << '(' << type << ")(" << std::showbase << std::hex << address << ')';
        if (SymbolGroupNode *node = sg->addSymbol(str.str(),
                                                  additionalSymbolIname(sg),
                                                  &m_errorMessage))
            return SymbolGroupValue(node, m_context);
    }
    return SymbolGroupValue();
}

std::wstring SymbolGroupValue::wcharPointerData(unsigned charCount, unsigned maxCharCount) const
{
    const bool truncated = charCount > maxCharCount;
    if (truncated)
        charCount = maxCharCount;
    if (const unsigned char *ucData = pointerData(charCount * sizeof(wchar_t))) {
        const wchar_t *utf16Data = reinterpret_cast<const wchar_t *>(ucData);
        // Be smart about terminating 0-character
        if (charCount && utf16Data[charCount - 1] == 0)
            charCount--;
        std::wstring rc = std::wstring(utf16Data, charCount);
        if (truncated)
            rc += L"...";
        delete [] ucData;
        return rc;
    }
    return std::wstring();
}

std::string SymbolGroupValue::error() const
{
    return m_errorMessage;
}

std::string SymbolGroupValue::stripPointerType(const std::string &t)
{
    return endsWith(t, " *") ? t.substr(0, t.size() - 2) : t;
}

std::string SymbolGroupValue::stripArrayType(const std::string &t)
{
    const std::string::size_type bracket = t.rfind('[');
    if (bracket != std::string::npos) {
        std::string rc = t.substr(0, bracket);
        trimBack(rc);
        return rc;
    }
    return t;
}

// get the inner types: "QMap<int, double>" -> "int", "double"
std::vector<std::string> SymbolGroupValue::innerTypesOf(const std::string &t)
{
    std::vector<std::string> rc;

    std::string::size_type pos = t.find('<');
    if (pos == std::string::npos)
        return rc;

    rc.reserve(5);
    const std::string::size_type size = t.size();
    // Record all elements of level 1 to work correctly for
    // 'std::map<std::basic_string< .. > >'
    unsigned level = 0;
    std::string::size_type start = 0;
    for ( ; pos < size ; pos++) {
        const char c = t.at(pos);
        switch (c) {
        case '<':
            if (++level == 1)
                start = pos + 1;
            break;
        case '>':
            if (--level == 0) { // last element
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                return rc;
            }
            break;
        case ',':
            if (level == 1) { // std::map<a, b>: start anew at ','.
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                start = pos + 1;
            }
            break;
        }
    }
    return rc;
}

// -------------------- Simple dumping helpers

// Courtesy of qdatetime.cpp
static inline void getDateFromJulianDay(unsigned julianDay, int *year, int *month, int *day)
{
    int y, m, d;

    if (julianDay >= 2299161) {
        typedef unsigned long long qulonglong;
        // Gregorian calendar starting from October 15, 1582
        // This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
        qulonglong ell, n, i, j;
        ell = qulonglong(julianDay) + 68569;
        n = (4 * ell) / 146097;
        ell = ell - (146097 * n + 3) / 4;
        i = (4000 * (ell + 1)) / 1461001;
        ell = ell - (1461 * i) / 4 + 31;
        j = (80 * ell) / 2447;
        d = int(ell - (2447 * j) / 80);
        ell = j / 11;
        m = int(j + 2 - (12 * ell));
        y = int(100 * (n - 49) + i + ell);
    } else {
        // Julian calendar until October 4, 1582
        // Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
        julianDay += 32082;
        int dd = (4 * julianDay + 3) / 1461;
        int ee = julianDay - (1461 * dd) / 4;
        int mm = ((5 * ee) + 2) / 153;
        d = ee - (153 * mm + 2) / 5 + 1;
        m = mm + 3 - 12 * (mm / 10);
        y = dd - 4800 + (mm / 10);
        if (y <= 0)
            --y;
    }
    if (year)
        *year = y;
    if (month)
        *month = m;
    if (day)
        *day = d;
}

// Convert and format Julian Date as used in QDate
static inline void formatJulianDate(std::wostream &str, unsigned julianDate)
{
    int y, m, d;
    getDateFromJulianDay(julianDate, &y, &m, &d);
    str << d << '.' << m << '.' << y;
}

// Format time in milliseconds as "hh:dd:ss:mmm"
static inline void formatMilliSeconds(std::wostream &str, int milliSecs)
{
    const int hourFactor = 1000 * 3600;
    const int hours = milliSecs / hourFactor;
    milliSecs = milliSecs % hourFactor;
    const int minFactor = 1000 * 60;
    const int minutes = milliSecs / minFactor;
    milliSecs = milliSecs % minFactor;
    const int secs = milliSecs / 1000;
    milliSecs = milliSecs % 1000;
    str.fill('0');
    str << std::setw(2) << hours << ':' << std::setw(2)
        << minutes << ':' << std::setw(2) << secs
        << '.' << std::setw(3) << milliSecs;
}


static const char stdStringTypeC[] = "class std::basic_string<char,std::char_traits<char>,std::allocator<char> >";
static const char stdWStringTypeC[] = "class std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >";

// Dump a QString.

KnownType knownType(const std::string &type)
{
    // Make sure this is 'class X' or 'struct X'. Strip that and pointer
    if (type.empty() || (type.at(0) != 'c' && type.at(0) != 's'))
        return KT_Unknown;
    const bool isClass = type.compare(0, 6, "class ") != 0;
    const bool isStruct = isClass ? false : type.compare(0, 7, "struct ") != 0;
    if (!isClass && !isStruct)
        return KT_Unknown;
    // Strip pointer types.
    const std::wstring::size_type compareLen =
            endsWith(type, " *") ? type.size() -2 : type.size();
    // STL ?
    const std::wstring::size_type templatePos = type.find('<');
    static const std::wstring::size_type stlClassPos = 11;
    if (!type.compare(0, stlClassPos, "class std::")) {
        // STL containers
        if (templatePos != std::string::npos) {
            switch (templatePos - stlClassPos) {
            case 3:
                if (!type.compare(stlClassPos, 3, "set"))
                    return KT_StdSet;
                if (!type.compare(stlClassPos, 3, "map"))
                    return KT_StdMap;
                break;
            case 4:
                if (!type.compare(stlClassPos, 4, "list"))
                    return KT_StdList;
                break;
            case 6:
                if (!type.compare(stlClassPos, 6, "vector"))
                    return KT_StdVector;
                break;
            case 8:
                if (!type.compare(stlClassPos, 8, "multimap"))
                    return KT_StdMultiMap;
                break;
            }
        }
        // STL strings
        if (!type.compare(0, compareLen, stdStringTypeC))
            return KT_StdString;
        if (!type.compare(0, compareLen, stdWStringTypeC))
            return KT_StdWString;
        return KT_Unknown;
    } // std::sth
    // Check for a 'Q' past the last namespace (beware of namespaced Qt:
    // 'nsp::QString').
    const std::wstring::size_type lastNameSpacePos = type.rfind(':');
    const std::wstring::size_type qPos =
            lastNameSpacePos == std::string::npos ? type.find('Q') : lastNameSpacePos + 1;
    if (qPos == std::string::npos || qPos >= type.size() || type.at(qPos) != 'Q')
        return KT_Unknown;
    // Qt types (templates)
    if (templatePos != std::string::npos) {
        switch (templatePos - qPos) {
        case 4:
            if (!type.compare(qPos, 4, "QSet"))
                return KT_QSet;
            if (!type.compare(qPos, 4, "QMap"))
                return KT_QMap;
            break;
        case 5:
            if (!type.compare(qPos, 5, "QHash"))
                return KT_QHash;
            if (!type.compare(qPos, 5, "QList"))
                return KT_QList;
            break;
        case 6:
            if (!type.compare(qPos, 6, "QFlags"))
                return KT_QFlags;
            break;
        case 7:
            if (!type.compare(qPos, 7, "QVector"))
                return KT_QVector;
            break;
        case 9:
            if (!type.compare(qPos, 9, "QMultiMap"))
                return KT_QMultiMap;
            break;
        }
    }
    // Remaining non-template types
    switch (compareLen - qPos) {
    case 5:
        if (!type.compare(qPos, 5, "QChar"))
            return KT_QChar;
        if (!type.compare(qPos, 5, "QDate"))
            return KT_QDate;
        if (!type.compare(qPos, 5, "QTime"))
            return KT_QTime;
        if (!type.compare(qPos, 5, "QSize"))
            return KT_QSize;
        if (!type.compare(qPos, 5, "QLine"))
            return KT_QLine;
        if (!type.compare(qPos, 5, "QRect"))
            return KT_QRect;
        break;
    case 6:
        if (!type.compare(qPos, 6, "QColor"))
            return KT_QColor;
        if (!type.compare(qPos, 6, "QSizeF"))
            return KT_QSizeF;
        if (!type.compare(qPos, 6, "QPoint"))
            return KT_QPoint;
        if (!type.compare(qPos, 6, "QLineF"))
            return KT_QLineF;
        if (!type.compare(qPos, 6, "QRectF"))
            return KT_QRectF;
        break;
    case 7:
        if (!type.compare(qPos, 7, "QString"))
            return KT_QString;
        if (!type.compare(qPos, 7, "QPointF"))
            return KT_QPointF;
        if (!type.compare(qPos, 7, "QObject"))
            return KT_QObject;
        if (!type.compare(qPos, 7, "QWidget"))
            return KT_QWidget;
        break;
    case 8:
        if (!type.compare(qPos, 8, "QVariant"))
            return KT_QVariant;
        break;
    case 10:
        if (!type.compare(qPos, 10, "QAtomicInt"))
            return KT_QAtomicInt;
        if (!type.compare(qPos, 10, "QByteArray"))
            return KT_QByteArray;
        break;
    case 11:
        if (!type.compare(qPos, 11, "QStringList"))
            return KT_QStringList;
        break;
    case 15:
        if (!type.compare(qPos, 15, "QBasicAtomicInt"))
            return KT_QBasicAtomicInt;
        break;
    }
    return KT_Unknown;
}

static inline bool dumpQString(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue d = v["d"]) {
        if (const SymbolGroupValue sizeValue = d["size"]) {
            const int size = sizeValue.intValue();
            if (size >= 0) {
                str << d["data"].wcharPointerData(size);
                return true;
            }
        }
    }
    return false;
}

// Dump QColor
static bool dumpQColor(const SymbolGroupValue &v, std::wostream &str)
{
    const SymbolGroupValue specV = v["cspec"];
    if (!specV)
        return false;
    const int spec = specV.intValue();
    if (spec == 0) {
        str << L"<Invalid color>";
        return true;
    }
    if (spec < 1 || spec > 4)
        return false;
    const SymbolGroupValue arrayV = v["ct"]["array"];
    if (!arrayV)
        return false;
    const int a0 = arrayV["0"].intValue();
    const int a1 = arrayV["1"].intValue();
    const int a2 = arrayV["2"].intValue();
    const int a3 = arrayV["3"].intValue();
    const int a4 = arrayV["4"].intValue();
    if (a0 < 0 || a1 < 0 || a2 < 0 || a3 < 0 || a4 < 0)
        return false;
    switch (spec) {
    case 1: // Rgb
        str << L"RGB alpha=" << (a0 / 0x101) << L", red=" << (a1 / 0x101)
            << L", green=" << (a2 / 0x101) << ", blue=" << (a3 / 0x101);
        break;
    case 2: // Hsv
        str << L"HSV alpha=" << (a0 / 0x101) << L", hue=" << (a1 / 100)
            << L", sat=" << (a2 / 0x101) << ", value=" << (a3 / 0x101);
        break;
    case 3: // Cmyk
        str << L"CMYK alpha=" << (a0 / 0x101) << L", cyan=" << (a1 / 100)
            << L", magenta=" << (a2 / 0x101) << ", yellow=" << (a3 / 0x101)
            << ", black=" << (a4 / 0x101);
        break;
    case 4: // Hsl
        str << L"HSL alpha=" << (a0 / 0x101) << L", hue=" << (a1 / 100)
            << L", sat=" << (a2 / 0x101) << ", lightness=" << (a3 / 0x101);
        break;
    }
    return true;
}

// Dump Qt's core types

static inline bool dumpQBasicAtomicInt(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue iValue = v["_q_value"]) {
        str <<  iValue.value();
        return true;
    }
    return false;
}

static inline bool dumpQAtomicInt(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue base = v[unsigned(0)])
        return dumpQBasicAtomicInt(base, str);
    return false;
}

static bool dumpQChar(const SymbolGroupValue &v, std::wostream &str)
{
    if (SymbolGroupValue cValue = v["ucs"]) {
        const int utf16 = cValue.intValue();
        if (utf16 >= 0) {
            // Print code = character,
            // exclude control characters and Pair indicator
            str << utf16;
            if (utf16 >= 32 && (utf16 < 0xD800 || utf16 > 0xDBFF))
                str << " '" << wchar_t(utf16) << '\'';
        }
        return true;
    }
    return false;
}

static inline bool dumpQFlags(const SymbolGroupValue &v, std::wostream &str)
{
    if (SymbolGroupValue iV = v["i"]) {
        const int i = iV.intValue();
        if (i >= 0) {
            str << i;
            return true;
        }
    }
    return false;
}

static inline bool dumpQDate(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue julianDayV = v["jd"]) {
        const int julianDay = julianDayV.intValue();
        if (julianDay > 0) {
            formatJulianDate(str, julianDay);
            return true;
        }
    }
    return false;
}

static bool dumpQTime(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue milliSecsV = v["mds"]) {
        const int milliSecs = milliSecsV.intValue();
        if (milliSecs >= 0) {
            formatMilliSeconds(str, milliSecs);
            return true;
        }
    }
    return false;
}

// Dump a QByteArray
static inline bool dumpQByteArray(const SymbolGroupValue &v, std::wostream &str)
{
    // TODO: More sophisticated dumping of binary data?
    if (const  SymbolGroupValue data = v["d"]["data"]) {
        str << data.value();
        return true;
    }
    return false;
}

// Dump a rectangle in X11 syntax
template <class T>
inline void dumpRect(std::wostream &str, T x, T y, T width, T height)
{
    str << width << 'x' << height;
    if (x >= 0)
        str << '+';
    str << x;
    if (y >= 0)
        str << '+';
    str << y;
}

template <class T>
inline void dumpRectPoints(std::wostream &str, T x1, T y1, T x2, T y2)
{
    dumpRect(str, x1, y1, (x2 - x1), (y2 - y1));
}

// Dump Qt's simple geometrical types
static inline bool dumpQSize_F(const SymbolGroupValue &v, std::wostream &str)
{
    str << '(' << v["wd"].value() << ", " << v["ht"].value() << ')';
    return true;
}

static inline bool dumpQPoint_F(const SymbolGroupValue &v, std::wostream &str)
{
    str << '(' << v["xp"].value() << ", " << v["yp"].value() << ')';
    return true;
}

static inline bool dumpQLine_F(const SymbolGroupValue &v, std::wostream &str)
{
    const SymbolGroupValue p1 = v["pt1"];
    const SymbolGroupValue p2 = v["pt2"];
    if (p1 && p2) {
        str << '(' << p1["xp"].value() << ", " << p1["yp"].value() << ") ("
            << p2["xp"].value() << ", " << p2["yp"].value() << ')';
        return true;
    }
    return false;
}

static inline bool dumpQRect(const SymbolGroupValue &v, std::wostream &str)
{
    dumpRectPoints(str, v["x1"].intValue(), v["y1"].intValue(), v["x2"].intValue(), v["y2"].intValue());
    return true;
}

static inline bool dumpQRectF(const SymbolGroupValue &v, std::wostream &str)
{
    dumpRect(str, v["xp"].floatValue(), v["yp"].floatValue(), v["w"].floatValue(), v["h"].floatValue());
    return true;
}

// Dump the object name
static inline bool dumpQObject(const SymbolGroupValue &v, std::wostream &str)
{
    if (SymbolGroupValue oName = v["d_ptr"]["d"].pointerTypeCast("QObjectPrivate *")["objectName"]) {
        str << L'"';
        dumpQString(oName, str);
        str << L'"';
        return true;
    }
    return false;
}

// Dump a std::string.
static bool dumpStd_W_String(const SymbolGroupValue &v, std::wostream &str)
{
    // MSVC 2010: Access Bx/_Buf in base class
    SymbolGroupValue buf = v[unsigned(0)]["_Bx"]["_Buf"];
    if (!buf) // MSVC2008: Bx/Buf are members
        buf = v["_Bx"]["_Buf"];
    if (buf) {
        str << buf.value();
        return true;
    }
    return false;
}

// QVariant employs a template for storage where anything bigger than the data union
// is pointed to by data.shared.ptr, else it is put into the data struct (pointer size)
// itself (notably Qt types consisting of a d-ptr only).
// The layout can vary between 32 /64 bit for some types: QPoint/QSize (of 2 ints) is bigger
// as a pointer only on 32 bit.

static inline SymbolGroupValue qVariantCast(const SymbolGroupValue &variantData, const char *type)
{
    const ULONG typeSize = SymbolGroupValue::sizeOf(type);
    const std::string ptrType = std::string(type) + " *";
    if (typeSize > variantData.size())
        return variantData["shared"]["ptr"].pointerTypeCast(ptrType.c_str());
    return variantData.typeCast(ptrType.c_str());
}

static bool dumpQVariant(const SymbolGroupValue &v, std::wostream &str)
{
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return false;
    const SymbolGroupValue typeV = dV["type"];
    const SymbolGroupValue dataV = dV["data"];
    if (!typeV || !dataV)
        return false;
    const int typeId = typeV.intValue();
    if (typeId <= 0) {
        str <<  L"<Invalid>";
        return true;
    }
    switch (typeId) {
    case 1: // Bool
        str << L"(bool) " << dataV["b"].value();
        break;
    case 2: // Int
        str << L"(int) " << dataV["i"].value();
        break;
    case 3: // UInt
        str << L"(unsigned) " << dataV["u"].value();
        break;
    case 4: // LongLong
        str << L"(long long) " << dataV["ll"].value();
        break;
    case 5: // LongLong
        str << L"(unsigned long long) " << dataV["ull"].value();
        break;
    case 6: // Double
        str << L"(double) " << dataV["d"].value();
        break;
    case 7: // Char
        str << L"(char) " << dataV["c"].value();
        break;
    case 10: // String
        str << L"(QString) \"";
        if (const SymbolGroupValue sv = dataV.typeCast("QString *")) {
            dumpQString(sv, str);
            str << L'"';
        }
        break;
    case 12: //ByteArray
        str << L"(QByteArray) ";
        if (const SymbolGroupValue sv = dataV.typeCast("QByteArray *"))
            dumpQByteArray(sv, str);
        break;
    case 13: // BitArray
        str << L"(QBitArray)";
        break;
    case 14: // Date
        str << L"(QDate) ";
        if (const SymbolGroupValue sv = dataV.typeCast("QDate *"))
            dumpQDate(sv, str);
        break;
    case 15: // Time
        str << L"(QTime) ";
        if (const SymbolGroupValue sv = dataV.typeCast("QTime *"))
            dumpQTime(sv, str);
        break;
    case 16: // DateTime
        str << L"(QDateTime)";
        break;
    case 17: // Url
        str << L"(QUrl)";
        break;
    case 18: // Locale
        str << L"(QLocale)";
        break;
    case 19: // Rect:
        str << L"(QRect) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QRect *"))
            dumpQRect(sv, str);
        break;
    case 20: // RectF
        str << L"(QRectF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QRectF *"))
            dumpQRectF(sv, str);
        break;
    case 21: // Size
        // Anything bigger than the data union is a pointer, else the data union is used
        str << L"(QSize) ";
        if (const SymbolGroupValue sv = qVariantCast(dataV, "QSize"))
            dumpQSize_F(sv, str);
        break;
    case 22: // SizeF
        str << L"(QSizeF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QSizeF *"))
            dumpQSize_F(sv, str);
        break;
    case 23: // Line
        str << L"(QLine) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QLine *"))
            dumpQLine_F(sv, str);
        break;
    case 24: // LineF
        str << L"(QLineF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QLineF *"))
            dumpQLine_F(sv, str);
        break;
    case 25: // Point
        str << L"(QPoint) ";
        if (const SymbolGroupValue sv = qVariantCast(dataV, "QPoint"))
            dumpQPoint_F(sv, str);
        break;
    case 26: // PointF
        str << L"(QPointF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast("QPointF *"))
            dumpQPoint_F(sv, str);
        break;
    default:
        str << L"Type " << typeId;
        break;
    }
    return true;
}

// Return size of container or -1

int containerSize(KnownType kt, SymbolGroupNode *n, const SymbolGroupValueContext &ctx)
{
    if ((kt & KT_ContainerType) == 0)
        return -1;
    return containerSize(kt, SymbolGroupValue(n, ctx));
}

// Return size from an STL vector (last/first iterators).
static inline int msvcStdVectorSize(const SymbolGroupValue &v)
{
    if (const SymbolGroupValue myFirstPtrV = v["_Myfirst"]) {
        if (const SymbolGroupValue myLastPtrV = v["_Mylast"]) {
            const ULONG64 firstPtr = myFirstPtrV.pointerValue();
            const ULONG64 lastPtr = myLastPtrV.pointerValue();
            if (!firstPtr || lastPtr < firstPtr)
                return -1;
            if (lastPtr == firstPtr)
                return 0;
            // Subtract the pointers: We need to do the pointer arithmetics ourselves
            // as we get char *pointers.
            const std::string innerType = SymbolGroupValue::stripPointerType(myFirstPtrV.type());
            const size_t size = SymbolGroupValue::sizeOf(innerType.c_str());
            if (size == 0)
                return -1;
            return static_cast<int>((lastPtr - firstPtr) / size);
        }
    }
    return -1;
}

int containerSize(KnownType kt, const SymbolGroupValue &v)
{
    switch (kt) {
    case KT_QStringList:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QList, base);
        break;
    case KT_QList:
        if (const SymbolGroupValue dV = v["d"]) {
            if (const SymbolGroupValue beginV = dV["begin"]) {
                const int begin = beginV.intValue();
                const int end = dV["end"].intValue();
                if (begin >= 0 && end >= begin)
                    return end - begin;
            }
        }
        break;
    case KT_QHash:
    case KT_QMap:
    case KT_QVector:
        if (const SymbolGroupValue sizeV = v["d"]["size"])
            return sizeV.intValue();
        break;
    case KT_QSet:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QHash, base);
        break;
    case KT_QMultiMap:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QMap, base);
        break;
    case KT_StdVector: {
        if (const SymbolGroupValue base = v[unsigned(0)]) {
            const int msvc10Size = msvcStdVectorSize(base);
            if (msvc10Size >= 0)
                return msvc10Size;
        }
        const int msvc8Size = msvcStdVectorSize(v);
        if (msvc8Size >= 0)
            return msvc8Size;
    }
        break;
    case KT_StdList:
        if (const SymbolGroupValue sizeV =  v["_Mysize"]) // VS 8
            return sizeV.intValue();
        if (const SymbolGroupValue sizeV = v[unsigned(0)][unsigned(0)]["_Mysize"]) // VS10
            return sizeV.intValue();
        break;
    case KT_StdSet:
    case KT_StdMap:
    case KT_StdMultiMap:
        if (const SymbolGroupValue baseV = v[unsigned(0)]) {
            if (const SymbolGroupValue sizeV = baseV["_Mysize"]) // VS 8
                return sizeV.intValue();
            if (const SymbolGroupValue sizeV = baseV[unsigned(0)][unsigned(0)]["_Mysize"]) // VS 10
                return sizeV.intValue();
        }
        break;
    }
    return -1;
}

static inline std::wstring msgContainerSize(int s)
{
    std::wostringstream str;
    str << L'<' << s << L" items>";
    return str.str();
}

// Dump builtin simple types using SymbolGroupValue expressions.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx,
                        std::wstring *s, int *knownTypeIn /* = 0 */,
                        int *containerSizeIn /* = 0 */)
{
    if (containerSizeIn)
        *containerSizeIn = -1;
    // Check for class types and strip pointer types (references appear as pointers as well)
    s->clear();
    const KnownType kt  = knownType(n->type());
    if (knownTypeIn)
        *knownTypeIn = kt;

    if (kt == KT_Unknown)
        return SymbolGroupNode::SimpleDumperNotApplicable;

    const SymbolGroupValue v(n, ctx);
    // Simple dump of size for containers
    if (kt & KT_ContainerType) {
        const int size = containerSize(kt, v);
        if (containerSizeIn)
            *containerSizeIn = size;
        if (size >= 0) {
            *s = msgContainerSize(size);
            return SymbolGroupNode::SimpleDumperOk;
        }
        return SymbolGroupNode::SimpleDumperFailed;
    }
    std::wostringstream str;
    unsigned rc = SymbolGroupNode::SimpleDumperNotApplicable;
    switch (kt) {
    case KT_QChar:
        rc = dumpQChar(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QByteArray:
        rc = dumpQByteArray(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QString:
        rc = dumpQString(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QColor:
        rc = dumpQColor(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QFlags:
        rc = dumpQFlags(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QDate:
        rc = dumpQDate(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QTime:
        rc = dumpQTime(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QPoint:
    case KT_QPointF:
        rc = dumpQPoint_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QSize:
    case KT_QSizeF:
        rc = dumpQSize_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QLine:
    case KT_QLineF:
        rc = dumpQLine_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QRect:
        rc = dumpQRect(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QRectF:
        rc = dumpQRectF(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QVariant:
        rc = dumpQVariant(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QAtomicInt:
        rc = dumpQAtomicInt(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QBasicAtomicInt:
        rc = dumpQBasicAtomicInt(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QObject:
        rc = dumpQObject(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QWidget:
        rc = dumpQObject(v[unsigned(0)], str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_StdString:
    case KT_StdWString:
        rc = dumpStd_W_String(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    default:
        break;
    }
    if (rc == SymbolGroupNode::SimpleDumperOk)
        *s = str.str();
    return rc;
}
