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

    void appendTo(QChar *&out) const
    {
        const char *s = m_data;
        for (int i = m_size; --i >= 0;)    
            *out++ = QLatin1Char(*s++);
    }

    operator QString() const
    {
#ifdef USE_CHANGED_QSTRING
        QString s(m_size, QChar(-1));
#else
        QString s;
        s.resize(m_size);
#endif
        QChar *d = s.data();
        appendTo(d);
        return s;
    }


private:
    const int m_size;
    const char *m_data;
};

template <typename A>
class QStringBuilder : public A
{
public:
    QStringBuilder(const A &a_) : A(a_) {}

    operator QString() const
    {
#ifdef USE_CHANGED_QSTRING
        QString s(this->size(), QChar(-1));
#else
        QString s;
        s.resize(this->size());
#endif
        QChar *d = s.data();
        this->appendTo(d);
        return s;
    }

};

template <>
class QStringBuilder<QString>
{
public:
    QStringBuilder(const QString &a_) : a(&a_) {}

    inline int size() const { return a->size(); }

    inline void appendTo(QChar *&out) const
    {
        const int n = a->size();
        memcpy(out, (char*)a->constData(), sizeof(QChar) * n);
        out += n; 
    }

    inline operator QString() const { return *a; }

private:
    const QString *a;
};


template <typename A>
int qStringBuilderSize(const A a) { return a.size(); }

inline int qStringBuilderSize(const char) { return 1; }

inline int qStringBuilderSize(const QLatin1Char) { return 1; }

inline int qStringBuilderSize(const QLatin1String a) { return qstrlen(a.latin1()); }


template <typename A>
inline void qStringBuilderAppend(const A a, QChar *&out) { a.appendTo(out); }

inline void qStringBuilderAppend(char c, QChar *&out) { *out++ = QLatin1Char(c); }

inline void qStringBuilderAppend(QLatin1Char c, QChar *&out) { *out++ = c; }

inline void qStringBuilderAppend(QLatin1String a, QChar *&out)
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



template <typename A, typename B>
class QStringBuilderPair
{
public:
    QStringBuilderPair(const A &a_, const B &b_) : a(a_), b(b_) {}

    inline int size() const
    {
        return qStringBuilderSize(a) + qStringBuilderSize(b);
    }

    inline void appendTo(QChar *&out) const
    {
        qStringBuilderAppend(a, out);
        qStringBuilderAppend(b, out);
    }

private:
    const A a;
    const B b;
};


template <typename A, typename B>
QStringBuilder< QStringBuilderPair<A, B> >
operator%(const A &a, const B &b)
{
    return QStringBuilderPair<A, B> (a, b);
}


// QString related specializations

template <typename A>
inline QStringBuilder< QStringBuilderPair<A, QStringBuilder<QString> > >
operator%(const A &a, const QString &b)
{
    return QStringBuilderPair<A, QStringBuilder<QString> > (a, b);
}

template <typename B>
inline QStringBuilder< QStringBuilderPair<QStringBuilder<QString>, B> >
operator%(const QString &a, const B &b)
{
    return QStringBuilderPair<QStringBuilder<QString>, B> (a, b);
}

inline QStringBuilder<
    QStringBuilderPair<QStringBuilder<QString>, QStringBuilder<QString> >
>
operator%(const QString &a, const QString &b)
{
    return QStringBuilderPair< QStringBuilder<QString>,
        QStringBuilder<QString> > (a, b);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTRINGBUILDER_H
