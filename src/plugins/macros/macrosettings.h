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

#ifndef MACROSPLUGIN_MACROSETTINGS_H
#define MACROSPLUGIN_MACROSETTINGS_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>

class QSettings;

namespace Macros {
namespace Internal {

class MacroSettings
{
public:
    MacroSettings();

    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);

    bool equals(const MacroSettings &ms) const;

    QString defaultDirectory;
    QStringList directories;
    QMap<QString, QVariant> shortcutIds;
    bool showSaveDialog;
};

inline bool operator==(const MacroSettings &m1, const MacroSettings &m2) { return m1.equals(m2); }
inline bool operator!=(const MacroSettings &m1, const MacroSettings &m2) { return !m1.equals(m2); }

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROSETTINGS_H
