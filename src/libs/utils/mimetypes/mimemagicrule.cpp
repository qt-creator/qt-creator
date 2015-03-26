/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "mimemagicrule_p.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QDebug>
#include <qendian.h>

using namespace Utils;
using namespace Utils::Internal;

// in the same order as Type!
static const char magicRuleTypes_string[] =
    "invalid\0"
    "string\0"
    "regexp\0"
    "host16\0"
    "host32\0"
    "big16\0"
    "big32\0"
    "little16\0"
    "little32\0"
    "byte\0"
    "\0";

static const int magicRuleTypes_indices[] = {
    0, 8, 15, 22, 29, 36, 42, 48, 57, 66, 71, 0
};

MimeMagicRule::Type MimeMagicRule::type(const QByteArray &theTypeName)
{
    for (int i = String; i <= Byte; ++i) {
        if (theTypeName == magicRuleTypes_string + magicRuleTypes_indices[i])
            return Type(i);
    }
    return Invalid;
}

QByteArray MimeMagicRule::typeName(MimeMagicRule::Type theType)
{
    return magicRuleTypes_string + magicRuleTypes_indices[theType];
}

namespace Utils {
namespace Internal {

class MimeMagicRulePrivate
{
public:
    bool operator==(const MimeMagicRulePrivate &other) const;

    MimeMagicRule::Type type;
    QByteArray value;
    int startPos;
    int endPos;
    QByteArray mask;

    QRegularExpression regexp;
    QByteArray pattern;
    quint32 number;
    quint32 numberMask;

    typedef bool (*MatchFunction)(const MimeMagicRulePrivate *d, const QByteArray &data);
    MatchFunction matchFunction;
};

bool MimeMagicRulePrivate::operator==(const MimeMagicRulePrivate &other) const
{
    return type == other.type &&
           value == other.value &&
           startPos == other.startPos &&
           endPos == other.endPos &&
           mask == other.mask &&
           pattern == other.pattern &&
           number == other.number &&
           numberMask == other.numberMask &&
           matchFunction == other.matchFunction;
}

} // Internal
} // Utils

// Used by both providers
bool MimeMagicRule::matchSubstring(const char *dataPtr, int dataSize, int rangeStart, int rangeLength,
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

static bool matchString(const MimeMagicRulePrivate *d, const QByteArray &data)
{
    const int rangeLength = d->endPos - d->startPos + 1;
    return MimeMagicRule::matchSubstring(data.constData(), data.size(), d->startPos, rangeLength, d->pattern.size(), d->pattern.constData(), d->mask.constData());
}

static bool matchRegExp(const MimeMagicRulePrivate *d, const QByteArray &data)
{
    const QString str = QString::fromUtf8(data);
    int length = d->endPos;
    if (length == d->startPos)
        length = -1; // from startPos to end of string
    const QString subStr = str.left(length);
    return d->regexp.match(subStr, d->startPos).hasMatch();
}

template <typename T>
static bool matchNumber(const MimeMagicRulePrivate *d, const QByteArray &data)
{
    const T value(d->number);
    const T mask(d->numberMask);

    //qDebug() << "matchNumber" << "0x" << QString::number(d->number, 16) << "size" << sizeof(T);
    //qDebug() << "mask" << QString::number(d->numberMask, 16);

    const char *p = data.constData() + d->startPos;
    const char *e = data.constData() + qMin(data.size() - int(sizeof(T)), d->endPos + 1);
    for ( ; p <= e; ++p) {
        if ((*reinterpret_cast<const T*>(p) & mask) == (value & mask))
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

MimeMagicRule::MimeMagicRule(MimeMagicRule::Type theType,
                             const QByteArray &theValue,
                             int theStartPos,
                             int theEndPos,
                             const QByteArray &theMask,
                             QString *errorString) :
    d(new MimeMagicRulePrivate)
{
    d->type = theType;
    d->value = theValue;
    d->startPos = theStartPos;
    d->endPos = theEndPos;
    d->mask = theMask;
    d->matchFunction = 0;

    if (d->value.isEmpty()) {
        d->type = Invalid;
        if (errorString)
            *errorString = QLatin1String("Invalid empty magic rule value");
        return;
    }

    if (d->type >= Host16 && d->type <= Byte) {
        bool ok;
        d->number = d->value.toUInt(&ok, 0); // autodetect
        if (!ok) {
            d->type = Invalid;
            if (errorString)
                *errorString = QString::fromLatin1("Invalid magic rule value \"%1\"").arg(
                        QString::fromLatin1(d->value));
            return;
        }
        d->numberMask = !d->mask.isEmpty() ? d->mask.toUInt(&ok, 0) : 0; // autodetect
    }

    switch (d->type) {
    case String:
        d->pattern = makePattern(d->value);
        d->pattern.squeeze();
        if (!d->mask.isEmpty()) {
            if (d->mask.size() < 4 || !d->mask.startsWith("0x")) {
                d->type = Invalid;
                if (errorString)
                    *errorString = QString::fromLatin1("Invalid magic rule mask \"%1\"").arg(
                            QString::fromLatin1(d->mask));
                return;
            }
            const QByteArray &tempMask = QByteArray::fromHex(QByteArray::fromRawData(
                                                     d->mask.constData() + 2, d->mask.size() - 2));
            if (tempMask.size() != d->pattern.size()) {
                d->type = Invalid;
                if (errorString)
                    *errorString = QString::fromLatin1("Invalid magic rule mask size \"%1\"").arg(
                            QString::fromLatin1(d->mask));
                return;
            }
            d->mask = tempMask;
        } else {
            d->mask.fill(char(-1), d->pattern.size());
        }
        d->mask.squeeze();
        d->matchFunction = matchString;
        break;
    case RegExp:
        d->regexp.setPatternOptions(QRegularExpression::MultilineOption
                                    | QRegularExpression::DotMatchesEverythingOption
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
                                    | QRegularExpression::OptimizeOnFirstUsageOption
#endif
                                    );
        d->regexp.setPattern(QString::fromUtf8(d->value));
        if (!d->regexp.isValid()) {
            d->type = Invalid;
            if (errorString)
                *errorString = QString::fromLatin1("Invalid magic rule regexp value \"%1\"").arg(
                        QString::fromLatin1(d->value));
            return;
        }
        d->matchFunction = matchRegExp;
        break;
    case Byte:
        if (d->number <= quint8(-1)) {
            if (d->numberMask == 0)
                d->numberMask = quint8(-1);
            d->matchFunction = matchNumber<quint8>;
        }
        break;
    case Big16:
    case Host16:
    case Little16:
        if (d->number <= quint16(-1)) {
            d->number = d->type == Little16 ? qFromLittleEndian<quint16>(d->number) : qFromBigEndian<quint16>(d->number);
            if (d->numberMask == 0)
                d->numberMask = quint16(-1);
            d->matchFunction = matchNumber<quint16>;
        }
        break;
    case Big32:
    case Host32:
    case Little32:
        if (d->number <= quint32(-1)) {
            d->number = d->type == Little32 ? qFromLittleEndian<quint32>(d->number) : qFromBigEndian<quint32>(d->number);
            if (d->numberMask == 0)
                d->numberMask = quint32(-1);
            d->matchFunction = matchNumber<quint32>;
        }
        break;
    default:
        break;
    }
}

MimeMagicRule::MimeMagicRule(const MimeMagicRule &other) :
    d(new MimeMagicRulePrivate(*other.d))
{
}

MimeMagicRule::~MimeMagicRule()
{
}

MimeMagicRule &MimeMagicRule::operator=(const MimeMagicRule &other)
{
    *d = *other.d;
    return *this;
}

bool MimeMagicRule::operator==(const MimeMagicRule &other) const
{
    return d == other.d ||
           *d == *other.d;
}

MimeMagicRule::Type MimeMagicRule::type() const
{
    return d->type;
}

QByteArray MimeMagicRule::value() const
{
    return d->value;
}

int MimeMagicRule::startPos() const
{
    return d->startPos;
}

int MimeMagicRule::endPos() const
{
    return d->endPos;
}

QByteArray MimeMagicRule::mask() const
{
    QByteArray result = d->mask;
    if (d->type == String) {
        // restore '0x'
        result = "0x" + result.toHex();
    }
    return result;
}

bool MimeMagicRule::isValid() const
{
    return d->matchFunction;
}

bool MimeMagicRule::matches(const QByteArray &data) const
{
    const bool ok = d->matchFunction && d->matchFunction(d.data(), data);
    if (!ok)
        return false;

    // No submatch? Then we are done.
    if (m_subMatches.isEmpty())
        return true;

    //qDebug() << "Checking" << m_subMatches.count() << "sub-rules";
    // Check that one of the submatches matches too
    for ( QList<MimeMagicRule>::const_iterator it = m_subMatches.begin(), end = m_subMatches.end() ;
          it != end ; ++it ) {
        if ((*it).matches(data)) {
            // One of the hierarchies matched -> mimetype recognized.
            return true;
        }
    }
    return false;


}
