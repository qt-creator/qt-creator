/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ICONTEXT_H
#define ICONTEXT_H

#include <coreplugin/core_global.h>

#include <QtCore/QList>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT Context
{
public:
    Context() {}

    explicit Context(const char *c1) { add(c1); }
    Context(const char *c1, const char *c2) { add(c1); add(c2); }
    Context(const char *c1, const char *c2, const char *c3) { add(c1); add(c2); add(c3); }
    Context(const char *base, int offset);
    void add(const char *c);
    bool contains(const char *c) const;
    bool contains(int c) const { return d.contains(c); }
    int size() const { return d.size(); }
    bool isEmpty() const { return d.isEmpty(); }
    int at(int i) const { return d.at(i); }

    // FIXME: Make interface slimmer.
    typedef QList<int>::const_iterator const_iterator;
    const_iterator begin() const { return d.begin(); }
    const_iterator end() const { return d.end(); }
    int indexOf(int c) const { return d.indexOf(c); }
    void removeAt(int i) { d.removeAt(i); }
    void prepend(int c) { d.prepend(c); }
    void add(const Context &c) { d += c.d; }
    void add(int c) { d.append(c); }
    bool operator==(const Context &c) const { return d == c.d; }

private:
    QList<int> d;
};

class CORE_EXPORT IContext : public QObject
{
    Q_OBJECT
public:
    IContext(QObject *parent = 0) : QObject(parent) {}
    virtual ~IContext() {}

    virtual Context context() const = 0;
    virtual QWidget *widget() = 0;
    virtual QString contextHelpId() const { return QString(); }
};

class BaseContext : public Core::IContext
{
public:
    BaseContext(QWidget *widget, const Context &context, QObject *parent = 0)
        : Core::IContext(parent),
        m_widget(widget),
        m_context(context)
    {
    }

    Context context() const { return m_context; }

    QWidget *widget() { return m_widget; }

private:
    QWidget *m_widget;
    Context m_context;
};

} // namespace Core

#endif //ICONTEXT_H
