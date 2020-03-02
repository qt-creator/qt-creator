/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtCore/qmetatype.h>
#include <QtCore/qdatastream.h>
#include <QtGui/qevent.h>

#include "instancecontainer.h"

namespace QmlDesigner {

class InputEventCommand
{
    friend QDataStream &operator>>(QDataStream &in, InputEventCommand &command);
    friend QDebug operator <<(QDebug debug, const InputEventCommand &command);

public:
    InputEventCommand();
    explicit InputEventCommand(QInputEvent *e);

    QEvent::Type type() const { return m_type; }
    QPoint pos() const { return m_pos; }
    Qt::MouseButton button() const { return m_button; }
    Qt::MouseButtons buttons() const { return m_buttons; }
    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }
    int angleDelta() const { return m_angleDelta; }

private:
    QEvent::Type m_type = QEvent::None;
    QPoint m_pos;
    Qt::MouseButton m_button = Qt::NoButton;
    Qt::MouseButtons m_buttons = Qt::NoButton;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;
    int m_angleDelta = 0;
};

QDataStream &operator<<(QDataStream &out, const InputEventCommand &command);
QDataStream &operator>>(QDataStream &in, InputEventCommand &command);

QDebug operator <<(QDebug debug, const InputEventCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InputEventCommand)
