// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprojectparameters.h"
#include <utils/codegeneration.h>

#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

// ----------- QtProjectParameters
QtProjectParameters::QtProjectParameters() = default;

FilePath QtProjectParameters::projectPath() const
{
    return path / fileName;
}

// Write out a QT module line.
static inline void writeQtModulesList(QTextStream &str,
                                      const QStringList &modules,
                                      char op ='+')
{
    if (const int size = modules.size()) {
        str << "QT       " << op << "= ";
        for (int i =0; i < size; ++i) {
            if (i)
                str << ' ';
            str << modules.at(i);
        }
        str << "\n\n";
    }
}

QString createMacro(const QString &name, const QString &suffix)
{
    QString rc = name.toUpper();
    const int extensionPosition = rc.indexOf(QLatin1Char('.'));
    if (extensionPosition != -1)
        rc.truncate(extensionPosition);
    rc += suffix;
    return Utils::fileNameToCppIdentifier(rc);
}

QString QtProjectParameters::libraryMacro(const QString &projectName)
{
     return createMacro(projectName, QLatin1String("_LIBRARY"));
}

} // namespace Internal
} // namespace QmakeProjectManager
