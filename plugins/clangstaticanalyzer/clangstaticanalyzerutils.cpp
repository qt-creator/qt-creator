/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerutils.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzersettings.h"

#include <utils/environment.h>

#include <QFileInfo>

static bool isFileExecutable(const QString &executablePath)
{
    if (executablePath.isEmpty())
        return false;

    const QFileInfo fileInfo(executablePath);
    return fileInfo.isFile() && fileInfo.isExecutable();
}

namespace ClangStaticAnalyzer {
namespace Internal {

QString clangExecutableFromSettings(bool *isValid)
{
    return clangExecutable(ClangStaticAnalyzerSettings::instance()->clangExecutable(), isValid);
}

QString clangExecutable(const QString &fileNameOrPath, bool *isValid)
{
    QString executable = fileNameOrPath;
    if (executable.isEmpty()) {
        *isValid = false;
        return executable;
    }

    if (!QFileInfo(executable).isAbsolute()) {
        const Utils::Environment &environment = Utils::Environment::systemEnvironment();
        const QString executableFromPath = environment.searchInPath(executable).toString();
        if (executableFromPath.isEmpty()) {
            *isValid = false;
            return executable;
        }
        executable = executableFromPath;
    }

    *isValid = isFileExecutable(executable);
    return executable;
}

QString createFullLocationString(const ClangStaticAnalyzer::Internal::Location &location)
{
    const QString filePath = location.filePath;
    const QString lineNumber = QString::number(location.line);
    return filePath + QLatin1Char(':') + lineNumber;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
