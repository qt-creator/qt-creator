// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/commandline.h>
#include <utils/filestreamer.h>
#include <utils/temporaryfile.h>
#include <utils/textfileformat.h>
#include <utils/unarchiver.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QLocale>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QTimer>
#include <QVersionNumber>

#include <QtTaskTree/QConditional>
#include <QtTaskTree/qprocesstask.h>
#include <QtTaskTree/qtasktree.h>

#include <chrono>
#include <cstdio>

using namespace QtTaskTree;
using namespace Utils;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(dlWrapper, "qtc.dlwrapper", QtWarningMsg)

// The rate at which both the download and the extraction refresh their line.
static constexpr auto progressInterval = 500ms;

// Progress goes to stderr: stdout belongs to the wrapped command, and for an
// ACP agent that is the protocol channel.
static void report(const QString &text)
{
    const QByteArray bytes = text.toUtf8();
    std::fwrite(bytes.constData(), 1, bytes.size(), stderr);
    std::fflush(stderr);
}

// A line that rewrites itself with a carriage return has to cover what the
// previous one left behind, so it is padded to the widest it has been. The
// last one of a series ends the line, and the padding starts over.
static void reportProgress(const QString &text, bool last = false)
{
    static qsizetype widest = 0;
    widest = qMax(widest, text.size());
    QString line = QLatin1Char('\r') + text.leftJustified(widest);
    if (last) {
        line += QLatin1Char('\n');
        widest = 0;
    }
    report(line);
}

static QString formattedSeconds(const QElapsedTimer &clock)
{
    return QString::number(clock.elapsed() / 1000.0, 'f', 1) + "s";
}

static QString formattedSize(qint64 bytes)
{
    return QLocale::system().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

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
    commandLineParser.addOption(QCommandLineOption(
        "version", "The version of the archive, used to detect available updates.", "version"));
    commandLineParser
        .addPositionalArgument("executable", "The executable relative to the extracted archive root");

    commandLineParser.process(a);

    const FilePath tempDir = FilePath::fromUserInput(QDir::tempPath());
    const FilePath archiveUrl = FilePath::fromUserInput(commandLineParser.value("download"));
    const FilePath extractedDir = tempDir / "qt-acp-dl" / archiveUrl.baseName();
    const FilePath versionFile = extractedDir / "qtcreator-dlwrapper-version.txt";
    const QString registryVersion = commandLineParser.value("version");
    CommandLine cmdLine{
        FilePath::fromUserInput(commandLineParser.positionalArguments().value(0)),
        commandLineParser.positionalArguments().mid(1)};

    const QStringList envVars = commandLineParser.values("env");
    if (!envVars.isEmpty())
        qCDebug(dlWrapper) << "Environment variables to set:" << envVars;

    if (archiveUrl.isEmpty() || cmdLine.isEmpty()) {
        qCWarning(dlWrapper).noquote() << commandLineParser.helpText();
        return 1;
    }

    QTaskTree taskTree;
    Storage<std::unique_ptr<TemporaryFilePath>> tempArchive;

    // The download reports no progress of its own, and a transfer only lands in
    // the destination once it completes, so the elapsed time carries the tick
    // and the size is shown as soon as there is one.
    FilePath downloadTarget;
    QElapsedTimer downloadClock;
    QTimer progressTimer;
    progressTimer.setInterval(progressInterval);
    QObject::connect(&progressTimer, &QTimer::timeout, &a, [&downloadTarget, &downloadClock] {
        const qint64 bytes = downloadTarget.fileSize();
        reportProgress(bytes > 0 ? QString("  %1 after %2")
                                       .arg(formattedSize(bytes), formattedSeconds(downloadClock))
                                 : QString("  downloading... %1")
                                       .arg(formattedSeconds(downloadClock)));
    });

    const auto setupDownload
        = [tempDir, archiveUrl, tempArchive, &downloadTarget, &downloadClock, &progressTimer](
              FileStreamer &task) {
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

        downloadTarget = (*tempArchive)->filePath();
        report(QString("Downloading %1\n").arg(archiveUrl.toUserOutput()));
        downloadClock.start();
        progressTimer.start();
        return SetupResult::Continue;
    };

    // FileStreamer reports only whether it succeeded, so there is nothing to
    // name here but what did not arrive.
    const auto onDownloadDone = [archiveUrl, &downloadTarget, &progressTimer, &downloadClock](
                                    const FileStreamer &, DoneWith doneWith) {
        progressTimer.stop();
        if (doneWith == DoneWith::Success) {
            reportProgress(QString("  %1 downloaded in %2")
                               .arg(formattedSize(downloadTarget.fileSize()),
                                    formattedSeconds(downloadClock)),
                           true);
        } else {
            reportProgress(QString("  %1 could not be downloaded")
                               .arg(archiveUrl.toUserOutput()),
                           true);
        }
    };

    // Unarchiver reports each entry it writes, which for a large archive is far
    // more often than a terminal can show, so the entries are counted and the
    // count is refreshed on the same tick as the download's.
    int extractedEntries = 0;
    QElapsedTimer extractClock;
    QElapsedTimer extractTick;

    const auto setupUnarchive = [extractedDir, tempArchive, &a, &extractedEntries, &extractClock,
                                 &extractTick](Unarchiver &task) {
        task.setArchive((*tempArchive)->filePath());
        task.setDestination(extractedDir);
        qCDebug(dlWrapper) << "Extracting archive to" << extractedDir;
        report(QStringLiteral("Extracting\n"));

        extractedEntries = 0;
        extractClock.start();
        extractTick.start();
        QObject::connect(&task, &Unarchiver::progress, &a,
                         [&extractedEntries, &extractClock, &extractTick](const FilePath &) {
            ++extractedEntries;
            if (extractTick.durationElapsed() < progressInterval)
                return;
            extractTick.restart();
            reportProgress(QString("  %1 files after %2")
                               .arg(extractedEntries)
                               .arg(formattedSeconds(extractClock)));
        });
        return SetupResult::Continue;
    };

    const auto onUnarchiverDone = [versionFile, registryVersion, &extractedEntries, &extractClock](
                                      const Unarchiver &unarchiver) {
        if (const Result<> result = unarchiver.result(); !result) {
            reportProgress(QString("  extracting failed: %1").arg(result.error()), true);
            return DoneResult::Error;
        }

        reportProgress(QString("  %1 files extracted in %2")
                           .arg(extractedEntries)
                           .arg(formattedSeconds(extractClock)),
                       true);

        if (!registryVersion.isEmpty()) {
            if (const auto result = versionFile.writeFileContents(registryVersion.toUtf8()); !result) {
                qCWarning(dlWrapper) << "Failed to write version file:" << result.error();
            }
        }
        return DoneResult::Success;
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

    const auto notAlreadyDownloaded = [&extractedDir, &cmdLine, &versionFile, &registryVersion] {
        const FilePath executablePath = extractedDir.resolvePath(cmdLine.executable());
        if (!executablePath.isExecutableFile())
            return true;

        if (!registryVersion.isEmpty()) {
            TextFileFormat format;
            format.setEncoding(TextEncoding::Utf8);
            const TextFileFormat::ReadResult versionFileContents
                = format.readFile(versionFile, TextEncoding::Utf8);
            if (versionFileContents.code == TextFileFormat::ReadSuccess)
                return versionFileContents.content != registryVersion;
            return true;
        }

        qCDebug(dlWrapper) << "Executable already exists at" << executablePath
                           << ", skipping download and extraction.";
        return false;
    };

    // clang-format off
    Group recipe {
        tempArchive,
        If (notAlreadyDownloaded) >> Then {
            FileStreamerTask(setupDownload, onDownloadDone),
            UnarchiverTask(setupUnarchive, onUnarchiverDone),
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
