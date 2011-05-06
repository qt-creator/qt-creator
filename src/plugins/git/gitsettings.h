/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
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

    /** Return the full path to the git executable */
    QString gitBinaryPath(bool *ok = 0, QString *errorMessage = 0) const;

    bool equals(const GitSettings &s) const;

    bool adoptPath;
    QString path;
    int logCount;
    int timeoutSeconds;
    bool pullRebase;
    bool promptToSubmit;
    bool omitAnnotationDate;
    bool ignoreSpaceChangesInDiff;
    bool ignoreSpaceChangesInBlame;
    bool diffPatience;
    bool winSetHomeEnvironment;
    int showPrettyFormat;
    QString gitkOptions;
};

inline bool operator==(const GitSettings &p1, const GitSettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const GitSettings &p1, const GitSettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace Git

#endif // GITSETTINGS_H
