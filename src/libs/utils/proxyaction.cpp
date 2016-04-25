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

#include "proxyaction.h"

using namespace Utils;

ProxyAction::ProxyAction(QObject *parent) :
    QAction(parent),
    m_action(0),
    m_attributes(0),
    m_showShortcut(false),
    m_block(false)
{
    connect(this, &QAction::changed, this, &ProxyAction::updateToolTipWithKeySequence);
    updateState();
}

void ProxyAction::setAction(QAction *action)
{
    if (m_action == action)
        return;
    disconnectAction();
    m_action = action;
    connectAction();
    updateState();
}

void ProxyAction::updateState()
{
    if (m_action) {
        update(m_action, false);
    } else {
        // no active/delegate action, "visible" action is not enabled/visible
        if (hasAttribute(Hide))
            setVisible(false);
        setEnabled(false);
    }
}

void ProxyAction::disconnectAction()
{
    if (m_action) {
        disconnect(m_action.data(), &QAction::changed, this, &ProxyAction::actionChanged);
        disconnect(this, &QAction::triggered, m_action.data(), &QAction::triggered);
        disconnect(this, &QAction::toggled, m_action.data(), &QAction::setChecked);
    }
}

void ProxyAction::connectAction()
{
    if (m_action) {
        connect(m_action.data(), &QAction::changed, this, &ProxyAction::actionChanged,
                Qt::QueuedConnection);
        connect(this, &QAction::triggered, m_action.data(), &QAction::triggered);
        connect(this, &ProxyAction::toggled, m_action.data(), &QAction::setChecked);
    }
}

QAction *ProxyAction::action() const
{
    return m_action;
}

void ProxyAction::setAttribute(ProxyAction::Attribute attribute)
{
    m_attributes |= attribute;
    updateState();
}

void ProxyAction::removeAttribute(ProxyAction::Attribute attribute)
{
    m_attributes &= ~attribute;
    updateState();
}

bool ProxyAction::hasAttribute(ProxyAction::Attribute attribute)
{
    return (m_attributes & attribute);
}

void ProxyAction::actionChanged()
{
    update(m_action, false);
}

void ProxyAction::initialize(QAction *action)
{
    update(action, true);
}

void ProxyAction::update(QAction *action, bool initialize)
{
    if (!action)
        return;
    disconnectAction();
    disconnect(this, &QAction::changed, this, &ProxyAction::updateToolTipWithKeySequence);
    if (initialize) {
        setSeparator(action->isSeparator());
        setMenuRole(action->menuRole());
    }
    if (hasAttribute(UpdateIcon) || initialize) {
        setIcon(action->icon());
        setIconText(action->iconText());
        setIconVisibleInMenu(action->isIconVisibleInMenu());
    }
    if (hasAttribute(UpdateText) || initialize) {
        setText(action->text());
        m_toolTip = action->toolTip();
        updateToolTipWithKeySequence();
        setStatusTip(action->statusTip());
        setWhatsThis(action->whatsThis());
    }

    setCheckable(action->isCheckable());

    if (!initialize) {
        setChecked(action->isChecked());
        setEnabled(action->isEnabled());
        setVisible(action->isVisible());
    }
    connectAction();
    connect(this, &QAction::changed, this, &ProxyAction::updateToolTipWithKeySequence);
}

bool ProxyAction::shortcutVisibleInToolTip() const
{
    return m_showShortcut;
}

void ProxyAction::setShortcutVisibleInToolTip(bool visible)
{
    m_showShortcut = visible;
    updateToolTipWithKeySequence();
}

void ProxyAction::updateToolTipWithKeySequence()
{
    if (m_block)
        return;
    m_block = true;
    if (!m_showShortcut || shortcut().isEmpty())
        setToolTip(m_toolTip);
    else
        setToolTip(stringWithAppendedShortcut(m_toolTip, shortcut()));
    m_block = false;
}

QString ProxyAction::stringWithAppendedShortcut(const QString &str, const QKeySequence &shortcut)
{
    QString s = str;
    s.replace(QLatin1String("&&"), QLatin1String("&"));
    return QString::fromLatin1("%1 <span style=\"color: gray; font-size: small\">%2</span>").
            arg(s, shortcut.toString(QKeySequence::NativeText));
}

ProxyAction *ProxyAction::proxyActionWithIcon(QAction *original, const QIcon &newIcon)
{
    ProxyAction *proxyAction = new ProxyAction(original);
    proxyAction->setAction(original);
    proxyAction->setIcon(newIcon);
    proxyAction->setAttribute(UpdateText);
    return proxyAction;
}
