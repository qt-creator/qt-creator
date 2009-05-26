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

    operator QString() const
    {
#ifdef USE_CHANGED_QSTRING
        QString s(m_size, QChar(-1));
#else
        QString s;
        s.resize(m_size);
#endif
        QChar *d = s.data();
        for (const char *s = m_data; *s; )
            *d++ = QLatin1Char(*s++);
        return s;
    }


private:
    const int m_size;
    const char *m_data;
};


namespace Qt {

inline int qStringBuilderSize(const char) { return 1; }

inline int qStringBuilderSize(const QLatin1Char) { return 1; }

inline int qStringBuilderSize(const QLatin1String &a) { return qstrlen(a.latin1()); }

inline int qStringBuilderSize(const QLatin1Literal &a) { return a.size(); }

inline int qStringBuilderSize(const QString &a) { return a.size(); }

inline int qStringBuilderSize(const QStringRef &a) { return a.size(); }

template <typename A, typename B> class  QStringBuilder;

template <typename A, typename B>
inline int qStringBuilderSize(const QStringBuilder<A, B> &p);

inline void qStringBuilderAppend(const char c, QChar *&out)
{
    *out++ = QLatin1Char(c);
}

inline void qStringBuilderAppend(const QLatin1Char c, QChar *&out)
{
    *out++ = c;
}

inline void qStringBuilderAppend(const QLatin1String &a, QChar *&out)
{
    for (const char *s = a.latin1(); *s; )
        *out++ = QLatin1Char(*s++);
}

inline void qStringBuilderAppend(QStringRef a, QChar *&out)
{
    const int n = a.size();
    memcpy(out, (char*)a.constData(), sizeof(QChar) * n);
    out += n; 
}

inline void qStringBuilderAppend(const QString &a, QChar *&out)
{
    const int n = a.size();
    memcpy(out, (char*)a.constData(), sizeof(QChar) * n);
    out += n; 
}

inline void qStringBuilderAppend(const QLatin1Literal &a, QChar *&out)
{
    for (const char *s = a.data(); *s; )
        *out++ = QLatin1Char(*s++);
}

template <typename A, typename B>
inline void qStringBuilderAppend(const QStringBuilder<A, B> &p, QChar
*&out);

template <typename A, typename B>
class QStringBuilder
{
public:
    QStringBuilder(const A &a_, const B &b_) : a(a_), b(b_) {}

    operator QString() const
    {
    #ifdef USE_CHANGED_QSTRING
        QString s(this->size(), QChar(-1));
    #else
        QString s;
        s.resize(Qt::qStringBuilderSize(*this));
    #endif
        QChar *d = s.data();
        Qt::qStringBuilderAppend(*this, d);
        return s;
    }

public:
    const A &a;
    const B &b;
};


template <typename A, typename B>
inline int qStringBuilderSize(const QStringBuilder<A, B> &p)
{
    return qStringBuilderSize(p.a) + qStringBuilderSize(p.b);
}

template <typename A, typename B>
inline void qStringBuilderAppend(const QStringBuilder<A, B> &p, QChar *&out)
{
    qStringBuilderAppend(p.a, out);
    qStringBuilderAppend(p.b, out);
}

} // Qt


template <typename A, typename B>
Qt::QStringBuilder<A, B>
operator%(const A &a, const B &b)
{
    return Qt::QStringBuilder<A, B>(a, b);
}


QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTRINGBUILDER_H
