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

#include "proitems.h"

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

ProString::ProString(const char *str, DoPreHashing) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str)), m_file(0)
{
    updatedHash();
}

ProString::ProString(const char *str) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str)), m_file(0), m_hash(0x80000000)
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

uint qHash(const ProString &str)
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
    return tmp.setRawData(m_string.constData() + m_offset, m_length);
}

/*!
 * \brief ProString::prepareExtend
 * \param extraLen number of new characters to be added
 * \param thisTarget offset to which current contents should be moved
 * \param extraTarget offset at which new characters will be added
 * \return pointer to storage location for new characters
 *
 * Prepares the string for adding new characters.
 * If the string is detached and has enough space, it will be changed in place.
 * Otherwise, it will be replaced with a new string object, thus detaching.
 * In either case, the hash will be reset.
 */
QChar *ProString::prepareExtend(int extraLen, int thisTarget, int extraTarget)
{
    if (m_string.isDetached() && m_length + extraLen <= m_string.capacity()) {
        m_string.reserve(0); // Prevent the resize() below from reallocating
        QChar *ptr = (QChar *)m_string.constData();
        if (m_offset != thisTarget)
            memmove(ptr + thisTarget, ptr + m_offset, m_length * 2);
        ptr += extraTarget;
        m_offset = 0;
        m_length += extraLen;
        m_string.resize(m_length);
        m_hash = 0x80000000;
        return ptr;
    } else {
        QString neu(m_length + extraLen, Qt::Uninitialized);
        QChar *ptr = (QChar *)neu.constData();
        memcpy(ptr + thisTarget, m_string.constData() + m_offset, m_length * 2);
        ptr += extraTarget;
        *this = ProString(neu);
        return ptr;
    }
}

ProString &ProString::prepend(const ProString &other)
{
    if (other.m_length) {
        if (!m_length) {
            *this = other;
        } else {
            QChar *ptr = prepareExtend(other.m_length, other.m_length, 0);
            memcpy(ptr, other.constData(), other.m_length * 2);
            if (!m_file)
                m_file = other.m_file;
        }
    }
    return *this;
}

ProString &ProString::append(const QLatin1String other)
{
    const char *latin1 = other.latin1();
    int size = other.size();
    if (size) {
        QChar *ptr = prepareExtend(size, 0, m_length);
        for (int i = 0; i < size; i++)
            *ptr++ = QLatin1Char(latin1[i]);
    }
    return *this;
}

ProString &ProString::append(QChar other)
{
    QChar *ptr = prepareExtend(1, 0, m_length);
    *ptr = other;
    return *this;
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
                ptr = prepareExtend(1 + other.m_length, 0, m_length);
                *ptr++ = 32;
            } else {
                ptr = prepareExtend(other.m_length, 0, m_length);
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

            QChar *ptr = prepareExtend(totalLength, 0, m_length);
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
    t << str.toQString(); // XXX optimize ... somehow
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
    for (const ProString &str : value)
        if (!str.isEmpty())
            removeAll(str);
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

bool ProStringList::contains(const char *str, Qt::CaseSensitivity cs) const
{
    for (int i = 0; i < size(); i++)
        if (!at(i).compare(str, cs))
            return true;
    return false;
}

ProFile::ProFile(const QString &fileName)
    : m_refCount(1),
      m_fileName(fileName),
      m_ok(true),
      m_hostBuild(false)
{
    if (!fileName.startsWith(QLatin1Char('(')))
        m_directoryName = QFileInfo( // qmake sickness: canonicalize only the directory!
                fileName.left(fileName.lastIndexOf(QLatin1Char('/')))).canonicalFilePath();
}

ProFile::~ProFile()
{
}

ProString ProFile::getStr(const ushort *&tPtr)
{
    uint len = *tPtr++;
    ProString ret(items(), tPtr - tokPtr(), len);
    ret.setSource(this);
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

QT_END_NAMESPACE
