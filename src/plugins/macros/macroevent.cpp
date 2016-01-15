/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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
    m_id = Core::Id::fromName(ba);
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
    stream << m_values.count();
    QMapIterator<quint8, QVariant> i(m_values);
    while (i.hasNext()) {
        i.next();
        stream << i.key() << i.value();
    }
}

Core::Id MacroEvent::id() const
{
    return m_id;
}

void MacroEvent::setId(Core::Id id)
{
    m_id = id;
}

} // namespace Internal
} // namespace Macro
