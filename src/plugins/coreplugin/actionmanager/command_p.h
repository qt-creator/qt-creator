/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COMMAND_P_H
#define COMMAND_P_H

#include "command.h"

#include <coreplugin/id.h>
#include <coreplugin/icontext.h>

#include <utils/proxyaction.h>

#include <QList>
#include <QMultiMap>
#include <QPointer>
#include <QMap>
#include <QKeySequence>

namespace Core {
namespace Internal {

class CommandPrivate : public Core::Command
{
    Q_OBJECT
public:
    CommandPrivate(Id id);
    virtual ~CommandPrivate() {}

    void setDefaultKeySequence(const QKeySequence &key);
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);

    void setDescription(const QString &text);
    QString description() const;

    Id id() const;

    Context context() const;


    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

    virtual void setCurrentContext(const Context &context) = 0;

    QString stringWithAppendedShortcut(const QString &str) const;

protected:
    Context m_context;
    CommandAttributes m_attributes;
    Id m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;
};

class Shortcut : public CommandPrivate
{
    Q_OBJECT
public:
    Shortcut(Id id);

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    void setShortcut(QShortcut *shortcut);
    QShortcut *shortcut() const;

    QAction *action() const { return 0; }

    void setContext(const Context &context);
    Context context() const;
    void setCurrentContext(const Context &context);

    bool isActive() const;

    bool isScriptable() const;
    bool isScriptable(const Context &) const;
    void setScriptable(bool value);

private:
    QShortcut *m_shortcut;
    bool m_scriptable;
};

class Action : public CommandPrivate
{
    Q_OBJECT
public:
    Action(Id id);

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    QAction *action() const;
    QShortcut *shortcut() const { return 0; }

    void setCurrentContext(const Context &context);
    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

    bool isScriptable() const;
    bool isScriptable(const Context &context) const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);

private slots:
    void updateActiveState();

private:
    void setActive(bool state);

    Utils::ProxyAction *m_action;
    QString m_toolTip;

    QMap<Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H
