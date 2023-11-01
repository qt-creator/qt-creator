// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsutils.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolssettings.h"
#include "clangtoolstr.h"

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpptoolsreuse.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static QString lineColumnString(const Debugger::DiagnosticLocation &location)
{
    return QString("%1:%2").arg(QString::number(location.line), QString::number(location.column));
}

static QString fixitStatus(FixitStatus status)
{
    switch (status) {
    case FixitStatus::NotAvailable:
        return Tr::tr("No Fixits");
    case FixitStatus::NotScheduled:
        return Tr::tr("Not Scheduled");
    case FixitStatus::Invalidated:
        return Tr::tr("Invalidated");
    case FixitStatus::Scheduled:
        return Tr::tr("Scheduled");
    case FixitStatus::FailedToApply:
        return Tr::tr("Failed to Apply");
    case FixitStatus::Applied:
        return Tr::tr("Applied");
    }
    return QString();
}

QString createDiagnosticToolTipString(
    const Diagnostic &diagnostic,
    std::optional<FixitStatus> status,
    bool showSteps)
{
    using StringPair = QPair<QString, QString>;
    QList<StringPair> lines;

    if (!diagnostic.category.isEmpty()) {
        lines.push_back({Tr::tr("Category:"),
                         diagnostic.category.toHtmlEscaped()});
    }

    if (!diagnostic.type.isEmpty()) {
        lines.push_back({Tr::tr("Type:"),
                         diagnostic.type.toHtmlEscaped()});
    }

    if (!diagnostic.description.isEmpty()) {
        lines.push_back({Tr::tr("Description:"),
                         diagnostic.description.toHtmlEscaped()});
    }

    lines.push_back({Tr::tr("Location:"),
                     createFullLocationString(diagnostic.location)});

    if (status) {
        lines.push_back({Tr::tr("Fixit status:"),
                         fixitStatus(*status)});
    }

    if (showSteps && !diagnostic.explainingSteps.isEmpty()) {
        StringPair steps;
        steps.first = Tr::tr("Steps:");
        for (const ExplainingStep &step : diagnostic.explainingSteps) {
            if (!steps.second.isEmpty())
                steps.second += "<br>";
            steps.second += QString("%1:%2: %3")
                    .arg(step.location.filePath.toUserOutput(),
                         lineColumnString(step.location),
                         step.message);
        }
        lines << steps;
    }

    const QString url = documentationUrl(diagnostic.name);
    if (!url.isEmpty()) {
        lines.push_back({Tr::tr("Documentation:"),
                         QString("<a href=\"%1\">%1</a>").arg(url)});
    }

    QString html = QLatin1String("<html>"
                                 "<head>"
                                 "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>"
                                 "</head>\n"
                                 "<body><dl>");

    for (const StringPair &pair : std::as_const(lines)) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += QLatin1String("</dl></body></html>");
    return html;
}

QString createFullLocationString(const Debugger::DiagnosticLocation &location)
{
    return location.filePath.toUserOutput() + QLatin1Char(':') + QString::number(location.line)
            + QLatin1Char(':') + QString::number(location.column);
}

QString hintAboutBuildBeforeAnalysis()
{
    return Tr::tr(
        "In general, the project should be built before starting the analysis to ensure that the "
        "code to analyze is valid.<br/><br/>"
        "Building the project might also run code generators that update the source files as "
        "necessary.");
}

void showHintAboutBuildBeforeAnalysis()
{
    Utils::CheckableMessageBox::information(Core::ICore::dialogParent(),
                                            Tr::tr("Info About Build the Project Before Analysis"),
                                            hintAboutBuildBeforeAnalysis(),
                                            Key("ClangToolsDisablingBuildBeforeAnalysisHint"));
}

FilePath fullPath(const FilePath &executable)
{
    FilePath candidate = executable;
    const bool hasSuffix = candidate.endsWith(QTC_HOST_EXE_SUFFIX);

    if (candidate.isAbsolutePath()) {
        if (!hasSuffix)
            candidate = candidate.withExecutableSuffix();
    } else {
        const Environment environment = Environment::systemEnvironment();
        const FilePath expandedPath = environment.searchInPath(candidate.toString());
        if (!expandedPath.isEmpty())
            candidate = expandedPath;
    }

    return candidate;
}

static FilePath findValidExecutable(const FilePaths &candidates)
{
    for (const FilePath &candidate : candidates) {
        const FilePath expandedPath = fullPath(candidate);
        if (expandedPath.isExecutableFile())
            return expandedPath;
    }

    return {};
}

FilePath toolShippedExecutable(ClangToolType tool)
{
    const FilePath shippedExecutable = tool == ClangToolType::Tidy
                                     ? Core::ICore::clangTidyExecutable(CLANG_BINDIR)
                                     : Core::ICore::clazyStandaloneExecutable(CLANG_BINDIR);
    if (shippedExecutable.isExecutableFile())
        return shippedExecutable;
    return {};
}

FilePath toolExecutable(ClangToolType tool)
{
    const FilePath fromSettings = ClangToolsSettings::instance()->executable(tool);
    if (!fromSettings.isEmpty())
        return fullPath(fromSettings);
    return toolFallbackExecutable(tool);
}

FilePath toolFallbackExecutable(ClangToolType tool)
{
    const FilePath fallback = tool == ClangToolType::Tidy
                            ? FilePath(Constants::CLANG_TIDY_EXECUTABLE_NAME)
                            : FilePath(Constants::CLAZY_STANDALONE_EXECUTABLE_NAME);
    return findValidExecutable({toolShippedExecutable(tool), fallback});
}

QString clangToolName(CppEditor::ClangToolType tool)
{
    return tool == ClangToolType::Tidy ? Tr::tr("Clang-Tidy") : Tr::tr("Clazy");
}

bool isVFSOverlaySupported(const FilePath &executable)
{
    static QMap<FilePath, bool> vfsCapabilities;
    auto it = vfsCapabilities.find(executable);
    if (it == vfsCapabilities.end()) {
        Process p;
        p.setCommand({executable, {"--help"}});
        p.runBlocking();
        it = vfsCapabilities.insert(executable, p.allOutput().contains("vfsoverlay"));
    }
    return it.value();
}

static void addBuiltinConfigs(ClangDiagnosticConfigsModel &model)
{
    model.appendOrUpdate(builtinConfig());
}

ClangDiagnosticConfig builtinConfig()
{
    ClangDiagnosticConfig config;
    config.setId(Constants::DIAG_CONFIG_TIDY_AND_CLAZY);
    config.setDisplayName(Tr::tr("Default Clang-Tidy and Clazy checks"));
    config.setIsReadOnly(true);
    config.setClangOptions({"-w"}); // Do not emit any clang-only warnings
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseDefaultChecks);
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseDefaultChecks);
    return config;
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

QString documentationUrl(const QString &checkName)
{
    QString name = checkName;
    const QString clangPrefix = "clang-diagnostic-";
    if (name.startsWith(clangPrefix))
        return {}; // No documentation for this.

    QString url;
    const QString clazyPrefix = "clazy-";
    const QString clangStaticAnalyzerPrefix = "clang-analyzer-core.";
    if (name.startsWith(clazyPrefix)) {
        name = checkName.mid(clazyPrefix.length());
        url = clazyDocUrl(name);
    } else if (name.startsWith(clangStaticAnalyzerPrefix)) {
        url = CppEditor::Constants::CLANG_STATIC_ANALYZER_DOCUMENTATION_URL;
    } else {
        url = clangTidyDocUrl(name);
    }

    return url;
}

ClangDiagnosticConfig diagnosticConfig(const Utils::Id &diagConfigId)
{
    const ClangDiagnosticConfigsModel configs = diagnosticConfigsModel();
    QTC_ASSERT(configs.hasConfigWithId(diagConfigId), return ClangDiagnosticConfig());
    return configs.configWithId(diagConfigId);
}

static QStringList extraOptions(const QString &envVar)
{
    if (!qtcEnvironmentVariableIsSet(envVar))
        return {};
    QString arguments = qtcEnvironmentVariable(envVar);
    return ProcessArgs::splitArgs(arguments, HostOsInfo::hostOs());
}

QStringList extraClangToolsPrependOptions()
{
    constexpr char csaPrependOptions[] = "QTC_CLANG_CSA_CMD_PREPEND";
    constexpr char toolsPrependOptions[] = "QTC_CLANG_TOOLS_CMD_PREPEND";
    static const QStringList options = extraOptions(csaPrependOptions)
                                       + extraOptions(toolsPrependOptions);
    if (!options.isEmpty())
        qWarning() << "ClangTools options are prepended with " << options;
    return options;
}

QStringList extraClangToolsAppendOptions()
{
    constexpr char csaAppendOptions[] = "QTC_CLANG_CSA_CMD_APPEND";
    constexpr char toolsAppendOptions[] = "QTC_CLANG_TOOLS_CMD_APPEND";
    static const QStringList options = extraOptions(csaAppendOptions)
                                       + extraOptions(toolsAppendOptions);
    if (!options.isEmpty())
        qWarning() << "ClangTools options are appended with " << options;
    return options;
}

static QVersionNumber fixupVersion(const VersionAndSuffix &versionAndSuffix)
{
    // llvm.org only does document releases for the first released version
    QVersionNumber version = QVersionNumber(versionAndSuffix.first.majorVersion(), 0, 0);

    if (version == QVersionNumber(0))
        version = QVersionNumber(12);

    // Version 17.0.0 was never released due to a git issue
    if (version.majorVersion() == 17)
        version = QVersionNumber(17, 0, 1);

    return version;
}

QString clangTidyDocUrl(const QString &check)
{
    VersionAndSuffix version = ClangToolsSettings::clangTidyVersion();
    version.first = fixupVersion(version);
    static const char versionedUrlPrefix[]
            = "https://releases.llvm.org/%1/tools/clang/tools/extra/docs/";
    static const char unversionedUrlPrefix[] = "https://clang.llvm.org/extra/";
    QString url = version.second.contains("git")
            ? QString::fromLatin1(unversionedUrlPrefix)
            : QString::fromLatin1(versionedUrlPrefix).arg(version.first.toString());
    url.append("clang-tidy/checks/");
    if (version.first.majorVersion() < 15) {
        url.append(check);
    } else {
        const int hyphenIndex = check.indexOf('-');
        QTC_ASSERT(hyphenIndex != -1, return {});
        url.append(check.left(hyphenIndex)).append('/').append(check.mid(hyphenIndex + 1));
    }
    return url.append(".html");
}

QString clazyDocUrl(const QString &check)
{
    QVersionNumber version = ClangToolsSettings::clazyVersion();
    if (!version.isNull())
        version = QVersionNumber(version.majorVersion(), version.minorVersion());
    const QString versionString = version.isNull() ? "master" : version.toString();
    static const char urlTemplate[]
            = "https://github.com/KDE/clazy/blob/%1/docs/checks/README-%2.md";
    return QString::fromLatin1(urlTemplate).arg(versionString, check);
}

bool toolEnabled(CppEditor::ClangToolType type, const ClangDiagnosticConfig &config,
                 const RunSettings &runSettings)
{
    if (type == ClangToolType::Tidy && runSettings.preferConfigFile())
        return true;
    return config.isEnabled(type);
}

} // namespace Internal
} // namespace ClangTools
