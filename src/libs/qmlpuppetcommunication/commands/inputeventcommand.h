// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    int key() const { return m_key; }
    int count() const { return m_count; }
    bool autoRepeat() const { return m_autoRepeat; }

private:
    QEvent::Type m_type = QEvent::None;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;

    // Mouse events
    QPoint m_pos;
    Qt::MouseButton m_button = Qt::NoButton;
    Qt::MouseButtons m_buttons = Qt::NoButton;

    // Wheel events
    int m_angleDelta = 0;

    // Key events
    int m_key = 0;
    int m_count = 1;
    bool m_autoRepeat = false;
};

QDataStream &operator<<(QDataStream &out, const InputEventCommand &command);
QDataStream &operator>>(QDataStream &in, InputEventCommand &command);

QDebug operator <<(QDebug debug, const InputEventCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InputEventCommand)
