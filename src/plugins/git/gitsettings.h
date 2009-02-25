/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef GITSETTINGS_H
#define GITSETTINGS_H

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

// Todo: Add user name and password?
struct GitSettings
{
    GitSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    bool equals(const GitSettings &s) const;

    bool adoptPath;
    QString path;
    int logCount;
    int timeout;
};

inline bool operator==(const GitSettings &p1, const GitSettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const GitSettings &p1, const GitSettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace Git

#endif // GITSETTINGS_H
