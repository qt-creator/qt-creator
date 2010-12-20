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

#ifndef MACROSPLUGIN_MACRO_H
#define MACROSPLUGIN_MACRO_H

#include <QList>
#include <QString>

#include "macroevent.h"
#include "macros_global.h"

namespace Macros {

class MACROS_EXPORT Macro
{
public:
    Macro();
    Macro(const Macro& other);
    ~Macro();
    Macro& operator=(const Macro& other);

    void load(QString fileName = QString::null);
    void loadHeader(const QString &fileName);
    void save(const QString &fileName);

    const QString &description() const;
    void setDescription(const QString &text);

    const QString &version() const;
    const QString &fileName() const;
    QString displayName() const;

    void append(const MacroEvent &event);
    const QList<MacroEvent> &events() const;

    void setShortcutId(int id);
    int shortcutId() const;

    bool isWritable() const;

private:
    class MacroPrivate;
    MacroPrivate* d;
};

} // namespace Macros

#endif // MACROSPLUGIN_MACRO_H
