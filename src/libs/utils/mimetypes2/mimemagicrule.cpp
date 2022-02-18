/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#define QT_NO_CAST_FROM_ASCII

#include "qmimemagicrule_p.h"

#include "qmimetypeparser_p.h"
#include <QtCore/QList>
#include <QtCore/QDebug>
#include <qendian.h>

QT_BEGIN_NAMESPACE

// in the same order as Type!
static const char magicRuleTypes_string[] =
    "invalid\0"
    "string\0"
    "host16\0"
    "host32\0"
    "big16\0"
    "big32\0"
    "little16\0"
    "little32\0"
    "byte\0"
    "\0";

static const int magicRuleTypes_indices[] = {
    0, 8, 15, 22, 29, 35, 41, 50, 59, 64, 0
};

QMimeMagicRule::Type QMimeMagicRule::type(const QByteArray &theTypeName)
{
    for (int i = String; i <= Byte; ++i) {
        if (theTypeName == magicRuleTypes_string + magicRuleTypes_indices[i])
            return Type(i);
    }
    return Invalid;
}

QByteArray QMimeMagicRule::typeName(QMimeMagicRule::Type theType)
{
    return magicRuleTypes_string + magicRuleTypes_indices[theType];
}

bool QMimeMagicRule::operator==(const QMimeMagicRule &other) const
{
    return m_type == other.m_type &&
           m_value == other.m_value &&
           m_startPos == other.m_startPos &&
           m_endPos == other.m_endPos &&
           m_mask == other.m_mask &&
           m_pattern == other.m_pattern &&
           m_number == other.m_number &&
           m_numberMask == other.m_numberMask &&
           m_matchFunction == other.m_matchFunction;
}

// Used by both providers
bool QMimeMagicRule::matchSubstring(const char *dataPtr, int dataSize, int rangeStart, int rangeLength,
                                    int valueLength, const char *valueData, const char *mask)
{
    // Size of searched data.
    // Example: value="ABC", rangeLength=3 -> we need 3+3-1=5 bytes (ABCxx,xABCx,xxABC would match)
    const int dataNeeded = qMin(rangeLength + valueLength - 1, dataSize - rangeStart);

    if (!mask) {
        // callgrind says QByteArray::indexOf is much slower, since our strings are typically too
        // short for be worth Boyer-Moore matching (1 to 71 bytes, 11 bytes on average).
        bool found = false;
        for (int i = rangeStart; i < rangeStart + rangeLength; ++i) {
            if (i + valueLength > dataSize)
                break;

            if (memcmp(valueData, dataPtr + i, valueLength) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    } else {
        bool found = false;
        const char *readDataBase = dataPtr + rangeStart;
        // Example (continued from above):
        // deviceSize is 4, so dataNeeded was max'ed to 4.
        // maxStartPos = 4 - 3 + 1 = 2, and indeed
        // we need to check for a match a positions 0 and 1 (ABCx and xABC).
        const int maxStartPos = dataNeeded - valueLength + 1;
        for (int i = 0; i < maxStartPos; ++i) {
            const char *d = readDataBase + i;
            bool valid = true;
            for (int idx = 0; idx < valueLength; ++idx) {
                if (((*d++) & mask[idx]) != (valueData[idx] & mask[idx])) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                found = true;
        }
        if (!found)
            return false;
    }
    //qDebug() << "Found" << value << "in" << searchedData;
    return true;
}

bool QMimeMagicRule::matchString(const QByteArray &data) const
{
    const int rangeLength = m_endPos - m_startPos + 1;
    return QMimeMagicRule::matchSubstring(data.constData(), data.size(), m_startPos, rangeLength, m_pattern.size(), m_pattern.constData(), m_mask.constData());
}

template <typename T>
bool QMimeMagicRule::matchNumber(const QByteArray &data) const
{
    const T value(m_number);
    const T mask(m_numberMask);

    //qDebug() << "matchNumber" << "0x" << QString::number(m_number, 16) << "size" << sizeof(T);
    //qDebug() << "mask" << QString::number(m_numberMask, 16);

    const char *p = data.constData() + m_startPos;
    const char *e = data.constData() + qMin(data.size() - int(sizeof(T)), m_endPos);
    for ( ; p <= e; ++p) {
        if ((qFromUnaligned<T>(p) & mask) == (value & mask))
            return true;
    }

    return false;
}

static inline QByteArray makePattern(const QByteArray &value)
{
    QByteArray pattern(value.size(), Qt::Uninitialized);
    char *data = pattern.data();

    const char *p = value.constData();
    const char *e = p + value.size();
    for ( ; p < e; ++p) {
        if (*p == '\\' && ++p < e) {
            if (*p == 'x') { // hex (\\xff)
                char c = 0;
                for (int i = 0; i < 2 && p + 1 < e; ++i) {
                    ++p;
                    if (*p >= '0' && *p <= '9')
                        c = (c << 4) + *p - '0';
                    else if (*p >= 'a' && *p <= 'f')
                        c = (c << 4) + *p - 'a' + 10;
                    else if (*p >= 'A' && *p <= 'F')
                        c = (c << 4) + *p - 'A' + 10;
                    else
                        continue;
                }
                *data++ = c;
            } else if (*p >= '0' && *p <= '7') { // oct (\\7, or \\77, or \\377)
                char c = *p - '0';
                if (p + 1 < e && p[1] >= '0' && p[1] <= '7') {
                    c = (c << 3) + *(++p) - '0';
                    if (p + 1 < e && p[1] >= '0' && p[1] <= '7' && p[-1] <= '3')
                        c = (c << 3) + *(++p) - '0';
                }
                *data++ = c;
            } else if (*p == 'n') {
                *data++ = '\n';
            } else if (*p == 'r') {
                *data++ = '\r';
            } else if (*p == 't') {
                *data++ = '\t';
            } else { // escaped
                *data++ = *p;
            }
        } else {
            *data++ = *p;
        }
    }
    pattern.truncate(data - pattern.data());

    return pattern;
}

// Evaluate a magic match rule like
//  <match value="must be converted with BinHex" type="string" offset="11"/>
//  <match value="0x9501" type="big16" offset="0:64"/>

QMimeMagicRule::QMimeMagicRule(const QString &type,
                               const QByteArray &value,
                               const QString &offsets,
                               const QByteArray &mask,
                               QString *errorString)
    : m_type(QMimeMagicRule::type(type.toLatin1())),
      m_value(value),
      m_mask(mask),
      m_matchFunction(nullptr)
{
    if (Q_UNLIKELY(m_type == Invalid))
        *errorString = QLatin1String("Type ") + type + QLatin1String(" is not supported");

    // Parse for offset as "1" or "1:10"
    const int colonIndex = offsets.indexOf(QLatin1Char(':'));
    const QStringView startPosStr = QStringView{offsets}.mid(0, colonIndex); // \ These decay to returning 'offsets'
    const QStringView endPosStr   = QStringView{offsets}.mid(colonIndex + 1);// / unchanged when colonIndex == -1
    if (Q_UNLIKELY(!QMimeTypeParserBase::parseNumber(startPosStr, &m_startPos, errorString)) ||
        Q_UNLIKELY(!QMimeTypeParserBase::parseNumber(endPosStr, &m_endPos, errorString))) {
        m_type = Invalid;
        return;
    }

    if (Q_UNLIKELY(m_value.isEmpty())) {
        m_type = Invalid;
        if (errorString)
            *errorString = QStringLiteral("Invalid empty magic rule value");
        return;
    }

    if (m_type >= Host16 && m_type <= Byte) {
        bool ok;
        m_number = m_value.toUInt(&ok, 0); // autodetect base
        if (Q_UNLIKELY(!ok)) {
            m_type = Invalid;
            if (errorString)
                *errorString = QLatin1String("Invalid magic rule value \"") + QLatin1String(m_value) + QLatin1Char('"');
            return;
        }
        m_numberMask = !m_mask.isEmpty() ? m_mask.toUInt(&ok, 0) : 0; // autodetect base
    }

    switch (m_type) {
    case String:
        m_pattern = makePattern(m_value);
        m_pattern.squeeze();
        if (!m_mask.isEmpty()) {
            if (Q_UNLIKELY(m_mask.size() < 4 || !m_mask.startsWith("0x"))) {
                m_type = Invalid;
                if (errorString)
                    *errorString = QLatin1String("Invalid magic rule mask \"") + QLatin1String(m_mask) + QLatin1Char('"');
                return;
            }
            const QByteArray &tempMask = QByteArray::fromHex(QByteArray::fromRawData(
                                                     m_mask.constData() + 2, m_mask.size() - 2));
            if (Q_UNLIKELY(tempMask.size() != m_pattern.size())) {
                m_type = Invalid;
                if (errorString)
                    *errorString = QLatin1String("Invalid magic rule mask size \"") + QLatin1String(m_mask) + QLatin1Char('"');
                return;
            }
            m_mask = tempMask;
        } else {
            m_mask.fill(char(-1), m_pattern.size());
        }
        m_mask.squeeze();
        m_matchFunction = &QMimeMagicRule::matchString;
        break;
    case Byte:
        if (m_number <= quint8(-1)) {
            if (m_numberMask == 0)
                m_numberMask = quint8(-1);
            m_matchFunction = &QMimeMagicRule::matchNumber<quint8>;
        }
        break;
    case Big16:
    case Little16:
        if (m_number <= quint16(-1)) {
            m_number = m_type == Little16 ? qFromLittleEndian<quint16>(m_number) : qFromBigEndian<quint16>(m_number);
            if (m_numberMask != 0)
                m_numberMask = m_type == Little16 ? qFromLittleEndian<quint16>(m_numberMask) : qFromBigEndian<quint16>(m_numberMask);
        }
        Q_FALLTHROUGH();
    case Host16:
        if (m_number <= quint16(-1)) {
            if (m_numberMask == 0)
                m_numberMask = quint16(-1);
            m_matchFunction = &QMimeMagicRule::matchNumber<quint16>;
        }
        break;
    case Big32:
    case Little32:
        m_number = m_type == Little32 ? qFromLittleEndian<quint32>(m_number) : qFromBigEndian<quint32>(m_number);
        if (m_numberMask != 0)
            m_numberMask = m_type == Little32 ? qFromLittleEndian<quint32>(m_numberMask) : qFromBigEndian<quint32>(m_numberMask);
        Q_FALLTHROUGH();
    case Host32:
        if (m_numberMask == 0)
            m_numberMask = quint32(-1);
        m_matchFunction = &QMimeMagicRule::matchNumber<quint32>;
        break;
    default:
        break;
    }
}

QByteArray QMimeMagicRule::mask() const
{
    QByteArray result = m_mask;
    if (m_type == String) {
        // restore '0x'
        result = "0x" + result.toHex();
    }
    return result;
}

bool QMimeMagicRule::matches(const QByteArray &data) const
{
    const bool ok = m_matchFunction && (this->*m_matchFunction)(data);
    if (!ok)
        return false;

    // No submatch? Then we are done.
    if (m_subMatches.isEmpty())
        return true;

    //qDebug() << "Checking" << m_subMatches.count() << "sub-rules";
    // Check that one of the submatches matches too
    for ( QList<QMimeMagicRule>::const_iterator it = m_subMatches.begin(), end = m_subMatches.end() ;
          it != end ; ++it ) {
        if ((*it).matches(data)) {
            // One of the hierarchies matched -> mimetype recognized.
            return true;
        }
    }
    return false;


}

QT_END_NAMESPACE
