// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        const QString &scope, const QString &continuationIndent);

    using VarLocation = QPair<QString, int>;
    using VarLocations = QList<VarLocation>;
    static QList<int> removeVarValues(
            ProFile *profile,
            QStringList *lines,
            const QStringList &values,
            const QStringList &vars,
            VarLocations *removedLocations = nullptr
            );

    static void addFiles(ProFile *profile, QStringList *lines, const QStringList &filePaths,
                         const QString &var, const QString &continuationIndent);
    static QStringList removeFiles(
            ProFile *profile,
            QStringList *lines,
            const QDir &proFileDir,
            const QStringList &filePaths,
            const QStringList &vars,
            VarLocations *removedLocations = nullptr);

private:
    static bool locateVarValues(const QString &device, const ushort *tokPtr, const ushort *tokPtrEnd,
                                const QString &scope, const QString &var, int *scopeStart, int *bestLine);
    static QString compileScope(const QString &device, const QString &scope);
};

} // namespace Internal
} // namespace QmakeProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmakeProjectManager::Internal::ProWriter::PutFlags)
