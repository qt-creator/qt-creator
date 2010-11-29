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
static inline void formatJulianDate(std::wostringstream &str, unsigned julianDate)
{
    int y, m, d;
    getDateFromJulianDay(julianDate, &y, &m, &d);
    str << d << '.' << m << '.' << y;
}

// Format time in milliseconds as "hh:dd:ss:mmm"
static inline void formatMilliSeconds(std::wostringstream &str, int milliSecs)
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
static unsigned dumpQString(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (!endsWith(type, "QString")) // namespaced Qt?
        return SymbolGroupNode::DumperNotApplicable;

    if (SymbolGroupValue d = v["d"]) {
        if (SymbolGroupValue sizeValue = d["size"]) {
            const int size = sizeValue.intValue();
            if (size >= 0) {
                *s = d["data"].wcharPointerData(size);
                return SymbolGroupNode::DumperOk;
            }
        }
    }
    return SymbolGroupNode::DumperFailed;
}

// Dump QColor
static unsigned dumpQColor(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (!endsWith(type, "QColor")) // namespaced Qt?
        return SymbolGroupNode::DumperNotApplicable;
    const SymbolGroupValue specV = v["cspec"];
    if (!specV)
        return SymbolGroupNode::DumperFailed;
    const int spec = specV.intValue();
    if (spec == 0) {
        *s = L"<Invalid color>";
        return SymbolGroupNode::DumperOk;
    }
    if (spec < 1 || spec > 4)
        return SymbolGroupNode::DumperFailed;
    const SymbolGroupValue arrayV = v["ct"]["array"];
    if (!arrayV)
        return SymbolGroupNode::DumperFailed;
    const int a0 = arrayV["0"].intValue();
    const int a1 = arrayV["1"].intValue();
    const int a2 = arrayV["2"].intValue();
    const int a3 = arrayV["3"].intValue();
    const int a4 = arrayV["4"].intValue();
    if (a0 < 0 || a1 < 0 || a2 < 0 || a3 < 0 || a4 < 0)
        return SymbolGroupNode::DumperFailed;
    std::wostringstream str;
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
    *s = str.str();
    return SymbolGroupNode::DumperOk;
}

// Dump Qt's core types
static unsigned dumpQtCoreTypes(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (endsWith(type, "QAtomicInt")) { // namespaced Qt?
        if (SymbolGroupValue iValue = v[unsigned(0)]["_q_value"]) {
            *s = iValue.value();
            return SymbolGroupNode::DumperOk;
        }
        return SymbolGroupNode::DumperFailed;
    }

    if (endsWith(type, "QChar")) {
        if (SymbolGroupValue cValue = v["ucs"]) {
            const int utf16 = cValue.intValue();
            if (utf16 >= 0) {
                // Print code = character,
                // exclude control characters and Pair indicator
                std::wostringstream str;
                str << utf16;
                if (utf16 >= 32 && (utf16 < 0xD800 || utf16 > 0xDBFF))
                    str << " '" << wchar_t(utf16) << '\'';
                *s = str.str();
            }
            return SymbolGroupNode::DumperOk;
        }
        return SymbolGroupNode::DumperFailed;
    }

    if (type.find("QFlags") != std::wstring::npos) {
        if (SymbolGroupValue iV = v["i"]) {
            const int i = iV.intValue();
            if (i >= 0) {
                *s = toWString(i);
                return SymbolGroupNode::DumperOk;
            }
        }
        return SymbolGroupNode::DumperFailed;
    }

    if (endsWith(type, "QDate")) {
        if (SymbolGroupValue julianDayV = v["jd"]) {
            const int julianDay = julianDayV.intValue();
            if (julianDay > 0) {
                std::wostringstream str;
                formatJulianDate(str, julianDay);
                *s = str.str();
                return SymbolGroupNode::DumperOk;
            }
        return SymbolGroupNode::DumperFailed;
        }
    }

    if (endsWith(type, "QTime")) {
        if (SymbolGroupValue milliSecsV = v["mds"]) {
            int milliSecs = milliSecsV.intValue();
            if (milliSecs >= 0) {
                std::wostringstream str;
                formatMilliSeconds(str, milliSecs);
                *s = str.str();
                return SymbolGroupNode::DumperOk;
            }
        return SymbolGroupNode::DumperFailed;
        }
    }

    return SymbolGroupNode::DumperNotApplicable;
}

// Dump a QByteArray
static unsigned dumpQByteArray(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (!endsWith(type, "QByteArray")) // namespaced Qt?
        return SymbolGroupNode::DumperNotApplicable;
    // TODO: More sophisticated dumping of binary data?
    if (SymbolGroupValue data = v["d"]["data"]) {
        *s = data.value();
        return SymbolGroupNode::DumperOk;
    }
    return SymbolGroupNode::DumperFailed;
}

// Dump a rectangle in X11 syntax
template <class T>
inline void dumpRect(std::wostringstream &str, T x, T y, T width, T height)
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
inline void dumpRectPoints(std::wostringstream &str, T x1, T y1, T x2, T y2)
{
    dumpRect(str, x1, y1, (x2 - x1), (y2 - y1));
}

// Dump Qt's simple geometrical types
static unsigned dumpQtGeometryTypes(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (endsWith(type, "QSize") || endsWith(type, "QSizeF")) { // namespaced Qt?
        std::wostringstream str;
        str << '(' << v["wd"].value() << ", " << v["ht"].value() << ')';
        *s = str.str();
        return SymbolGroupNode::DumperOk;
    }
    if (endsWith(type, "QPoint") || endsWith(type, "QPointF")) { // namespaced Qt?
        std::wostringstream str;
        str << '(' << v["xp"].value() << ", " << v["yp"].value() << ')';
        *s = str.str();
        return SymbolGroupNode::DumperOk;
    }
    if (endsWith(type, "QLine") || endsWith(type, "QLineF")) { // namespaced Qt?
        const SymbolGroupValue p1 = v["pt1"];
        const SymbolGroupValue p2 = v["pt2"];
        if (p1 && p2) {
            std::wostringstream str;
            str << '(' << p1["xp"].value() << ", " << p1["yp"].value() << ") ("
                 << p2["xp"].value() << ", " << p2["yp"].value() << ')';
            *s = str.str();
            return SymbolGroupNode::DumperOk;
        }
        return SymbolGroupNode::DumperFailed;
    }
    if (endsWith(type, "QRect")) {
        std::wostringstream str;
        dumpRectPoints(str, v["x1"].intValue(), v["y1"].intValue(), v["x2"].intValue(), v["y2"].intValue());
        *s = str.str();
        return SymbolGroupNode::DumperOk;
    }
    if (endsWith(type, "QRectF")) {
        std::wostringstream str;
        dumpRect(str, v["xp"].floatValue(), v["yp"].floatValue(), v["w"].floatValue(), v["h"].floatValue());
        *s = str.str();
        return SymbolGroupNode::DumperOk;
    }
    return SymbolGroupNode::DumperNotApplicable;
}

// Dump a std::string.
static unsigned dumpStdString(const std::string &type, const SymbolGroupValue &v, std::wstring *s)
{
    if (type != stdStringTypeC && type != stdWStringTypeC)
        return SymbolGroupNode::DumperNotApplicable;
    // MSVC 2010: Access Bx/_Buf in base class
    SymbolGroupValue buf = v[unsigned(0)]["_Bx"]["_Buf"];
    if (!buf) // MSVC2008: Bx/Buf are members
        buf = v["_Bx"]["_Buf"];
    if (buf) {
        *s = buf.value();
        return SymbolGroupNode::DumperOk;
    }
    return SymbolGroupNode::DumperFailed;
}

// Dump builtin simple types using SymbolGroupValue expressions.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx, std::wstring *s)
{
    // Check for class types and strip pointer types (references appear as pointers as well)
    std::string type = n->type();
    if (type.compare(0, 6, "class ") != 0)
        return SymbolGroupNode::DumperNotApplicable;
    if (endsWith(type, " *"))
        type.erase(type.size() - 2, 2);
    const SymbolGroupValue v(n, ctx);
    unsigned rc = dumpQString(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpQByteArray(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpQtCoreTypes(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpStdString(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpQtGeometryTypes(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpQColor(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    return rc;
}
