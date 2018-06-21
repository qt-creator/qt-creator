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

#pragma once

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>

#include <QList>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include <functional>

namespace Core {

class CORE_EXPORT Context
{
public:
    Context() {}

    explicit Context(Id c1) { add(c1); }
    Context(Id c1, Id c2) { add(c1); add(c2); }
    Context(Id c1, Id c2, Id c3) { add(c1); add(c2); add(c3); }
    bool contains(Id c) const { return d.contains(c); }
    int size() const { return d.size(); }
    bool isEmpty() const { return d.isEmpty(); }
    Id at(int i) const { return d.at(i); }

    // FIXME: Make interface slimmer.
    typedef QList<Id>::const_iterator const_iterator;
    const_iterator begin() const { return d.begin(); }
    const_iterator end() const { return d.end(); }
    int indexOf(Id c) const { return d.indexOf(c); }
    void removeAt(int i) { d.removeAt(i); }
    void prepend(Id c) { d.prepend(c); }
    void add(const Context &c) { d += c.d; }
    void add(Id c) { d.append(c); }
    bool operator==(const Context &c) const { return d == c.d; }

private:
    QList<Id> d;
};

class CORE_EXPORT IContext : public QObject
{
    Q_OBJECT
public:
    IContext(QObject *parent = nullptr) : QObject(parent) {}

    virtual Context context() const { return m_context; }
    virtual QWidget *widget() const { return m_widget; }
    using HelpIdCallback = std::function<void(const QString &id)>;
    virtual void contextHelpId(const HelpIdCallback &callback) const { callback(m_contextHelpId); }

    virtual void setContext(const Context &context) { m_context = context; }
    virtual void setWidget(QWidget *widget) { m_widget = widget; }
    virtual void setContextHelpId(const QString &id) { m_contextHelpId = id; }

protected:
    Context m_context;
    QPointer<QWidget> m_widget;
    QString m_contextHelpId;
};

} // namespace Core
CORE_EXPORT QDebug operator<<(QDebug debug, const Core::Context &context);
