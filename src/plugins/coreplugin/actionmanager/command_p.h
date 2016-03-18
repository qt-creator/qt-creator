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

class Action : public Command
{
    Q_OBJECT
public:
    Action(Id id);

    Id id() const;

    void setDefaultKeySequence(const QKeySequence &key);
    QKeySequence defaultKeySequence() const;

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    void setDescription(const QString &text);
    QString description() const;

    QAction *action() const;

    QString stringWithAppendedShortcut(const QString &str) const;

    Context context() const;
    void setCurrentContext(const Context &context);

    bool isActive() const;
    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

    bool isScriptable() const;
    bool isScriptable(const Context &context) const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

private:
    void updateActiveState();
    void setActive(bool state);

    Context m_context;
    CommandAttributes m_attributes;
    Id m_id;
    QKeySequence m_defaultKey;
    QString m_defaultText;
    bool m_isKeyInitialized;

    Utils::ProxyAction *m_action;
    QString m_toolTip;

    QMap<Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active;
    bool m_contextInitialized;
};

} // namespace Internal
} // namespace Core
