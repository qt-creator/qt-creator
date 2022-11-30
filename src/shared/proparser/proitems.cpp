// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "proitems.h"

#include <qdebug.h>
#include <qfileinfo.h>
#include <qset.h>
#include <qstringlist.h>
#include <qtextstream.h>

QT_BEGIN_NAMESPACE

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

ProString::ProString(const QString &str, DoPreHashing) :
    m_string(str), m_offset(0), m_length(str.length()), m_file(0)
{
    updatedHash();
}

ProString::ProString(const QString &str) :
    m_string(str), m_offset(0), m_length(str.length()), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(QStringView str) :
    m_string(str.toString()), m_offset(0), m_length(str.size()), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(const char *str, DoPreHashing) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(int(qstrlen(str))), m_file(0)
{
    updatedHash();
}

ProString::ProString(const char *str) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(int(qstrlen(str))), m_file(0), m_hash(0x80000000)
{
}

ProString::ProString(const QString &str, int offset, int length, DoPreHashing) :
    m_string(str), m_offset(offset), m_length(length), m_file(0)
{
    updatedHash();
}

ProString::ProString(const QString &str, int offset, int length, uint hash) :
    m_string(str), m_offset(offset), m_length(length), m_file(0), m_hash(hash)
{
}

ProString::ProString(const QString &str, int offset, int length) :
    m_string(str), m_offset(offset), m_length(length), m_file(0), m_hash(0x80000000)
{
}

void ProString::setValue(const QString &str)
{
    m_string = str, m_offset = 0, m_length = str.length(), m_hash = 0x80000000;
}

uint ProString::updatedHash() const
{
     return (m_hash = hash(m_string.constData() + m_offset, m_length));
}

size_t qHash(const ProString &str)
{
    if (!(str.m_hash & 0x80000000))
        return str.m_hash;
    return str.updatedHash();
}

ProKey::ProKey(const QString &str) :
    ProString(str, DoHash)
{
}

ProKey::ProKey(const char *str) :
    ProString(str, DoHash)
{
}

ProKey::ProKey(const QString &str, int off, int len) :
    ProString(str, off, len, DoHash)
{
}

ProKey::ProKey(const QString &str, int off, int len, uint hash) :
    ProString(str, off, len, hash)
{
}

void ProKey::setValue(const QString &str)
{
    m_string = str, m_offset = 0, m_length = str.length();
    updatedHash();
}

QString ProString::toQString() const
{
    return m_string.mid(m_offset, m_length);
}

QString &ProString::toQString(QString &tmp) const
{
    tmp = m_string.mid(m_offset, m_length);
    return tmp;
}

ProString &ProString::prepend(const ProString &other)
{
    if (other.m_length) {
        if (!m_length) {
            *this = other;
        } else {
            m_string = other.toStringView() + toStringView();
            m_offset = 0;
            m_length = m_string.length();
            if (!m_file)
                m_file = other.m_file;
            m_hash = 0x80000000;
        }
    }
    return *this;
}

ProString &ProString::append(const QLatin1String other)
{
    if (other.size()) {
        if (m_length != m_string.length()) {
            m_string = toStringView() + other;
            m_offset = 0;
            m_length = m_string.length();
        } else {
            Q_ASSERT(m_offset == 0);
            m_string.append(other);
            m_length += other.size();
        }
        m_hash = 0x80000000;
    }
    return *this;
}

ProString &ProString::append(QChar other)
{
    if (m_length != m_string.length()) {
        m_string = toStringView() + other;
        m_offset = 0;
        m_length = m_string.length();
    } else {
        Q_ASSERT(m_offset == 0);
        m_string.append(other);
        ++m_length;
    }
    m_hash = 0x80000000;
    return *this;
}

// If pending != 0, prefix with space if appending to non-empty non-pending
ProString &ProString::append(const ProString &other, bool *pending)
{
    if (other.m_length) {
        if (!m_length) {
            *this = other;
        } else {
            if (m_length != m_string.length())
                m_string = toQString();
            if (pending && !*pending) {
                m_string += QLatin1Char(' ') + other.toStringView();
            } else {
                m_string += other.toStringView();
            }
            m_length = m_string.length();
            m_offset = 0;
            if (other.m_file)
                m_file = other.m_file;
            m_hash = 0x80000000;
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

            m_string = toQString();
            m_offset = 0;
            for (int i = startIdx; i < sz; ++i) {
                if (putSpace)
                    m_string += QLatin1Char(' ');
                else
                    putSpace = true;
                const ProString &str = other.at(i);
                m_string += str.toStringView();
            }
            m_length = m_string.length();
            if (other.last().m_file)
                m_file = other.last().m_file;
            m_hash = 0x80000000;
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
    if ((uint)ret.m_length > (uint)len)  // Unsigned comparison to interpret < 0 as infinite
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

QTextStream &operator<<(QTextStream &t, const ProString &str)
{
    t << str.toStringView();
    return t;
}

static QString ProStringList_join(const ProStringList &this_, const QChar *sep, const int sepSize)
{
    int totalLength = 0;
    const int sz = this_.size();

    for (int i = 0; i < sz; ++i)
        totalLength += this_.at(i).size();

    if (sz)
        totalLength += sepSize * (sz - 1);

    QString res(totalLength, Qt::Uninitialized);
    QChar *ptr = (QChar *)res.constData();
    for (int i = 0; i < sz; ++i) {
        if (i) {
            memcpy(ptr, sep, sepSize * sizeof(QChar));
            ptr += sepSize;
        }
        const ProString &str = this_.at(i);
        memcpy(ptr, str.constData(), str.size() * sizeof(QChar));
        ptr += str.size();
    }
    return res;
}

QString ProStringList::join(const ProString &sep) const
{
    return ProStringList_join(*this, sep.constData(), sep.size());
}

QString ProStringList::join(const QString &sep) const
{
    return ProStringList_join(*this, sep.constData(), sep.size());
}

QString ProStringList::join(QChar sep) const
{
    return ProStringList_join(*this, &sep, 1);
}

void ProStringList::removeAll(const ProString &str)
{
    for (int i = size(); --i >= 0; )
        if (at(i) == str)
            remove(i);
}

void ProStringList::removeAll(const char *str)
{
    for (int i = size(); --i >= 0; )
        if (at(i) == str)
            remove(i);
}

void ProStringList::removeEach(const ProStringList &value)
{
    for (const ProString &str : value) {
        if (isEmpty())
            break;
        if (!str.isEmpty())
            removeAll(str);
    }
}

void ProStringList::removeEmpty()
{
    for (int i = size(); --i >= 0;)
        if (at(i).isEmpty())
            remove(i);
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

void ProStringList::insertUnique(const ProStringList &value)
{
    for (const ProString &str : value)
        if (!str.isEmpty() && !contains(str))
            append(str);
}

ProStringList::ProStringList(const QStringList &list)
{
    reserve(list.size());
    for (const QString &str : list)
        *this << ProString(str);
}

QStringList ProStringList::toQStringList() const
{
    QStringList ret;
    ret.reserve(size());
    for (const auto &e : *this)
        ret.append(e.toQString());
    return ret;
}

bool ProStringList::contains(const ProString &str, Qt::CaseSensitivity cs) const
{
    for (int i = 0; i < size(); i++)
        if (!at(i).compare(str, cs))
            return true;
    return false;
}

bool ProStringList::contains(QStringView str, Qt::CaseSensitivity cs) const
{
    for (int i = 0; i < size(); i++)
        if (!at(i).toStringView().compare(str, cs))
            return true;
    return false;
}

bool ProStringList::contains(const char *str, Qt::CaseSensitivity cs) const
{
    for (int i = 0; i < size(); i++)
        if (!at(i).compare(str, cs))
            return true;
    return false;
}

ProFile::ProFile(const QString &device, int id, const QString &fileName)
    : m_refCount(1),
      m_fileName(fileName),
      m_device(device),
      m_id(id),
      m_ok(true),
      m_hostBuild(false)
{
    if (!fileName.startsWith(QLatin1Char('('))) {
        m_directoryName = fileName.left(fileName.lastIndexOf(QLatin1Char('/')));
        if (device.isEmpty()) {
            // qmake sickness: canonicalize only the directory!
            m_directoryName = QFileInfo(m_directoryName).canonicalFilePath();
        }
    }
}

ProFile::~ProFile()
{
}

ProString ProFile::getStr(const ushort *&tPtr)
{
    uint len = *tPtr++;
    ProString ret(items(), tPtr - tokPtr(), len);
    ret.setSource(m_id);
    tPtr += len;
    return ret;
}

ProKey ProFile::getHashStr(const ushort *&tPtr)
{
    uint hash = *tPtr++;
    hash |= (uint)*tPtr++ << 16;
    uint len = *tPtr++;
    ProKey ret(items(), tPtr - tokPtr(), len, hash);
    tPtr += len;
    return ret;
}

QDebug operator<<(QDebug debug, const ProString &str)
{
    return debug << str.toQString();
}

QT_END_NAMESPACE
