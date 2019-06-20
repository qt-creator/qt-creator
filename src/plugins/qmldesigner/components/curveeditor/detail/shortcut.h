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

#pragma once

#include <QMouseEvent>

namespace DesignTools {

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

} // End namespace DesignTools.
