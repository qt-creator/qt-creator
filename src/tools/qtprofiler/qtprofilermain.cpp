// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprofilerinit.h"
#include "qtprofilerwindow.h"
#include "qtprofilersettings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <coreplugin/icore.h>

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/qtcsettings.h>
#include <utils/qtcsettings_p.h>

#include <app/app_version.h>

#include <chrono>

#include <iostream>

#ifndef Q_OS_WIN
#include <csignal>
#endif

using namespace Utils;

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
#ifndef Q_OS_WIN
    // Launched targets are profiled with their stdio forwarded to our controlling
    // terminal (see launchThenCapture()). When such a target exits, the
    // terminal/job-control layer can deliver SIGHUP to us; its default action is
    // to terminate, which would kill the viewer just as the recording finishes.
    // A GUI tool has no use for a terminal hangup, so ignore it.
    std::signal(SIGHUP, SIG_IGN);
#endif

    QApplication app(argc, argv);
    QApplication::setOrganizationName(Core::Constants::IDE_SETTINGSVARIANT_STR);
    QApplication::setApplicationName("QtProfiler");
    QApplication::setApplicationDisplayName("Qt Profiler");
    QApplication::setApplicationVersion(Core::Constants::IDE_VERSION_COMPAT);

    QtcSettings *userSettings = new QtcSettings(QSettings::IniFormat,
                                                QSettings::UserScope,
                                                Core::Constants::IDE_SETTINGSVARIANT_STR);
    Internal::SettingsSetup::setupSettings(userSettings, new QtcSettings);
    QtProfiler::init();

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "A tool for analyzing QML profiler, Chrome Trace Format and Common Trace Format traces.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
        "tracefile",
        "A QML trace (*.qtd, *.qzip), a Chrome Trace Format file (*.json), or a Common Trace "
        "Format trace directory (or its metadata file).");
    QCommandLineOption exitOnError(QStringList({"e", "exit-on-error"}),
                                   "Exit on error, with error message on stderr.");
    parser.addOption(exitOnError);
    QCommandLineOption withRpc(QStringList({"r", "rpc"}),
                               "Activate JSON-RPC 2.0 through stdio.");
    parser.addOption(withRpc);
    QCommandLineOption printRpcSchema(QStringList({"rpc-schema"}),
                                      "Print JSON-RPC 2.0 schema to stdout and exit.");
    parser.addOption(printRpcSchema);
    QCommandLineOption windowTitle(QStringList({"t", "title"}),
                                   "Set a custom window title. Defaults to the trace file name.",
                                   "title");
    parser.addOption(windowTitle);
    QCommandLineOption launch(QStringList({"l", "launch"}),
                              "Launch the given command and profile it (e.g. \"/path/to/app --arg\").",
                              "command");
    parser.addOption(launch);
    QCommandLineOption backend(QStringList({"b", "backend"}),
                               "Recording backend to use, matched by name (e.g. \"QML\").",
                               "name");
    parser.addOption(backend);
    QCommandLineOption recordFor(QStringList({"record-for"}),
                                 "Start recording on launch and stop after <seconds>, then show "
                                 "the trace. Combine with --launch (and optionally --backend).",
                                 "seconds");
    parser.addOption(recordFor);
    parser.process(app);

    if (parser.isSet(printRpcSchema)) {
        const FilePath schemaFile(":/qtprofiler/schema/qtprofilerapi.json.schema");
        Result<QByteArray> schema = schemaFile.fileContents();
        if (schema) {
            std::cout << schema->data() << std::endl;
            return 0;
        } else {
            std::cerr << schemaFile.toUserOutput().toStdString() << " not found" << std::endl;
            return -1;
        }
    }

    QtProfiler::settings().exitOnError.setValue(parser.isSet(exitOnError));
    QtProfiler::settings().withRpc.setValue(parser.isSet(withRpc));

    if (parser.isSet(launch)) {
        const CommandLine cmd = CommandLine::fromUserInput(parser.value(launch));
        QtProfiler::settings().recordExecutable.setValue(cmd.executable());
        QtProfiler::settings().recordArguments.setValue(cmd.arguments());
    }

    QtProfiler::Window window;
    if (!parser.positionalArguments().isEmpty()) {
        const QString tracefile = parser.positionalArguments().constFirst();
        const FilePath file = FilePath::fromUserInput(tracefile);
        QTimer::singleShot(0, &window, [&window, file] {
            window.loadTraceFile(file);
        });
        const QString title = parser.isSet(windowTitle) ? parser.value(windowTitle)
                                                        : file.toUserOutput();
        window.setWindowTitle(title);
    } else if (parser.isSet(windowTitle)) {
        window.setWindowTitle(parser.value(windowTitle));
    }

    if (parser.isSet(backend)) {
        if (!window.selectBackend(parser.value(backend))) {
            std::cerr << "Unknown backend: " << parser.value(backend).toStdString() << std::endl;
            return -1;
        }
    }

    if (parser.isSet(recordFor)) {
        bool ok = false;
        const double seconds = parser.value(recordFor).toDouble(&ok);
        if (!ok || seconds <= 0.0) {
            std::cerr << "--record-for expects a positive number of seconds" << std::endl;
            return -1;
        }
        const auto duration = std::chrono::milliseconds(qint64(seconds * 1000));
        QTimer::singleShot(0, &window, [&window, duration] {
            window.startTimedRecording(duration);
        });
    }

    window.show();
    const int exitCode = app.exec();
    QtProfiler::deinit();
    Internal::SettingsSetup::destroySettings();
    return exitCode;
}
