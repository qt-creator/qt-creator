// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMouseEvent>

namespace QmlDesigner {

class Shortcut
{
public:
    Shortcut();

    Shortcut(QMouseEvent *event);

    Shortcut(const Qt::KeyboardModifiers &mods, const Qt::Key &key);

    Shortcut(const Qt::MouseButtons &buttons, const Qt::KeyboardModifiers &mods = Qt::NoModifier);

    Shortcut(const Qt::MouseButtons &buttons, const Qt::KeyboardModifiers &mods, const Qt::Key &key);

    bool exactMatch(const Qt::Key &key) const;

    bool exactMatch(const Qt::MouseButton &button) const;

    bool exactMatch(const Qt::KeyboardModifier &modifier) const;

    bool operator==(const Shortcut &other) const;

private:
    Qt::Key m_key;

    Qt::MouseButtons m_buttons;

    Qt::KeyboardModifiers m_modifiers;
};

} // End namespace QmlDesigner.
