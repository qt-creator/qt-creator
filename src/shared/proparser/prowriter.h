/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmake_global.h"
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDir;
class ProFile;
QT_END_NAMESPACE

namespace QmakeProjectManager {
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

    static void addFiles(ProFile *profile, QStringList *lines, const QStringList &filePaths, const QString &var);
    static QStringList removeFiles(ProFile *profile, QStringList *lines,
        const QDir &proFileDir, const QStringList &filePaths, const QStringList &vars);

private:
    static bool locateVarValues(const ushort *tokPtr, const ushort *tokPtrEnd,
                                const QString &scope, const QString &var, int *scopeStart, int *bestLine);
    static QString compileScope(const QString &scope);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ProWriter::PutFlags)

} // namespace Internal
} // namespace QmakeProjectManager
