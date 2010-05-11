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

#ifndef PROITEMS_H
#define PROITEMS_H

#include <QtCore/QString>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

#ifdef PROPARSER_THREAD_SAFE
typedef QAtomicInt ProItemRefCount;
#else
class ProItemRefCount {
public:
    ProItemRefCount(int cnt = 0) : m_cnt(cnt) {}
    bool ref() { return ++m_cnt != 0; }
    bool deref() { return --m_cnt != 0; }
    ProItemRefCount &operator=(int value) { m_cnt = value; return *this; }
private:
    int m_cnt;
};
#endif

namespace ProStringConstants {
enum OmitPreHashing { NoHash };
}

class ProString {
public:
    ProString();
    ProString(const ProString &other);
    ProString(const ProString &other, ProStringConstants::OmitPreHashing);
    explicit ProString(const QString &str);
    ProString(const QString &str, ProStringConstants::OmitPreHashing);
    explicit ProString(const char *str);
    ProString(const char *str, ProStringConstants::OmitPreHashing);
    ProString(const QString &str, int offset, int length);
    ProString(const QString &str, int offset, int length, uint hash);
    ProString(const QString &str, int offset, int length, ProStringConstants::OmitPreHashing);
    QString toQString() const;
    QString &toQString(QString &tmp) const;
    bool operator==(const ProString &other) const;
    bool operator==(const QString &other) const;
    bool operator==(const QLatin1String &other) const;
    bool operator!=(const ProString &other) const { return !(*this == other); }
    bool operator!=(const QString &other) const { return !(*this == other); }
    bool operator!=(const QLatin1String &other) const { return !(*this == other); }
    bool isEmpty() const { return !m_length; }
    int size() const { return m_length; }
    const QChar *constData() const { return m_string.constData() + m_offset; }
    ProString mid(int off, int len = -1) const;
    ProString left(int len) const { return mid(0, len); }
    ProString right(int len) const { return mid(qMax(0, size() - len)); }
    ProString trimmed() const;
    void clear() { m_string.clear(); m_length = 0; }

    static uint hash(const QChar *p, int n);

private:
    QString m_string;
    int m_offset, m_length;
    mutable uint m_hash;
    uint updatedHash() const;
    friend uint qHash(const ProString &str);
    friend QString operator+(const ProString &one, const ProString &two);
};
Q_DECLARE_TYPEINFO(ProString, Q_MOVABLE_TYPE);

uint qHash(const ProString &str);
QString operator+(const ProString &one, const ProString &two);
inline QString operator+(const ProString &one, const QString &two)
    { return one + ProString(two, ProStringConstants::NoHash); }
inline QString operator+(const QString &one, const ProString &two)
    { return ProString(one, ProStringConstants::NoHash) + two; }

class ProStringList : public QVector<ProString> {
public:
    ProStringList() {}
    ProStringList(const ProString &str) { *this << str; }
    QString join(const QString &sep) const;
    void removeDuplicates();
};

// These token definitions affect both ProFileEvaluator and ProWriter
enum ProToken {
    TokTerminator = 0,  // end of stream (possibly not included in length; must be zero)
    TokLine,            // line marker: // +1 (2-nl) to 1st token of each line
                        // - line (1)
    TokAssign,          // variable =   // "A=":2 => 1+4+2=7 (8)
    TokAppend,          // variable +=  // "A+=":3 => 1+4+2=7 (8)
    TokAppendUnique,    // variable *=  // "A*=":3 => 1+4+2=7 (8)
    TokRemove,          // variable -=  // "A-=":3 => 1+4+2=7 (8)
    TokReplace,         // variable ~=  // "A~=":3 => 1+4+2=7 (8)
                        // - variable name: hash (2), length (1), chars (length)
                        // - expression: length (2), chars (length)
    TokCondition,       // CONFIG test:   // "A":1 => 1+2=3 (4)
                        // - test name: lenght (1), chars (length)
    TokNot,             // '!' operator
    TokAnd,             // ':' operator
    TokOr,              // '|' operator
    TokBranch,          // branch point:   // "X:A=":4 => [5]+1+4+1+1+[7]=19 (20)
                        // - then block length (2)
                        // - then block + TokTerminator (then block length)
                        // - else block length (2)
                        // - else block + TokTerminator (else block length)
    TokForLoop,         // for loop:   // "for(A,B)":8 => 1+4+3+2+1=11 (12)
                        // - variable name: hash (2), length (1), chars (length)
                        // - expression: length (2), chars (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
    TokTestDef,         // test function definition:     // "defineTest(A):":14 => 1+4+2+1=8 (9)
    TokReplaceDef,      // replace function definition:  // "defineReplace(A):":17 => 1+4+2+1=8 (9)
                        // - function name: hash (2), length (1), chars (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
};

class ProFile
{
public:
    explicit ProFile(const QString &fileName);
    ~ProFile();

    QString displayFileName() const { return m_displayFileName; }
    QString fileName() const { return m_fileName; }
    QString directoryName() const { return m_directoryName; }
    const QString &items() const { return m_proitems; }
    QString *itemsRef() { return &m_proitems; }

    void ref() { m_refCount.ref(); }
    void deref() { if (!m_refCount.deref()) delete this; }

private:
    ProItemRefCount m_refCount;
    QString m_proitems;
    QString m_fileName;
    QString m_displayFileName;
    QString m_directoryName;
};

QT_END_NAMESPACE

#endif // PROITEMS_H
