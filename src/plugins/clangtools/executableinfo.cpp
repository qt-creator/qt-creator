/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "executableinfo.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/environment.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

using namespace Utils;

namespace ClangTools {
namespace Internal {

enum class FailSilently { Yes, No };
static QString runExecutable(const Utils::CommandLine &commandLine,
                             FailSilently failSilently = FailSilently::No)
{
    if (commandLine.executable().isEmpty() || !commandLine.executable().toFileInfo().isExecutable())
        return {};

    SynchronousProcess cpp;
    Utils::Environment env = Environment::systemEnvironment();
    Environment::setupEnglishOutput(&env);
    cpp.setEnvironment(env.toStringList());

    const SynchronousProcessResponse response = cpp.runBlocking(commandLine);
    if (response.result != SynchronousProcessResponse::Finished
            && (failSilently == FailSilently::No
                || response.result != SynchronousProcessResponse::FinishedError)) {
        Core::MessageManager::write(response.exitMessage(commandLine.toUserOutput(), 10));
        Core::MessageManager::write(QString::fromUtf8(response.allRawOutput()));
        return {};
    }

    return response.stdOut();
}

static QStringList queryClangTidyChecks(const QString &executable,
                                        const QString &checksArgument)
{
    QStringList arguments = QStringList("-list-checks");
    if (!checksArgument.isEmpty())
        arguments.prepend(checksArgument);

    const CommandLine commandLine(executable, arguments);
    QString output = runExecutable(commandLine);
    if (output.isEmpty())
        return {};

    // Expected output is (clang-tidy 8.0):
    //   Enabled checks:
    //       abseil-duration-comparison
    //       abseil-duration-division
    //       abseil-duration-factory-float
    //       ...

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
}

static ClazyChecks querySupportedClazyChecks(const QString &executablePath)
{
    static const QString queryFlag = "-supported-checks-json";
    QString jsonOutput = runExecutable(CommandLine(executablePath, {queryFlag}));

    // Some clazy 1.6.x versions have a bug where they expect an argument after the
    // option.
    if (jsonOutput.isEmpty())
        jsonOutput = runExecutable(CommandLine(executablePath, {queryFlag, "dummy"}));
    if (jsonOutput.isEmpty())
        return {};

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

    ClazyChecks infos;

    const QJsonDocument document = QJsonDocument::fromJson(jsonOutput.toUtf8());
    if (document.isNull())
        return {};
    const QJsonArray checksArray = document.object()["checks"].toArray();

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
}

ClangTidyInfo::ClangTidyInfo(const QString &executablePath)
    : defaultChecks(queryClangTidyChecks(executablePath, {}))
    , supportedChecks(queryClangTidyChecks(executablePath, "-checks=*"))
{}

ClazyStandaloneInfo::ClazyStandaloneInfo(const QString &executablePath)
    : defaultChecks(queryClangTidyChecks(executablePath, {})) // Yup, behaves as clang-tidy.
    , supportedChecks(querySupportedClazyChecks(executablePath))
{}

static FilePath queryResourceDir(const FilePath &clangToolPath)
{
    QString output = runExecutable(CommandLine(clangToolPath, {"someFilePath", "--",
                                                               "-print-resource-dir"}),
                                   FailSilently::Yes);

    // Expected output is (clang-tidy 10):
    //   lib/clang/10.0.1
    //   Error while trying to load a compilation database:
    //   ...

    // Parse
    QTextStream stream(&output);
    const QString path = clangToolPath.parentDir().parentDir()
            .pathAppended(stream.readLine()).toString();
    const auto filePath = FilePath::fromUserInput(QDir::cleanPath(path));
    if (filePath.exists())
        return filePath;
    return {};
}

static QString queryVersion(const FilePath &clangToolPath)
{
    QString output = runExecutable(CommandLine(clangToolPath, {"--version"}));
    QTextStream stream(&output);
    while (!stream.atEnd()) {
        static const QStringList versionPrefixes{"LLVM version ", "clang version: "};
        const QString line = stream.readLine().simplified();
        for (const QString &prefix : versionPrefixes) {
            if (line.startsWith(prefix))
                return line.mid(prefix.length());
        }
    }
    return {};
}

QPair<FilePath, QString> getClangResourceDirAndVersion(const FilePath &clangToolPath)
{
    const FilePath dynamicResourceDir = queryResourceDir(clangToolPath);
    const QString dynamicVersion = queryVersion(clangToolPath);
    if (dynamicResourceDir.isEmpty() || dynamicVersion.isEmpty())
        return qMakePair(FilePath::fromString(CLANG_RESOURCE_DIR), QString(CLANG_VERSION));
    return qMakePair(dynamicResourceDir, dynamicVersion);
}

} // namespace Internal
} // namespace ClangTools
