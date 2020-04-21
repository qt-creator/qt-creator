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

#include "inputeventcommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

InputEventCommand::InputEventCommand() = default;

InputEventCommand::InputEventCommand(QInputEvent *e)
    : m_type(e->type()),
      m_modifiers(e->modifiers())
{
    if (m_type == QEvent::Wheel) {
        auto we = static_cast<QWheelEvent *>(e);
#if QT_VERSION <= QT_VERSION_CHECK(5, 15, 0)
        m_pos = we->pos();
#else
        m_pos = we->position().toPoint();
#endif
        m_buttons = we->buttons();
        m_angleDelta = we->angleDelta().y();
    } else {
        auto me = static_cast<QMouseEvent *>(e);
        m_pos = me->pos();
        m_button = me->button();
        m_buttons = me->buttons();
    }
}

QDataStream &operator<<(QDataStream &out, const InputEventCommand &command)
{
    out << int(command.type());
    out << command.pos();
    out << int(command.button());
    out << command.buttons();
    out << command.modifiers();
    out << command.angleDelta();

    return out;
}

QDataStream &operator>>(QDataStream &in, InputEventCommand &command)
{
    int type;
    int button;
    in >> type;
    command.m_type = (QEvent::Type)type;
    in >> command.m_pos;
    in >> button;
    command.m_button = (Qt::MouseButton)button;
    in >> command.m_buttons;
    in >> command.m_modifiers;
    in >> command.m_angleDelta;

    return in;
}

QDebug operator <<(QDebug debug, const InputEventCommand &command)
{
    return debug.nospace() << "InputEventCommand("
                           << "type: " << command.type() << ", "
                           << "pos: " << command.pos() << ", "
                           << "button: " << command.button() << ", "
                           << "buttons: " << command.buttons() << ", "
                           << "modifiers: " << command.modifiers() << ", "
                           << "angleDelta: " << command.angleDelta() << ")";

}

} // namespace QmlDesigner
