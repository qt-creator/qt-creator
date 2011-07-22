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

#ifndef QSYSTEM_H
#define QSYSTEM_H

#include <QString>

class QSystem
{
public:
    static QString which(const QString &path, const QString &applicationName);
    static bool hasEnvKey(const QString &envString, QString &key, int &pos, int start = 0);
    static bool unsetEnvKey(QStringList *list, const QString &key);
    static bool setEnvKey(QStringList *list, const QString &key, const QString &value);
    static QString envKey(QStringList *list, const QString &key);
    static bool processEnvValue(QStringList *list, QString &envString);
    static void addEnvPath(QStringList *environment, const QString &key, const QString &addedPath);

    static QString userName();
    static QString hostName();
    static QString OSName();
};

#endif
