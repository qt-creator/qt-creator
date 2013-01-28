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

#include "proxyaction.h"

using namespace Utils;

ProxyAction::ProxyAction(QObject *parent) :
    QAction(parent),
    m_action(0),
    m_attributes(0),
    m_showShortcut(false),
    m_block(false)
{
    connect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
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
        disconnect(m_action, SIGNAL(changed()), this, SLOT(actionChanged()));
        disconnect(this, SIGNAL(triggered(bool)), m_action, SIGNAL(triggered(bool)));
        disconnect(this, SIGNAL(toggled(bool)), m_action, SLOT(setChecked(bool)));
    }
}

void ProxyAction::connectAction()
{
    if (m_action) {
        connect(m_action, SIGNAL(changed()), this, SLOT(actionChanged()));
        connect(this, SIGNAL(triggered(bool)), m_action, SIGNAL(triggered(bool)));
        connect(this, SIGNAL(toggled(bool)), m_action, SLOT(setChecked(bool)));
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
    disconnect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
    if (initialize)
        setSeparator(action->isSeparator());
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
    connect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
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
    return QString::fromLatin1("%1 <span style=\"color: gray; font-size: small\">%2</span>").
            arg(str, shortcut.toString(QKeySequence::NativeText));
}
