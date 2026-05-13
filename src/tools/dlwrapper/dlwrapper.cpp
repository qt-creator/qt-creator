// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/commandline.h>
#include <utils/filestreamer.h>
#include <utils/temporaryfile.h>
#include <utils/unarchiver.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QSocketNotifier>

#include <QtTaskTree/QConditional>
#include <QtTaskTree/qprocesstask.h>
#include <QtTaskTree/qtasktree.h>

using namespace QtTaskTree;
using namespace Utils;

Q_LOGGING_CATEGORY(dlWrapper, "qtc.dlwrapper", QtWarningMsg)

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("Qt Creator Download Wrapper");
    a.setOrganizationName("Qt");

    QCommandLineParser commandLineParser;
    commandLineParser.setApplicationDescription("Downloads and executes a command from an archive.");
    commandLineParser.addHelpOption();
    commandLineParser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsPositionalArguments);
    commandLineParser.addOption(
        QCommandLineOption("env", "Environment Variable values to set", "env"));
    commandLineParser.addOption(QCommandLineOption(
        "download", "The archive to download the command to run from.", "download"));
    commandLineParser
        .addPositionalArgument("executable", "The executable relative to the extracted archive root");

    commandLineParser.process(a);

    const FilePath tempDir = FilePath::fromUserInput(QDir::tempPath());
    const FilePath archiveUrl = FilePath::fromUserInput(commandLineParser.value("download"));
    const FilePath extractedDir = tempDir / "qt-acp-dl" / archiveUrl.baseName();
    CommandLine cmdLine{
        FilePath::fromUserInput(commandLineParser.positionalArguments().value(0)),
        commandLineParser.positionalArguments().mid(1)};

    QStringList envVars = commandLineParser.values("env");
    if (!envVars.isEmpty())
        qCDebug(dlWrapper) << "Environment variables to set:" << envVars;

    if (archiveUrl.isEmpty() || cmdLine.isEmpty()) {
        qCWarning(dlWrapper).noquote() << commandLineParser.helpText();
        return 1;
    }

    QTaskTree taskTree;
    Storage<std::unique_ptr<TemporaryFilePath>> tempArchive;

    const auto setupDownload = [tempDir, archiveUrl, tempArchive](FileStreamer &task) {
        auto tmpFileResult = TemporaryFilePath::create(tempDir / archiveUrl.fileName());
        if (!tmpFileResult) {
            qCWarning(dlWrapper) << "Failed to create temporary file:" << tmpFileResult.error();
            return SetupResult::StopWithError;
        }
        *tempArchive = std::move(*tmpFileResult);

        qCDebug(dlWrapper) << "Downloading" << archiveUrl << "to temporary file"
                           << (*tempArchive)->filePath();

        task.setSource(archiveUrl);
        task.setDestination((*tempArchive)->filePath());
        task.setStreamMode(StreamMode::Transfer);
        return SetupResult::Continue;
    };

    const auto setupUnarchive = [extractedDir, tempArchive](Unarchiver &task) {
        task.setArchive((*tempArchive)->filePath());
        task.setDestination(extractedDir);
        qCDebug(dlWrapper) << "Extracting archive to" << extractedDir;
        return SetupResult::Continue;
    };

    const auto setupProcess = [&](QProcess &process) {
        CommandLine c = cmdLine;
        c.setExecutable(extractedDir / cmdLine.executable().toUrlishString());
        process.setProgram(c.executable().toFSPathString());
        process.setArguments(c.splitArguments());
        process.setWorkingDirectory(extractedDir.toFSPathString());
        process.setInputChannelMode(QProcess::ForwardedInputChannel);
        process.setProcessChannelMode(QProcess::ForwardedChannels);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const QString &envVar : envVars) {
            const int eqIdx = envVar.indexOf('=');
            if (eqIdx > 0) {
                const QString name = envVar.left(eqIdx);
                const QString value = envVar.mid(eqIdx + 1);
                env.insert(name, value);
                qCDebug(dlWrapper) << "Setting environment variable:" << name << "=" << value;
            } else {
                qCWarning(dlWrapper)
                    << "Invalid environment variable format (expected NAME=VALUE):" << envVar;
            }
        }
        process.setProcessEnvironment(env);
        qCDebug(dlWrapper) << "Executing command" << c.toUserOutput() << "in directory"
                           << extractedDir;
    };

    const auto processDone = [&](const QProcess &process, DoneWith doneWith) {
        if (doneWith != DoneWith::Success)
            qCWarning(dlWrapper).noquote() << process.errorString();
    };

    const auto notAlreadyDownloaded = [&extractedDir, &cmdLine] {
        const FilePath executablePath = extractedDir.resolvePath(cmdLine.executable());
        if (executablePath.isExecutableFile()) {
            qCDebug(dlWrapper) << "Executable already exists at" << executablePath
                               << ", skipping download and extraction.";
            return false;
        }
        return true;
    };

    // clang-format off
    Group recipe {
        tempArchive,
        If (notAlreadyDownloaded) >> Then {
            FileStreamerTask(setupDownload),
            UnarchiverTask(setupUnarchive),
        },
        QProcessTask(setupProcess, processDone),
        onGroupDone([&](DoneWith doneWith){
            a.exit(doneWith == DoneWith::Success ? 0 : 1);
        })
    };
    // clang-format on

    taskTree.setRecipe(recipe);
    taskTree.start();

    return a.exec();
}
