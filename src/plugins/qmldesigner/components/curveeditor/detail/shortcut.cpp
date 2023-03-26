// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "shortcut.h"

namespace QmlDesigner {

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

} // End namespace QmlDesigner.
