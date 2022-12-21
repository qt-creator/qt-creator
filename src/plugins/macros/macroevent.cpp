// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macroevent.h"

#include <QString>
#include <QVariant>
#include <QDataStream>

namespace Macros {
namespace Internal {

/*!
    \class Macros::MacroEvent
    \brief The MacroEvent class represents an event in a macro.

    An event stores information so it can be replayed. An event can be:
    \list
    \li menu action
    \li key event on an editor
    \li find/replace usage
    \endlist

    The information are stored in a map of QVariants (using quint8 for keys).
*/

QVariant MacroEvent::value(quint8 id) const
{
    return m_values.value(id);
}

void MacroEvent::setValue(quint8 id, const QVariant &value)
{
    m_values[id] = value;
}

void MacroEvent::load(QDataStream &stream)
{
    QByteArray ba;
    stream >> ba;
    m_id = Utils::Id::fromName(ba);
    int count;
    stream >> count;
    quint8 id;
    QVariant value;
    for (int i = 0; i < count; ++i) {
        stream >> id;
        stream >> value;
        m_values[id] = value;
    }
}

void MacroEvent::save(QDataStream &stream) const
{
    stream << m_id.name();
    stream << int(m_values.count());
    for (auto i = m_values.cbegin(), end = m_values.cend(); i != end; ++i)
        stream << i.key() << i.value();
}

Utils::Id MacroEvent::id() const
{
    return m_id;
}

void MacroEvent::setId(Utils::Id id)
{
    m_id = id;
}

} // namespace Internal
} // namespace Macro
