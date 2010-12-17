/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROWRITER_H
#define PROWRITER_H

#include "namespace_global.h"

#include <QStringList>

QT_BEGIN_NAMESPACE
class QDir;
class ProFile;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ProWriter
{
public:
    static void addFiles(ProFile *profile, QStringList *lines,
         const QDir &proFileDir, const QStringList &filePaths, const QString &var)
    {
        addVarValues(profile, lines, proFileDir, filePaths, var, true);
    }

    static QStringList removeFiles(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &filePaths,
        const QStringList &vars)
    {
        return removeVarValues(profile, lines, proFileDir, filePaths, vars, true);
    }

    static void addVarValues(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &values, const QString &var)
    {
        addVarValues(profile, lines, proFileDir, values, var, false);
    }

    static QStringList removeVarValues(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &values,
        const QStringList &vars)
    {
        return removeVarValues(profile, lines, proFileDir, values, vars, false);
    }

private:

    static void addVarValues(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &values, const QString &var,
        bool valuesAreFiles);

    static QStringList removeVarValues(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &values,
        const QStringList &vars, bool valuesAreFiles);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROWRITER_H
