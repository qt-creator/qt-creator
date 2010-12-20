/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROEVENT_H
#define MACROSPLUGIN_MACROEVENT_H

#include "macros_global.h"

#include <QString>
#include <QVariant>
#include <QDataStream>

namespace Macros {

class MACROS_EXPORT MacroEvent
{
public:
    MacroEvent();
    MacroEvent(const MacroEvent &other);
    virtual ~MacroEvent();
    MacroEvent& operator=(const MacroEvent &other);

    const QByteArray &id() const;
    void setId(const char *id);

    QVariant value(quint8 id) const;
    void setValue(quint8 id, const QVariant &value);

    QMap<quint8, QVariant> values() const;

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

private:
    class MacroEventPrivate;
    MacroEventPrivate* d;
};

} // namespace Macros

#endif // MACROSPLUGIN_MACROEVENT_H
