/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMMAND_P_H
#define COMMAND_P_H

#include "command.h"
#include "actionmanager_p.h"

#include <QtCore/QList>
#include <QtCore/QMultiMap>
#include <QtCore/QPointer>
#include <QtGui/QKeySequence>

namespace Core {
namespace Internal {

class CommandPrivate : public Core::Command
{
    Q_OBJECT
public:
    CommandPrivate(int id);
    virtual ~CommandPrivate() {}

    virtual QString name() const = 0;

    void setDefaultKeySequence(const QKeySequence &key);
    QKeySequence defaultKeySequence() const;

    void setDefaultText(const QString &text);
    QString defaultText() const;

    int id() const;

    QAction *action() const;
    QShortcut *shortcut() const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

    virtual bool setCurrentContext(const QList<int> &context) = 0;

    QString stringWithAppendedShortcut(const QString &str) const;

protected:
    QString m_category;
    int m_attributes;
    int m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
};

class Shortcut : public CommandPrivate
{
    Q_OBJECT
public:
    Shortcut(int id);

    QString name() const;

    void setDefaultKeySequence(const QKeySequence &key);
    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    virtual void setDefaultText(const QString &key);
    virtual QString defaultText() const;

    void setShortcut(QShortcut *shortcut);
    QShortcut *shortcut() const;

    void setContext(const QList<int> &context);
    QList<int> context() const;
    bool setCurrentContext(const QList<int> &context);

    bool isActive() const;
private:
    QList<int> m_context;
    QShortcut *m_shortcut;
    QString m_defaultText;
};

class Action : public CommandPrivate
{
    Q_OBJECT
public:
    Action(int id);

    QString name() const;

    void setDefaultKeySequence(const QKeySequence &key);
    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    virtual void setAction(QAction *action);
    QAction *action() const;

    void setLocations(const QList<CommandLocation> &locations);
    QList<CommandLocation> locations() const;

protected:
    void updateToolTipWithKeySequence();
    
    QAction *m_action;
    QList<CommandLocation> m_locations;
    QString m_toolTip;
};

class OverrideableAction : public Action
{
    Q_OBJECT

public:
    OverrideableAction(int id);

    void setAction(QAction *action);
    bool setCurrentContext(const QList<int> &context);
    void addOverrideAction(QAction *action, const QList<int> &context);
    bool isActive() const;

private slots:
    void actionChanged();

private:
    QPointer<QAction> m_currentAction;
    QList<int> m_context;
    QMap<int, QPointer<QAction> > m_contextActionMap;
    bool m_active;
    bool m_contextInitialized;
    QAction m_dummyShortcutEater;
};

} // namespace Internal
} // namespace Core

#endif // COMMAND_P_H
