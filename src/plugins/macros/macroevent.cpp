/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "macroevent.h"

#include <QString>
#include <QVariant>
#include <QDataStream>

using namespace Macros;

/*!
    \class Macros::MacroEvent
    \brief Represents an event in a macro

    An event stores information so it can be replayed. An event can be:
    \list
    \o menu action
    \o key event on an editor
    \o find/replace usage
    \o ...
    \endlist

    The information are stored in a map of QVariants (using quint8 for keys).
*/

class MacroEvent::MacroEventPrivate
{
public:
    QByteArray id;
    QMap<quint8, QVariant> values;
};


// ---------- MacroEvent ------------

MacroEvent::MacroEvent():
    d(new MacroEventPrivate)
{
}

MacroEvent::MacroEvent(const MacroEvent &other):
    d(new MacroEventPrivate)
{
    d->id = other.d->id;
    d->values = other.d->values;
}

MacroEvent::~MacroEvent()
{
    delete d;
}

MacroEvent& MacroEvent::operator=(const MacroEvent &other)
{
    if (this == &other)
        return *this;
    d->id = other.d->id;
    d->values = other.d->values;
    return *this;
}

QVariant MacroEvent::value(quint8 id) const
{
    if (d->values.contains(id))
        return d->values.value(id);
    return QVariant();
}

void MacroEvent::setValue(quint8 id, const QVariant &value)
{
    d->values[id] = value;
}

void MacroEvent::load(QDataStream &stream)
{
    stream >> d->id;
    int count;
    stream >> count;
    quint8 id;
    QVariant value;
    for (int i = 0; i < count; ++i) {
        stream >> id;
        stream >> value;
        d->values[id] = value;
    }
}

void MacroEvent::save(QDataStream &stream) const
{
    stream << d->id;
    stream << d->values.count();
    QMapIterator<quint8, QVariant> i(d->values);
    while (i.hasNext()) {
        i.next();
        stream << i.key() << i.value();
    }
}

const QByteArray & MacroEvent::id() const
{
    return d->id;
}

void MacroEvent::setId(const char *id)
{
    d->id = id;
}

QMap<quint8, QVariant> MacroEvent::values() const
{
    return d->values;
}
