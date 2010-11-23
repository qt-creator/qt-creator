/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarative.h"
#include "qmlruntime.h"
#include "qdeclarativeengine.h"
#include "loggerwidget.h"
#include <QWidget>
#include <QDir>
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <QMessageBox>
#include "qdeclarativetester.h"
#include "qt_private/qdeclarativedebughelper_p.h"

QT_USE_NAMESPACE

QtMsgHandler systemMsgOutput = 0;

#if defined (Q_OS_SYMBIAN)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void myMessageOutput(QtMsgType type, const char *msg)
{
    static int fd = -1;
    if (fd == -1)
        fd = ::open("E:\\qml.log", O_WRONLY | O_CREAT);

    ::write(fd, msg, strlen(msg));
    ::write(fd, "\n", 1);
    ::fsync(fd);

    switch (type) {
    case QtFatalMsg:
        abort();
    }
}

#else // !defined (Q_OS_SYMBIAN)

QWeakPointer<LoggerWidget> logger;

QString warnings;
void showWarnings()
{
    if (!warnings.isEmpty()) {
        int argc = 0; char **argv = 0;
        QApplication application(argc, argv); // QApplication() in main has been destroyed already.
        Q_UNUSED(application)
        QMessageBox::warning(0, QApplication::tr("Qt QML Viewer"), warnings);
    }
}

void myMessageOutput(QtMsgType type, const char *msg)
{
    if (!logger.isNull()) {
        QString strMsg = QString::fromAscii(msg);
        QMetaObject::invokeMethod(logger.data(), "append", Q_ARG(QString, strMsg));
    } else {
        warnings += msg;
        warnings += QLatin1Char('\n');
    }
    if (systemMsgOutput) { // Windows
        systemMsgOutput(type, msg);
    } else { // Unix
        fprintf(stderr, "%s\n",msg);
        fflush(stderr);
    }
}

#endif

void usage()
{
    qWarning("Usage: qmlobserver [options] <filename>");
    qWarning(" ");
    qWarning(" options:");
    qWarning("  -v, -version ............................. display version");
    qWarning("  -frameless ............................... run with no window frame");
    qWarning("  -maximized................................ run maximized");
    qWarning("  -fullscreen............................... run fullscreen");
    qWarning("  -stayontop................................ keep viewer window on top");
    qWarning("  -sizeviewtorootobject .................... the view resizes to the changes in the content");
    qWarning("  -sizerootobjecttoview .................... the content resizes to the changes in the view (default)");
    qWarning("  -qmlbrowser .............................. use a QML-based file browser");
    qWarning("  -warnings [show|hide]..................... show warnings in a separate log window");
#ifndef NO_PRIVATE_HEADERS
    qWarning("  -recordfile <output> ..................... set video recording file");
    qWarning("                                              - ImageMagick 'convert' for GIF)");
    qWarning("                                              - png file for raw frames");
    qWarning("                                              - 'ffmpeg' for other formats");
    qWarning("  -recorddither ordered|threshold|floyd .... set GIF dither recording mode");
    qWarning("  -recordrate <fps> ........................ set recording frame rate");
    qWarning("  -record arg .............................. add a recording process argument");
    qWarning("  -autorecord [from-]<tomilliseconds> ...... set recording to start and stop");
#endif
    qWarning("  -devicekeys .............................. use numeric keys (see F1)");
    qWarning("  -dragthreshold <size> .................... set mouse drag threshold size");
    qWarning("  -netcache <size> ......................... set disk cache to size bytes");
    qWarning("  -translation <translationfile> ........... set the language to run in");
    qWarning("  -I <directory> ........................... prepend to the module import search path,");
    qWarning("                                             display path if <directory> is empty");
    qWarning("  -P <directory> ........................... prepend to the plugin search path");
    qWarning("  -opengl .................................. use a QGLWidget for the viewport");
#ifndef NO_PRIVATE_HEADERS
    qWarning("  -script <path> ........................... set the script to use");
    qWarning("  -scriptopts <options>|help ............... set the script options to use");
#endif

    qWarning(" ");
    qWarning(" Press F1 for interactive help");
    exit(1);
}

void scriptOptsUsage()
{
    qWarning("Usage: qmlobserver -scriptopts <option>[,<option>...] ...");
    qWarning(" options:");
    qWarning("  record ................................... record a new script");
    qWarning("  play ..................................... playback an existing script");
    qWarning("  testimages ............................... record images or compare images on playback");
    qWarning("  testerror ................................ test 'error' property of root item on playback");
    qWarning("  snapshot ................................. file being recorded is static,");
    qWarning("                                             only one frame will be recorded or tested");
    qWarning("  exitoncomplete ........................... cleanly exit the viewer on script completion");
    qWarning("  exitonfailure ............................ immediately exit the viewer on script failure");
    qWarning("  saveonexit ............................... save recording on viewer exit");
    qWarning(" ");
    qWarning(" One of record, play or both must be specified.");
    exit(1);
}

enum WarningsConfig { ShowWarnings, HideWarnings, DefaultWarnings };

int main(int argc, char ** argv)
{
#if defined (Q_OS_SYMBIAN)
    qInstallMsgHandler(myMessageOutput);
#else
    systemMsgOutput = qInstallMsgHandler(myMessageOutput);
#endif

#if defined (Q_OS_WIN)
    // Debugging output is not visible by default on Windows -
    // therefore show modal dialog with errors instead.

    // (Disabled in QmlObserver: We're usually running inside QtCreator anyway, see also QTCREATORBUG-2748)
//    atexit(showWarnings);
#endif

#if defined (Q_WS_X11) || defined (Q_WS_MAC)
    //### default to using raster graphics backend for now
    bool gsSpecified = false;
    for (int i = 0; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "-graphicssystem") {
            gsSpecified = true;
            break;
        }
    }

    if (!gsSpecified)
        QApplication::setGraphicsSystem("raster");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("QtQmlViewer");
    app.setOrganizationName("Nokia");
    app.setOrganizationDomain("nokia.com");

    QDeclarativeViewer::registerTypes();
    QDeclarativeTester::registerTypes();

    bool frameless = false;
    QString fileName;
    double fps = 0;
    int autorecord_from = 0;
    int autorecord_to = 0;
    QString dither = "none";
    QString recordfile;
    QStringList recordargs;
    QStringList imports;
    QStringList plugins;
    QString script;
    QString scriptopts;
    bool runScript = false;
    bool devkeys = false;
    int cache = 0;
    QString translationFile;
    bool useGL = false;
    bool fullScreen = false;
    bool stayOnTop = false;
    bool maximized = false;
    bool useNativeFileBrowser = true;
    bool experimentalGestures = false;
    bool designModeBehavior = false;
    bool debuggerModeBehavior = false;

    WarningsConfig warningsConfig = DefaultWarnings;
    bool sizeToView = true;

#if defined(Q_OS_SYMBIAN)
    maximized = true;
    useNativeFileBrowser = false;
#endif

#if defined(Q_WS_MAC)
    useGL = true;
#endif

    for (int i = 1; i < argc; ++i) {
        bool lastArg = (i == argc - 1);
        QString arg = argv[i];
        if (arg == "-frameless") {
            frameless = true;
        } else if (arg == "-maximized") {
            maximized = true;
        } else if (arg == "-fullscreen") {
            fullScreen = true;
        } else if (arg == "-stayontop") {
            stayOnTop = true;
        } else if (arg == "-netcache") {
            if (lastArg) usage();
            cache = QString(argv[++i]).toInt();
        } else if (arg == "-recordrate") {
            if (lastArg) usage();
            fps = QString(argv[++i]).toDouble();
        } else if (arg == "-recordfile") {
            if (lastArg) usage();
            recordfile = QString(argv[++i]);
        } else if (arg == "-record") {
            if (lastArg) usage();
            recordargs << QString(argv[++i]);
        } else if (arg == "-recorddither") {
            if (lastArg) usage();
            dither = QString(argv[++i]);
        } else if (arg == "-autorecord") {
            if (lastArg) usage();
            QString range = QString(argv[++i]);
            int dash = range.indexOf('-');
            if (dash > 0)
                autorecord_from = range.left(dash).toInt();
            autorecord_to = range.mid(dash+1).toInt();
        } else if (arg == "-devicekeys") {
            devkeys = true;
        } else if (arg == "-dragthreshold") {
            if (lastArg) usage();
            app.setStartDragDistance(QString(argv[++i]).toInt());
        } else if (arg == QLatin1String("-v") || arg == QLatin1String("-version")) {
            qWarning("Qt QML Viewer version %s", QT_VERSION_STR);
            exit(0);
        } else if (arg == "-translation") {
            if (lastArg) usage();
            translationFile = argv[++i];
        } else if (arg == "-opengl") {
            useGL = true;
        } else if (arg == "-qmlbrowser") {
            useNativeFileBrowser = false;
        } else if (arg == "-warnings") {
            if (lastArg) usage();
            QString warningsStr = QString(argv[++i]);
            if (warningsStr == QLatin1String("show")) {
                warningsConfig = ShowWarnings;
            } else if (warningsStr == QLatin1String("hide")) {
                warningsConfig = HideWarnings;
            } else {
                usage();
            }
        } else if (arg == "-I" || arg == "-L") {
            if (arg == "-L")
                qWarning("-L option provided for compatibility only, use -I instead");
            if (lastArg) {
                QDeclarativeEngine tmpEngine;
                QString paths = tmpEngine.importPathList().join(QLatin1String(":"));
                qWarning("Current search path: %s", paths.toLocal8Bit().constData());
                exit(0);
            }
            imports << QString(argv[++i]);
        } else if (arg == "-P") {
            if (lastArg) usage();
            plugins << QString(argv[++i]);
        } else if (arg == "-script") {
            if (lastArg) usage();
            script = QString(argv[++i]);
        } else if (arg == "-scriptopts") {
            if (lastArg) usage();
            scriptopts = QString(argv[++i]);
        } else if (arg == "-savescript") {
            if (lastArg) usage();
            script = QString(argv[++i]);
            runScript = false;
        } else if (arg == "-playscript") {
            if (lastArg) usage();
            script = QString(argv[++i]);
            runScript = true;
        } else if (arg == "-sizeviewtorootobject") {
            sizeToView = false;
        } else if (arg == "-sizerootobjecttoview") {
            sizeToView = true;
        } else if (arg == "-experimentalgestures") {
            experimentalGestures = true;
        } else if (arg == "-designmode") {
            designModeBehavior = true;
        } else if (arg == "-debugger") {
            debuggerModeBehavior = true;
        } else if (arg[0] != '-') {
            fileName = arg;
        } else if (1 || arg == "-help") {
            usage();
        }
    }

    QTranslator qmlTranslator;
    if (!translationFile.isEmpty()) {
        qmlTranslator.load(translationFile);
        app.installTranslator(&qmlTranslator);
    }

    Qt::WFlags wflags = (frameless ? Qt::FramelessWindowHint : Qt::Widget);
    if (stayOnTop)
        wflags |= Qt::WindowStaysOnTopHint;

    // enable remote debugging
    QDeclarativeDebugHelper::enableDebugging();

    QDeclarativeViewer *viewer = new QDeclarativeViewer(0, wflags);
    viewer->setAttribute(Qt::WA_DeleteOnClose, true);
    if (!scriptopts.isEmpty()) {
        QStringList options =
            scriptopts.split(QLatin1Char(','), QString::SkipEmptyParts);

        QDeclarativeViewer::ScriptOptions scriptOptions = 0;
        for (int i = 0; i < options.count(); ++i) {
            const QString &option = options.at(i);
            if (option == QLatin1String("help")) {
                scriptOptsUsage();
            } else if (option == QLatin1String("play")) {
                scriptOptions |= QDeclarativeViewer::Play;
            } else if (option == QLatin1String("record")) {
                scriptOptions |= QDeclarativeViewer::Record;
            } else if (option == QLatin1String("testimages")) {
                scriptOptions |= QDeclarativeViewer::TestImages;
            } else if (option == QLatin1String("testerror")) {
                scriptOptions |= QDeclarativeViewer::TestErrorProperty;
            } else if (option == QLatin1String("exitoncomplete")) {
                scriptOptions |= QDeclarativeViewer::ExitOnComplete;
            } else if (option == QLatin1String("exitonfailure")) {
                scriptOptions |= QDeclarativeViewer::ExitOnFailure;
            } else if (option == QLatin1String("saveonexit")) {
                scriptOptions |= QDeclarativeViewer::SaveOnExit;
            } else if (option == QLatin1String("snapshot")) {
                scriptOptions |= QDeclarativeViewer::Snapshot;
            } else {
                scriptOptsUsage();
            }
        }

        if (script.isEmpty())
            usage();

        if (!(scriptOptions & QDeclarativeViewer::Record) && !(scriptOptions & QDeclarativeViewer::Play))
            scriptOptsUsage();
        viewer->setScriptOptions(scriptOptions);
        viewer->setScript(script);
    }  else if (!script.isEmpty()) {
        usage();
    }

#if !defined(Q_OS_SYMBIAN)
    logger = viewer->warningsWidget();
    if (warningsConfig == ShowWarnings) {
        logger.data()->setDefaultVisibility(LoggerWidget::ShowWarnings);
        logger.data()->show();
    } else if (warningsConfig == HideWarnings){
        logger.data()->setDefaultVisibility(LoggerWidget::HideWarnings);
    }
#endif

    if (experimentalGestures)
        viewer->enableExperimentalGestures();

    viewer->setDesignModeBehavior(designModeBehavior);
    viewer->setStayOnTop(stayOnTop);

    foreach (QString lib, imports)
        viewer->addLibraryPath(lib);

    foreach (QString plugin, plugins)
        viewer->addPluginPath(plugin);

    viewer->setNetworkCacheSize(cache);
    viewer->setRecordFile(recordfile);
    viewer->setSizeToView(sizeToView);
    if (fps>0)
        viewer->setRecordRate(fps);
    if (autorecord_to)
        viewer->setAutoRecord(autorecord_from,autorecord_to);
    if (devkeys)
        viewer->setDeviceKeys(true);
    viewer->setRecordDither(dither);
    if (recordargs.count())
        viewer->setRecordArgs(recordargs);

    viewer->setUseNativeFileBrowser(useNativeFileBrowser);
    if (fullScreen && maximized)
        qWarning() << "Both -fullscreen and -maximized specified. Using -fullscreen.";

    if (fileName.isEmpty()) {
        QFile qmlapp(QLatin1String("qmlapp"));
        if (qmlapp.exists() && qmlapp.open(QFile::ReadOnly)) {
                QString content = QString::fromUtf8(qmlapp.readAll());
                qmlapp.close();

                int newline = content.indexOf(QLatin1Char('\n'));
                if (newline >= 0)
                    fileName = content.left(newline);
                else
                    fileName = content;
            }
    }

    if (!fileName.isEmpty()) {
        viewer->open(fileName);
        fullScreen ? viewer->showFullScreen() : maximized ? viewer->showMaximized() : viewer->show();
    } else {
        if (!useNativeFileBrowser)
            viewer->openFile();
        fullScreen ? viewer->showFullScreen() : maximized ? viewer->showMaximized() : viewer->show();
        if (useNativeFileBrowser)
            viewer->openFile();
    }
    viewer->setUseGL(useGL);
    viewer->raise();

    return app.exec();
}
