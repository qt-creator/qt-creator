// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerinit.h"
#include "qmltraceviewerwindow.h"
#include "qmltraceviewersettings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <qmlprofiler/qmlprofilerplainviewmanager.h>

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/qtcsettings.h>
#include <utils/qtcsettings_p.h>

#include <app/app_version.h>

#include <iostream>

using namespace Utils;

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(Core::Constants::IDE_SETTINGSVARIANT_STR);
    QApplication::setApplicationName("QmlTraceViewer");
    QApplication::setApplicationDisplayName("QML Profiler Trace Viewer");
    QApplication::setApplicationVersion(Core::Constants::IDE_VERSION_COMPAT);

    QtcSettings *userSettings = new QtcSettings(QSettings::IniFormat,
                                                QSettings::UserScope,
                                                Core::Constants::IDE_SETTINGSVARIANT_STR);
    Internal::SettingsSetup::setupSettings(userSettings, new QtcSettings);
    QmlTraceViewer::init();

    QCommandLineParser parser;
    parser.setApplicationDescription("A tool for analyzing QML profiler traces.");
    parser.addHelpOption();
    parser.addVersionOption();
    const QString extensions = QmlProfiler::QmlProfilerPlainViewManager::fileDialogTraceFilesFilter()
                                   .split(";;").constFirst();
    parser.addPositionalArgument("tracefile", extensions);
    QCommandLineOption exitOnError(QStringList({"e", "exit-on-error"}),
                                   "Exit on error, with error message on stderr.");
    parser.addOption(exitOnError);
    QCommandLineOption withRpc(QStringList({"r", "rpc"}),
                               "Activate JSON-RPC 2.0 through stdio.");
    parser.addOption(withRpc);
    QCommandLineOption printRpcSchema(QStringList({"rpc-schema"}),
                                      "Print JSON-RPC 2.0 schema to stdout and exit.");
    parser.addOption(printRpcSchema);
    parser.process(app);

    if (parser.isSet(printRpcSchema)) {
        const FilePath schemaFile(":/qmltraceviewer/schema/qmltraceviewerapi.json.schema");
        Result<QByteArray> schema = schemaFile.fileContents();
        if (schema) {
            std::cout << schema->data() << std::endl;
            return 0;
        } else {
            std::cerr << schemaFile.toUserOutput().toStdString() << " not found" << std::endl;
            return -1;
        }
    }

    QmlTraceViewer::settings().exitOnError.setValue(parser.isSet(exitOnError));
    QmlTraceViewer::settings().withRpc.setValue(parser.isSet(withRpc));

    QmlTraceViewer::Window window;
    if (!parser.positionalArguments().isEmpty()) {
        const QString tracefile = parser.positionalArguments().constFirst();
        const FilePath file = FilePath::fromUserInput(tracefile);
        QTimer::singleShot(0, &window, [&window, file] {
            window.loadTraceFile(file);
        });
    }

    window.show();
    const int exitCode = app.exec();
    QmlTraceViewer::deinit();
    Internal::SettingsSetup::destroySettings();
    return exitCode;
}
