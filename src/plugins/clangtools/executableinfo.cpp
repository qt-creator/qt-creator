// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/datafromprocess.h>
#include <utils/environment.h>
#include <utils/process.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

using namespace Utils;

namespace ClangTools {
namespace Internal {

static void handleProcessError(const Process &p)
{
    Core::MessageManager::writeFlashing(p.exitMessage());
    Core::MessageManager::writeFlashing(QString::fromUtf8(p.allRawOutput()));
}

static QStringList queryClangTidyChecks(const FilePath &executable,
                                        const QString &checksArgument)
{
    QStringList arguments = QStringList("-list-checks");
    if (!checksArgument.isEmpty())
        arguments.prepend(checksArgument);

    // Expected output is (clang-tidy 8.0):
    //   Enabled checks:
    //       abseil-duration-comparison
    //       abseil-duration-division
    //       abseil-duration-factory-float
    //       ...
    static const auto parser = [](const QString &stdOut) -> std::optional<QStringList> {
        QString output = stdOut;
        QTextStream stream(&output);
        QString line = stream.readLine();
        if (!line.startsWith("Enabled checks:"))
            return {};
        QStringList checks;
        while (!stream.atEnd()) {
            const QString candidate = stream.readLine().trimmed();
            if (!candidate.isEmpty())
                checks << candidate;
        }
        return checks;
    };

    DataFromProcess<QStringList>::Parameters params({executable, arguments}, parser);
    params.environment.setupEnglishOutput();
    params.errorHandler = handleProcessError;
    if (const auto checks = DataFromProcess<QStringList>::getData(params))
        return *checks;
    return {};
}

static ClazyChecks querySupportedClazyChecks(const FilePath &executablePath)
{
    // Expected output is (clazy-standalone 1.6):
    //   {
    //       "available_categories" : ["readability", "qt4", "containers", ... ],
    //       "checks" : [
    //           {
    //               "name"  : "qt-keywords",
    //               "level" : -1,
    //               "fixits" : [ { "name" : "qt-keywords" } ]
    //           },
    //           ...
    //           {
    //               "name"  : "inefficient-qlist",
    //               "level" : -1,
    //               "categories" : ["containers", "performance"],
    //               "visits_decls" : true
    //           },
    //           ...
    //       ]
    //   }
    static const auto parser = [](const QString &jsonOutput) -> std::optional<ClazyChecks> {
        const QJsonDocument document = QJsonDocument::fromJson(jsonOutput.toUtf8());
        if (document.isNull())
            return {};
        const QJsonArray checksArray = document.object()["checks"].toArray();
        ClazyChecks infos;
        for (const QJsonValue &item: checksArray) {
            const QJsonObject checkObject = item.toObject();
            ClazyCheck info;
            info.name = checkObject["name"].toString().trimmed();
            if (info.name.isEmpty())
                continue;
            info.level = checkObject["level"].toInt();
            for (const QJsonValue &item : checkObject["categories"].toArray())
                info.topics.append(item.toString().trimmed());
            infos << info;
        }
        return infos;
    };

    static const QString queryFlag = "-supported-checks-json";
    DataFromProcess<ClazyChecks>::Parameters params(CommandLine(executablePath, {queryFlag}),
                                                    parser);
    params.environment.setupEnglishOutput();
    params.errorHandler = handleProcessError;
    auto checks = DataFromProcess<ClazyChecks>::getData(params);
    if (!checks) {
        // Some clazy 1.6.x versions have a bug where they expect an argument after the
        // option.
        params.commandLine = CommandLine(executablePath, {queryFlag, "dummy"});
        checks = DataFromProcess<ClazyChecks>::getData(params);
    }
    if (checks)
        return *checks;
    return {};
}

ClangTidyInfo::ClangTidyInfo(const FilePath &executablePath)
    : defaultChecks(queryClangTidyChecks(executablePath, {}))
    , supportedChecks(queryClangTidyChecks(executablePath, "-checks=*"))
{}

ClazyStandaloneInfo::ClazyStandaloneInfo(const FilePath &executablePath)
    : defaultChecks(queryClangTidyChecks(executablePath, {})) // Yup, behaves as clang-tidy.
    , supportedChecks(querySupportedClazyChecks(executablePath))
{
    static const auto parser = [](const QString &stdOut) -> std::optional<QVersionNumber> {
        QString output = stdOut;
        QTextStream stream(&output);
        while (!stream.atEnd()) {
            // It's just "clazy version " right now, but let's be prepared for someone
            // adding a colon later on.
            static const QStringList versionPrefixes{"clazy version ", "clazy version: "};
            const QString line = stream.readLine().simplified();
            for (const QString &prefix : versionPrefixes) {
                if (line.startsWith(prefix))
                    return QVersionNumber::fromString(line.mid(prefix.length()));
            }
        }
        return {};
    };
    DataFromProcess<QVersionNumber>::Parameters params({{executablePath, {"--version"}}, parser});
    params.environment.setupEnglishOutput();
    if (const auto v = DataFromProcess<QVersionNumber>::getData(params))
        version = *v;
}

static FilePath queryResourceDir(const FilePath &clangToolPath)
{
    // Expected output is (clang-tidy 10):
    //   lib/clang/10.0.1
    //   Error while trying to load a compilation database:
    //   ...
    const auto parser = [&clangToolPath](const QString &stdOut) -> std::optional<FilePath> {
        QString output = stdOut;
        QTextStream stream(&output);
        const QString path = clangToolPath.parentDir().parentDir()
                                 .pathAppended(stream.readLine()).toString();
        const auto filePath = FilePath::fromUserInput(QDir::cleanPath(path));
        if (filePath.exists())
            return filePath;
        return {};
    };

    DataFromProcess<FilePath>::Parameters params({clangToolPath,
                                                  {"someFilePath", "--", "-print-resource-dir"}},
                                                 parser);
    params.environment.setupEnglishOutput();
    params.allowedResults << ProcessResult::FinishedWithError;
    if (const auto filePath = DataFromProcess<FilePath>::getData(params))
        return *filePath;
    return {};
}

QString queryVersion(const FilePath &clangToolPath, QueryFailMode failMode)
{
    static const auto parser = [](const QString &stdOut) -> std::optional<QString> {
        QString output = stdOut;
        QTextStream stream(&output);
        while (!stream.atEnd()) {
            static const QStringList versionPrefixes{"LLVM version ", "clang version: "};
            const QString line = stream.readLine().simplified();
            for (const QString &prefix : versionPrefixes) {
                auto idx = line.indexOf(prefix);
                if (idx >= 0)
                    return line.mid(idx + prefix.length());
            }
        }
        return {};
    };
    DataFromProcess<QString>::Parameters params({clangToolPath, {"--version"}}, parser);
    params.environment.setupEnglishOutput();
    if (failMode == QueryFailMode::Noisy)
        params.errorHandler = handleProcessError;
    if (const auto version = DataFromProcess<QString>::getData(params))
        return *version;
    return {};
}

static QPair<FilePath, QString> clangIncludeDirAndVersion(const FilePath &clangToolPath)
{
    const FilePath dynamicResourceDir = queryResourceDir(clangToolPath);
    const QString dynamicVersion = queryVersion(clangToolPath, QueryFailMode::Noisy);
    if (dynamicResourceDir.isEmpty() || dynamicVersion.isEmpty())
        return {FilePath::fromUserInput(CLANG_INCLUDE_DIR), QString(CLANG_VERSION)};
    return {dynamicResourceDir / "include", dynamicVersion};
}

QPair<FilePath, QString> getClangIncludeDirAndVersion(const FilePath &clangToolPath)
{
    QTC_CHECK(!clangToolPath.isEmpty());
    static QMap<FilePath, QPair<FilePath, QString>> cache;
    auto it = cache.find(clangToolPath);
    if (it == cache.end())
        it = cache.insert(clangToolPath, clangIncludeDirAndVersion(clangToolPath));
    return it.value();
}

} // namespace Internal
} // namespace ClangTools
