// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    } else if (m_type == QEvent::KeyPress || m_type == QEvent::KeyRelease) {
        auto ke = static_cast<QKeyEvent *>(e);
        m_key = ke->key();
        m_count = ke->count();
        m_autoRepeat = ke->isAutoRepeat();
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
    out << command.key();
    out << command.count();
    out << command.autoRepeat();

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
    in >> command.m_key;
    in >> command.m_count;
    in >> command.m_autoRepeat;

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
                           << "angleDelta: " << command.angleDelta() << ", "
                           << "key: " << command.key() << ", "
                           << "count: " << command.count() << ", "
                           << "autoRepeat: " << command.autoRepeat() << ")";

}

} // namespace QmlDesigner
