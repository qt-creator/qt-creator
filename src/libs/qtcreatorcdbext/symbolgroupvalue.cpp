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
        if (integerFromString(wStringToString(value()), &rc))
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
    rc = dumpStdString(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    rc = dumpQtGeometryTypes(type, v, s);
    if (rc != SymbolGroupNode::DumperNotApplicable)
        return rc;
    return rc;
}
