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

#include "clangstaticanalyzerutils.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzersettings.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
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
    QString executable = ClangStaticAnalyzerSettings::instance()->clangExecutable();
    if (executable.isEmpty()) {
        *isValid = false;
        return executable;
    }

    const QString hostExeSuffix = QLatin1String(QTC_HOST_EXE_SUFFIX);
    const Qt::CaseSensitivity caseSensitivity = Utils::HostOsInfo::fileNameCaseSensitivity();
    const bool hasSuffix = executable.endsWith(hostExeSuffix, caseSensitivity);

    if (toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        if (hasSuffix)
            executable.chop(hostExeSuffix.length());
        executable.append(QLatin1String("-cl"));
        if (hasSuffix)
            executable.append(hostExeSuffix);
    }

    const QFileInfo fileInfo = QFileInfo(executable);
    if (fileInfo.isAbsolute()) {
        if (!hasSuffix)
            executable.append(hostExeSuffix);
    } else {
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

QString createFullLocationString(const Debugger::DiagnosticLocation &location)
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
                    "for analyzing.\nPlease set a real Clang executable.")
                    .arg(filePath);
        }
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
