// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "proxyaction.h"

#include "stringutils.h"

using namespace Utils;

ProxyAction::ProxyAction(QObject *parent) :
    QAction(parent)
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
    emit currentActionChanged(action);
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
        disconnect(this, &ProxyAction::triggered, m_action.data(), &QAction::triggered);
        disconnect(this, &ProxyAction::toggled, m_action.data(), &QAction::setChecked);
    }
}

void ProxyAction::connectAction()
{
    if (m_action) {
        connect(m_action.data(), &QAction::changed, this, &ProxyAction::actionChanged);
        connect(this, &ProxyAction::triggered, m_action.data(), &QAction::triggered);
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
    disconnect(this, &ProxyAction::changed, this, &ProxyAction::updateToolTipWithKeySequence);
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
        if (isChecked() != action->isChecked()) {
            if (m_action)
                disconnect(this, &ProxyAction::toggled, m_action.data(), &QAction::setChecked);
            setChecked(action->isChecked());
            if (m_action)
                connect(this, &ProxyAction::toggled, m_action.data(), &QAction::setChecked);
        }
        setEnabled(action->isEnabled());
        setVisible(action->isVisible());
    }
    connect(this, &ProxyAction::changed, this, &ProxyAction::updateToolTipWithKeySequence);
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
    const QString s = stripAccelerator(str);
    return QString::fromLatin1("<div style=\"white-space:pre\">%1 "
                               "<span style=\"color: gray; font-size: small\">%2</span></div>")
        .arg(s, shortcut.toString(QKeySequence::NativeText));
}

ProxyAction *ProxyAction::proxyActionWithIcon(QAction *original, const QIcon &newIcon)
{
    auto proxyAction = new ProxyAction(original);
    proxyAction->setAction(original);
    proxyAction->setIcon(newIcon);
    proxyAction->setAttribute(UpdateText);
    return proxyAction;
}
