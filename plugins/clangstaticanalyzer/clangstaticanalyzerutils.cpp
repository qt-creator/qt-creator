/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "clangstaticanalyzerutils.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzersettings.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/environment.h>

#include <QCoreApplication>
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

QString clangExecutableFromSettings(Core::Id toolchainType, bool *isValid)
{
    QString exeFromSettings = ClangStaticAnalyzerSettings::instance()->clangExecutable();
    if (toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_ID)
        exeFromSettings.replace(QLatin1String("clang.exe"), QLatin1String("clang-cl.exe"));
    return clangExecutable(exeFromSettings, isValid);
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

    *isValid = isFileExecutable(executable) && isClangExecutableUsable(executable);
    return executable;
}

QString createFullLocationString(const ClangStaticAnalyzer::Internal::Location &location)
{
    const QString filePath = location.filePath;
    const QString lineNumber = QString::number(location.line);
    return filePath + QLatin1Char(':') + lineNumber;
}

bool isClangExecutableUsable(const QString &filePath, QString *errorMessage)
{
    const QFileInfo fi(filePath);
    if (fi.isSymLink() && fi.symLinkTarget().contains(QLatin1String("icecc"))) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("ClangStaticAnalyzer",
                    "The chosen file \"%1\" seems to point to an icecc binary not suitable "
                    "for analyzing.\nPlease set a real clang executable.")
                    .arg(filePath);
        }
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
