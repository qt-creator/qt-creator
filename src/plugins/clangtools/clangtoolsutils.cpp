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

#include "clangtoolsutils.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolssettings.h"

#include <coreplugin/icore.h>
#include <cpptools/cpptoolsreuse.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#include <QFileInfo>

#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/clangdiagnosticconfigsmodel.h>

using namespace CppTools;

namespace ClangTools {
namespace Internal {

QString createFullLocationString(const Debugger::DiagnosticLocation &location)
{
    return location.filePath + QLatin1Char(':') + QString::number(location.line)
            + QLatin1Char(':') + QString::number(location.column);
}

QString hintAboutBuildBeforeAnalysis()
{
    return ClangTool::tr(
        "In general, the project should be built before starting the analysis to ensure that the "
        "code to analyze is valid.<br/><br/>"
        "Building the project might also run code generators that update the source files as "
        "necessary.");
}

void showHintAboutBuildBeforeAnalysis()
{
    Utils::CheckableMessageBox::doNotShowAgainInformation(
        Core::ICore::dialogParent(),
        ClangTool::tr("Info About Build the Project Before Analysis"),
        hintAboutBuildBeforeAnalysis(),
        Core::ICore::settings(),
        "ClangToolsDisablingBuildBeforeAnalysisHint");
}

bool isFileExecutable(const QString &filePath)
{
    if (filePath.isEmpty())
        return false;

    const QFileInfo fileInfo(filePath);
    return fileInfo.isFile() && fileInfo.isExecutable();
}

QString shippedClangTidyExecutable()
{
    const QString shippedExecutable = Core::ICore::clangTidyExecutable(CLANG_BINDIR);
    if (isFileExecutable(shippedExecutable))
        return shippedExecutable;
    return {};
}

QString shippedClazyStandaloneExecutable()
{
    const QString shippedExecutable = Core::ICore::clazyStandaloneExecutable(CLANG_BINDIR);
    if (isFileExecutable(shippedExecutable))
        return shippedExecutable;
    return {};
}

static QString fullPath(const QString &executable)
{
    const QString hostExeSuffix = QLatin1String(QTC_HOST_EXE_SUFFIX);
    const Qt::CaseSensitivity caseSensitivity = Utils::HostOsInfo::fileNameCaseSensitivity();

    QString candidate = executable;
    const bool hasSuffix = candidate.endsWith(hostExeSuffix, caseSensitivity);

    const QFileInfo fileInfo = QFileInfo(candidate);
    if (fileInfo.isAbsolute()) {
        if (!hasSuffix)
            candidate.append(hostExeSuffix);
    } else {
        const Utils::Environment environment = Utils::Environment::systemEnvironment();
        const QString expandedPath = environment.searchInPath(candidate).toString();
        if (!expandedPath.isEmpty())
            candidate = expandedPath;
    }

    return candidate;
}

static QString findValidExecutable(const QStringList &candidates)
{
    for (QString candidate : candidates) {
        const QString expandedPath = fullPath(candidate);
        if (isFileExecutable(expandedPath))
            return expandedPath;
    }

    return {};
}

QString clangTidyExecutable()
{
    const QString fromSettings = ClangToolsSettings::instance()->clangTidyExecutable();
    if (!fromSettings.isEmpty())
        return fullPath(fromSettings);

    return findValidExecutable({
        shippedClangTidyExecutable(),
        Constants::CLANG_TIDY_EXECUTABLE_NAME,
    });
}

QString clazyStandaloneExecutable()
{
    const QString fromSettings = ClangToolsSettings::instance()->clazyStandaloneExecutable();
    if (!fromSettings.isEmpty())
        return fullPath(fromSettings);

    return findValidExecutable({
        shippedClazyStandaloneExecutable(),
        qEnvironmentVariable("QTC_USE_CLAZY_STANDALONE_PATH"),
        Constants::CLAZY_STANDALONE_EXECUTABLE_NAME,
                               });
}

constexpr const char *DEFAULT_TIDY_CHECKS = "-*,"
                                            "bugprone-*,"
                                            "cppcoreguidelines-*,"
                                            "misc-*,"
                                            "modernize-*,"
                                            "performance-*,"
                                            "readability-*,"
                                            "-cppcoreguidelines-owning-memory,"
                                            "-readability-braces-around-statements,"
                                            "-readability-implicit-bool-conversion,"
                                            "-readability-named-parameter";

static void addBuiltinConfigs(ClangDiagnosticConfigsModel &model)
{
    // Clang-Tidy
    ClangDiagnosticConfig config;
    config.setId("Builtin.Tidy");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clang-Tidy thorough checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);
    config.setClangTidyChecks(QString::fromUtf8(DEFAULT_TIDY_CHECKS));
    model.appendOrUpdate(config);

    // Clang static analyzer
    config = ClangDiagnosticConfig();
    config.setId("Builtin.TidyClangAnalyze");
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Clang-Tidy static analyzer checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{
        QStringLiteral("-w"),
    });
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);
    config.setClangTidyChecks("-*,clang-analyzer-*");
    model.appendOrUpdate(config);

    // Clazy
    config = ClangDiagnosticConfig();
    config.setId("Builtin.Clazy");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clazy level0 checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClazyChecks(CppTools::clazyChecksForLevel(0));
    model.appendOrUpdate(config);

    // Clang-Tidy and Clazy
    config = ClangDiagnosticConfig();
    config.setId(Constants::DIAG_CONFIG_TIDY_AND_CLAZY);
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clang-Tidy and Clazy preselected checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);
    config.setClangTidyChecks(QString::fromUtf8(DEFAULT_TIDY_CHECKS));
    config.setClazyChecks(clazyChecksForLevel(0));
    model.appendOrUpdate(config);
}

ClangDiagnosticConfigsModel diagnosticConfigsModel(const ClangDiagnosticConfigs &customConfigs)
{
    ClangDiagnosticConfigsModel model;
    addBuiltinConfigs(model);
    for (const ClangDiagnosticConfig &config : customConfigs)
        model.appendOrUpdate(config);
    return model;
}

ClangDiagnosticConfigsModel diagnosticConfigsModel()
{
    return Internal::diagnosticConfigsModel(ClangToolsSettings::instance()->diagnosticConfigs());
}

} // namespace Internal
} // namespace ClangTools
