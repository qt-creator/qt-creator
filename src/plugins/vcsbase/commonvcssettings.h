/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMMONVCSSETTINGS_H
#define COMMONVCSSETTINGS_H

#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
class QDebug;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {

// Common VCS settings, message check script and user nick names.
class CommonVcsSettings
{
public:
    CommonVcsSettings();

    QString nickNameMailMap;
    QString nickNameFieldListFile;

    QString submitMessageCheckScript;

    // Executable run to graphically prompt for a SSH-password.
    QString sshPasswordPrompt;

    QString patchCommand;

    bool lineWrap;
    int lineWrapWidth;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);

    bool equals(const CommonVcsSettings &rhs) const;
};

inline bool operator==(const CommonVcsSettings &s1, const CommonVcsSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const CommonVcsSettings &s1, const CommonVcsSettings &s2) { return !s1.equals(s2); }

QDebug operator<<(QDebug, const CommonVcsSettings &);

} // namespace Internal
} // namespace VcsBase

#endif // COMMONVCSSETTINGS_H
