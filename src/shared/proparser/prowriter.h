/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROWRITER_H
#define PROWRITER_H

#include "qmake_global.h"
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDir;
class ProFile;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class QMAKE_EXPORT ProWriter
{
public:
    enum PutFlag {
        AppendValues = 0,
        ReplaceValues = 1,
        OneLine = 0, // this works only when replacing (or adding a new assignment)
        MultiLine = 2,
        AssignOperator = 0, // ignored when changing an existing assignment
        AppendOperator = 4
    };
    Q_DECLARE_FLAGS(PutFlags, PutFlag)

    static void putVarValues(ProFile *profile, QStringList *lines,
        const QStringList &values, const QString &var, PutFlags flags,
        const QString &scope = QString());
    static QList<int> removeVarValues(ProFile *profile, QStringList *lines,
        const QStringList &values, const QStringList &vars);

    static void addFiles(ProFile *profile, QStringList *lines,
         const QDir &proFileDir, const QStringList &filePaths, const QString &var);
    static QStringList removeFiles(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &filePaths, const QStringList &vars);

private:
    static bool locateVarValues(const ushort *tokPtr,
        const QString &scope, const QString &var, int *scopeStart, int *bestLine);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ProWriter::PutFlags)

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROWRITER_H
