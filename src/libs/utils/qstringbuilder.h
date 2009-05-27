/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSTRINGBUILDER_H
#define QSTRINGBUILDER_H

#include <QtCore/qstring.h>

#include <string.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)


// Using this relies on changing the QString::QString(QChar *, int)
// constructor to allocated an unitialized string of the given size.
//#define USE_CHANGED_QSTRING 1

class QLatin1Literal
{
public:
    template <int N>
    QLatin1Literal(const char (&str)[N]) : m_size(N - 1), m_data(str) {}

    inline int size() const { return m_size; }
    inline const char *data() const { return m_data; }

private:
    const int m_size;
    const char *m_data;
};


namespace {

template <typename A, typename B>
class QStringBuilder
{
public:
    QStringBuilder(const A &a_, const B &b_) : a(a_), b(b_) {}
    operator QString() const;

public:
    const A &a;
    const B &b;
};

// make sure the operator% defined below acts only on types we want to handle.
template <typename T> struct QConcatenable {};


template <typename A, typename B>
QStringBuilder<A, B>::operator QString() const 
{
#ifdef USE_CHANGED_QSTRING
    QString s(this->size(), QChar(-1));
#else
    QString s;
    s.resize(QConcatenable< QStringBuilder<A, B> >::size(*this));
#endif
    QChar *d = s.data();
    QConcatenable< QStringBuilder<A, B> >::appendTo(*this, d);
    return s;
}

template <> struct QConcatenable<char>
{
    typedef char type;
    static int size(const char) { return 1; }
    static inline void appendTo(const char c, QChar *&out)
    {
        *out++ = QLatin1Char(c);
    }
};

template <> struct QConcatenable<QLatin1Char>
{
    typedef QLatin1Char type;
    static int size(const QLatin1Char) { return 1; }
    static inline void appendTo(const QLatin1Char c, QChar *&out)
    {
        *out++ = c;
    }
};

template <> struct QConcatenable<QLatin1String>
{
    typedef QLatin1String type;
    static int size(const QLatin1String &a) { return qstrlen(a.latin1()); }
    static inline void appendTo(const QLatin1String &a, QChar *&out)
    {
        for (const char *s = a.latin1(); *s; )
            *out++ = QLatin1Char(*s++);
    }

};

template <> struct QConcatenable<QLatin1Literal>
{
    typedef QLatin1Literal type;
    static int size(const QLatin1Literal &a) { return a.size(); }
    static inline void appendTo(const QLatin1Literal &a, QChar *&out)
    {
        for (const char *s = a.data(); *s; )
            *out++ = QLatin1Char(*s++);
    }
};

template <> struct QConcatenable<QString>
{
    typedef QString type;
    static int size(const QString &a) { return a.size(); }
    static inline void appendTo(const QString &a, QChar *&out)
    {
        const int n = a.size();
        memcpy(out, (char*)a.constData(), sizeof(QChar) * n);
        out += n; 
    }
};

template <> struct QConcatenable<QStringRef>
{
    typedef QStringRef type;
    static int size(const QStringRef &a) { return a.size(); }
    static inline void appendTo(QStringRef a, QChar *&out)
    {
        const int n = a.size();
        memcpy(out, (char*)a.constData(), sizeof(QChar) * n);
        out += n; 
    }
};


template <typename A, typename B>
struct QConcatenable< QStringBuilder<A, B> >
{
    typedef QStringBuilder<A, B> type;
    static int size(const type &p) 
    {
        return QConcatenable<A>::size(p.a) + QConcatenable<B>::size(p.b);
    }
    static inline void appendTo(const QStringBuilder<A, B> &p, QChar *&out)
    {
        QConcatenable<A>::appendTo(p.a, out);
        QConcatenable<B>::appendTo(p.b, out);
    }
};

} // namespace


template <typename A, typename B>
QStringBuilder<A, B> operator%(const A &a, const B &b)
{
    typedef typename QConcatenable<A>::type A1;
    typedef typename QConcatenable<B>::type B1;
    return QStringBuilder<A1, B1>(a, b);
}


QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTRINGBUILDER_H
