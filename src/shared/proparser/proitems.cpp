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

#include "proitems.h"

#include <QtCore/QFileInfo>
#include <QtCore/QSet>

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;

// from qhash.cpp
static uint hash(const QChar *p, int n)
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
    m_offset(0), m_length(0), m_hash(0x80000000)
{
}

ProString::ProString(const ProString &other) :
        m_string(other.m_string), m_offset(other.m_offset), m_length(other.m_length), m_hash(other.m_hash)
{
}

ProString::ProString(const ProString &other, OmitPreHashing) :
        m_string(other.m_string), m_offset(other.m_offset), m_length(other.m_length), m_hash(0x80000000)
{
}

ProString::ProString(const QString &str) :
    m_string(str), m_offset(0), m_length(str.length())
{
    updatedHash();
}

ProString::ProString(const QString &str, OmitPreHashing) :
    m_string(str), m_offset(0), m_length(str.length()), m_hash(0x80000000)
{
}

ProString::ProString(const char *str) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str))
{
    updatedHash();
}

ProString::ProString(const char *str, OmitPreHashing) :
    m_string(QString::fromLatin1(str)), m_offset(0), m_length(qstrlen(str)), m_hash(0x80000000)
{
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

void ProItem::disposeItems(ProItem *nitm)
{
    for (ProItem *itm; (itm = nitm); ) {
        nitm = itm->next();
        switch (itm->kind()) {
        case ProItem::ConditionKind: delete static_cast<ProCondition *>(itm); break;
        case ProItem::VariableKind: delete static_cast<ProVariable *>(itm); break;
        case ProItem::BranchKind: delete static_cast<ProBranch *>(itm); break;
        case ProItem::LoopKind: delete static_cast<ProLoop *>(itm); break;
        case ProItem::FunctionDefKind: static_cast<ProFunctionDef *>(itm)->deref(); break;
        case ProItem::OpNotKind:
        case ProItem::OpAndKind:
        case ProItem::OpOrKind:
            delete itm;
            break;
        }
    }
}

ProBranch::~ProBranch()
{
    disposeItems(m_thenItems);
    disposeItems(m_elseItems);
}

ProLoop::~ProLoop()
{
    disposeItems(m_proitems);
}

ProFunctionDef::~ProFunctionDef()
{
    disposeItems(m_proitems);
}

ProFile::ProFile(const QString &fileName)
    : m_proitems(0),
      m_refCount(1),
      m_fileName(fileName)
{
    int nameOff = fileName.lastIndexOf(QLatin1Char('/'));
    m_displayFileName = QString(fileName.constData() + nameOff + 1,
                                fileName.length() - nameOff - 1);
    m_directoryName = QString(fileName.constData(), nameOff);
}

ProFile::~ProFile()
{
    ProItem::disposeItems(m_proitems);
}

QT_END_NAMESPACE
