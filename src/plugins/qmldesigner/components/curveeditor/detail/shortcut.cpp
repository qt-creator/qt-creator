/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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
#include "shortcut.h"

namespace DesignTools {

Shortcut::Shortcut()
    : m_key()
    , m_buttons()
    , m_modifiers()
{}

Shortcut::Shortcut(QMouseEvent *event)
    : m_key()
    , m_buttons(event->buttons())
    , m_modifiers(event->modifiers())
{}

Shortcut::Shortcut(const Qt::KeyboardModifiers &mods, const Qt::Key &key)
    : m_key(key)
    , m_buttons()
    , m_modifiers(mods)
{}

Shortcut::Shortcut(const Qt::MouseButtons &buttons, const Qt::KeyboardModifiers &mods)
    : m_key()
    , m_buttons(buttons)
    , m_modifiers(mods)
{}

Shortcut::Shortcut(const Qt::MouseButtons &buttons,
                   const Qt::KeyboardModifiers &mods,
                   const Qt::Key &key)
    : m_key(key)
    , m_buttons(buttons)
    , m_modifiers(mods)
{}

bool Shortcut::exactMatch(const Qt::Key &key) const
{
    return m_key == key;
}

bool Shortcut::exactMatch(const Qt::MouseButton &button) const
{
    return static_cast<int>(m_buttons) == static_cast<int>(button);
}

bool Shortcut::exactMatch(const Qt::KeyboardModifier &modifier) const
{
    return static_cast<int>(m_modifiers) == static_cast<int>(modifier);
}

bool Shortcut::operator==(const Shortcut &other) const
{
    return m_key == other.m_key && m_buttons == other.m_buttons && m_modifiers == other.m_modifiers;
}

} // End namespace DesignTools.
