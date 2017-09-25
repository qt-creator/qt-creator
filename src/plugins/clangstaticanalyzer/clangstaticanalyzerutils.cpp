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
#include <utils/synchronousprocess.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>

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
    QString executable = ClangStaticAnalyzerSettings::instance()->clangExecutable();
    if (executable.isEmpty()) {
        *isValid = false;
        return executable;
    }

    const QString hostExeSuffix = QLatin1String(QTC_HOST_EXE_SUFFIX);
    const Qt::CaseSensitivity caseSensitivity = Utils::HostOsInfo::fileNameCaseSensitivity();
    const bool hasSuffix = executable.endsWith(hostExeSuffix, caseSensitivity);

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

ClangExecutableVersion clangExecutableVersion(const QString &executable)
{
    const ClangExecutableVersion invalidVersion;

    // Sanity checks
    const QFileInfo fileInfo(executable);
    const bool isExecutableFile = fileInfo.isFile() && fileInfo.isExecutable();
    if (!isExecutableFile)
        return invalidVersion;

    // Get version output
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    Utils::Environment::setupEnglishOutput(&environment);
    Utils::SynchronousProcess runner;
    runner.setEnvironment(environment.toStringList());
    runner.setTimeoutS(10);
    // We would prefer "-dumpversion", but that one is only there for GCC compatibility
    // and returns some static/old version.
    // See also https://bugs.llvm.org/show_bug.cgi?id=28597
    const QStringList arguments(QLatin1String(("--version")));
    const Utils::SynchronousProcessResponse response = runner.runBlocking(executable, arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return invalidVersion;
    const QString output = response.stdOut();

    // Parse version output
    const QRegularExpression re(QLatin1String("clang version (\\d+)\\.(\\d+)\\.(\\d+)"));
    const QRegularExpressionMatch reMatch = re.match(output);
    if (re.captureCount() != 3)
        return invalidVersion;

    const QString majorString = reMatch.captured(1);
    bool convertedSuccessfully = false;
    const int major = majorString.toInt(&convertedSuccessfully);
    if (!convertedSuccessfully)
        return invalidVersion;

    const QString minorString = reMatch.captured(2);
    const int minor = minorString.toInt(&convertedSuccessfully);
    if (!convertedSuccessfully)
        return invalidVersion;

    const QString patchString = reMatch.captured(3);
    const int patch = patchString.toInt(&convertedSuccessfully);
    if (!convertedSuccessfully)
        return invalidVersion;

    return ClangExecutableVersion(major, minor, patch);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
