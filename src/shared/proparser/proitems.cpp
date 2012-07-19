/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "proitems.h"

#include <QFileInfo>
#include <QSet>
#include <QStringList>

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;

// from qhash.cpp
uint ProString::hash(const QChar *p, int n)
{
    uint h = 0;

    while (n--) {
        h = (h << 4) + (*p++).unicode();
        h ^= (h & 0xf0000000) >> 23;
        h &= 0x0fffffff;
    }
    return h;
}

ProString::ProString() :
    m_offset(0), m_length(0), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(const ProString &other) :
    m_string(other.m_string), m_offset(other.m_offset), m_length(other.m_length), m_file(other.m_file), m_hash(other.m_hash)
{
}

ProString::ProString(const ProString &other, OmitPreHashing) :
    m_string(other.m_string), m_offset(other.m_offset), m_length(other.m_length), m_file(other.m_file), m_hash(0x80000000)
{
}

ProString::ProString(const QString &str) :
    m_string(str), m_offset(0), m_length(str.length()), m_file(0)
{
    updatedHash();
}

ProString::ProString(const QString &str, OmitPreHashing) :
    m_string(str), m_offset(0), m_length(str.length()), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(const char *str) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str)), m_file(0)
{
    updatedHash();
}

ProString::ProString(const char *str, OmitPreHashing) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str)), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(const QString &str, int offset, int length) :
    m_string(str), m_offset(offset), m_length(length), m_file(0)
{
    updatedHash();
}

ProString::ProString(const QString &str, int offset, int length, uint hash) :
    m_string(str), m_offset(offset), m_length(length), m_file(0), m_hash(hash)
{
}

ProString::ProString(const QString &str, int offset, int length, ProStringConstants::OmitPreHashing) :
    m_string(str), m_offset(offset), m_length(length), m_file(0), m_hash(0x80000000)
{
}

void ProString::setValue(const QString &str)
{
    m_string = str, m_offset = 0, m_length = str.length();
    updatedHash();
}

void ProString::setValue(const QString &str, OmitPreHashing)
{
    m_string = str, m_offset = 0, m_length = str.length(), m_hash = 0x80000000;
}

uint ProString::updatedHash() const
{
     return (m_hash = hash(m_string.constData() + m_offset, m_length));
}

uint qHash(const ProString &str)
{
    if (!(str.m_hash & 0x80000000))
        return str.m_hash;
    return str.updatedHash();
}

QString ProString::toQString() const
{
    return m_string.mid(m_offset, m_length);
}

QString &ProString::toQString(QString &tmp) const
{
    return tmp.setRawData(m_string.constData() + m_offset, m_length);
}

bool ProString::operator==(const ProString &other) const
{
    if (m_length != other.m_length)
        return false;
    return !memcmp(m_string.constData() + m_offset,
                   other.m_string.constData() + other.m_offset, m_length * 2);
}

bool ProString::operator==(const QString &other) const
{
    if (m_length != other.length())
        return false;
    return !memcmp(m_string.constData() + m_offset, other.constData(), m_length * 2);
}

bool ProString::operator==(const QLatin1String &other) const
{
    const ushort *uc = (ushort *)m_string.constData() + m_offset;
    const ushort *e = uc + m_length;
    const uchar *c = (uchar *)other.latin1();

    if (!c)
        return isEmpty();

    while (*c) {
        if (uc == e || *uc != *c)
            return false;
        ++uc;
        ++c;
    }
    return (uc == e);
}

QChar *ProString::prepareAppend(int extraLen)
{
    if (m_string.isDetached() && m_length + extraLen <= m_string.capacity()) {
        m_string.reserve(0); // Prevent the resize() below from reallocating
        QChar *ptr = (QChar *)m_string.constData();
        if (m_offset)
            memmove(ptr, ptr + m_offset, m_length * 2);
        ptr += m_length;
        m_offset = 0;
        m_length += extraLen;
        m_string.resize(m_length);
        m_hash = 0x80000000;
        return ptr;
    } else {
        QString neu(m_length + extraLen, Qt::Uninitialized);
        QChar *ptr = (QChar *)neu.constData();
        memcpy(ptr, m_string.constData() + m_offset, m_length * 2);
        ptr += m_length;
        *this = ProString(neu, NoHash);
        return ptr;
    }
}

// If pending != 0, prefix with space if appending to non-empty non-pending
ProString &ProString::append(const ProString &other, bool *pending)
{
    if (other.m_length) {
        if (!m_length) {
            *this = other;
        } else {
            QChar *ptr;
            if (pending && !*pending) {
                ptr = prepareAppend(1 + other.m_length);
                *ptr++ = 32;
            } else {
                ptr = prepareAppend(other.m_length);
            }
            memcpy(ptr, other.m_string.constData() + other.m_offset, other.m_length * 2);
            if (other.m_file)
                m_file = other.m_file;
        }
        if (pending)
            *pending = true;
    }
    return *this;
}

ProString &ProString::append(const ProStringList &other, bool *pending, bool skipEmpty1st)
{
    if (const int sz = other.size()) {
        int startIdx = 0;
        if (pending && !*pending && skipEmpty1st && other.at(0).isEmpty()) {
            if (sz == 1)
                return *this;
            startIdx = 1;
        }
        if (!m_length && sz == startIdx + 1) {
            *this = other.at(startIdx);
        } else {
            int totalLength = sz - startIdx;
            for (int i = startIdx; i < sz; ++i)
                totalLength += other.at(i).size();
            bool putSpace = false;
            if (pending && !*pending && m_length)
                putSpace = true;
            else
                totalLength--;

            QChar *ptr = prepareAppend(totalLength);
            for (int i = startIdx; i < sz; ++i) {
                if (putSpace)
                    *ptr++ = 32;
                else
                    putSpace = true;
                const ProString &str = other.at(i);
                memcpy(ptr, str.m_string.constData() + str.m_offset, str.m_length * 2);
                ptr += str.m_length;
            }
            if (other.last().m_file)
                m_file = other.last().m_file;
        }
        if (pending)
            *pending = true;
    }
    return *this;
}

QString operator+(const ProString &one, const ProString &two)
{
    if (two.m_length) {
        if (!one.m_length) {
            return two.toQString();
        } else {
            QString neu(one.m_length + two.m_length, Qt::Uninitialized);
            ushort *ptr = (ushort *)neu.constData();
            memcpy(ptr, one.m_string.constData() + one.m_offset, one.m_length * 2);
            memcpy(ptr + one.m_length, two.m_string.constData() + two.m_offset, two.m_length * 2);
            return neu;
        }
    }
    return one.toQString();
}


ProString ProString::mid(int off, int len) const
{
    ProString ret(*this, NoHash);
    if (off > m_length)
        off = m_length;
    ret.m_offset += off;
    ret.m_length -= off;
    if (ret.m_length > len)
        ret.m_length = len;
    return ret;
}

ProString ProString::trimmed() const
{
    ProString ret(*this, NoHash);
    int cur = m_offset;
    int end = cur + m_length;
    const QChar *data = m_string.constData();
    for (; cur < end; cur++)
        if (!data[cur].isSpace()) {
            // No underrun check - we know there is at least one non-whitespace
            while (data[end - 1].isSpace())
                end--;
            break;
        }
    ret.m_offset = cur;
    ret.m_length = end - cur;
    return ret;
}

QString ProStringList::join(const QString &sep) const
{
    int totalLength = 0;
    const int sz = size();

    for (int i = 0; i < sz; ++i)
        totalLength += at(i).size();

    if (sz)
        totalLength += sep.size() * (sz - 1);

    QString res(totalLength, Qt::Uninitialized);
    QChar *ptr = (QChar *)res.constData();
    for (int i = 0; i < sz; ++i) {
        if (i) {
            memcpy(ptr, sep.constData(), sep.size() * 2);
            ptr += sep.size();
        }
        memcpy(ptr, at(i).constData(), at(i).size() * 2);
        ptr += at(i).size();
    }
    return res;
}

void ProStringList::removeDuplicates()
{
    int n = size();
    int j = 0;
    QSet<ProString> seen;
    seen.reserve(n);
    for (int i = 0; i < n; ++i) {
        const ProString &s = at(i);
        if (seen.contains(s))
            continue;
        seen.insert(s);
        if (j != i)
            (*this)[j] = s;
        ++j;
    }
    if (n != j)
        erase(begin() + j, end());
}

QStringList ProStringList::toQStringList() const
{
    QStringList ret;
    ret.reserve(size());
    foreach (const ProString &str, *this)
        ret << str.toQString();
    return ret;
}

ProFile::ProFile(const QString &fileName)
    : m_refCount(1),
      m_fileName(fileName),
      m_ok(true)
{
    if (!fileName.startsWith(QLatin1Char('(')))
        m_directoryName = QFileInfo( // qmake sickness: canonicalize only the directory!
                fileName.left(fileName.lastIndexOf(QLatin1Char('/')))).canonicalFilePath();
}

ProFile::~ProFile()
{
}

QT_END_NAMESPACE
