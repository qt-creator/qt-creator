/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QAtomicInt>
#include "qdeclarativetester.h"
#include "qt_private/qdeclarativedebughelper_p.h"

QT_USE_NAMESPACE

QtMsgHandler systemMsgOutput = 0;

static QDeclarativeViewer *openFile(const QString &fileName);
static void showViewer(QDeclarativeViewer *viewer);

QWeakPointer<LoggerWidget> logger;

QString warnings;
void exitApp(int i)
{
#ifdef Q_OS_WIN
    // Debugging output is not visible by default on Windows -
    // therefore show modal dialog with errors instead.
    if (!warnings.isEmpty()) {
        QMessageBox::warning(0, QApplication::tr("Qt QML Viewer"), warnings);
    }
#endif
    exit(i);
}

static QAtomicInt recursiveLock(0);

void myMessageOutput(QtMsgType type, const char *msg)
{
    QString strMsg = QString::fromLatin1(msg);

    if (!QCoreApplication::closingDown()) {
        if (!logger.isNull()) {
            if (recursiveLock.testAndSetOrdered(0, 1)) {
                QMetaObject::invokeMethod(logger.data(), "append", Q_ARG(QString, strMsg));
                recursiveLock = 0;
            }
        } else {
            warnings += strMsg;
            warnings += QLatin1Char('\n');
        }
    }
    if (systemMsgOutput) { // Windows
        systemMsgOutput(type, msg);
    } else { // Unix
        fprintf(stderr, "%s\n", msg);
        fflush(stderr);
    }
}

static QDeclarativeViewer* globalViewer = 0;

// The qml file that is shown if the user didn't specify a QML file
QString initialFile = "qrc:/startup/startup.qml";

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
#if defined(Q_OS_MAC)
    qWarning("  -no-opengl ............................... don't use a QGLWidget for the viewport");
#else
    qWarning("  -opengl .................................. use a QGLWidget for the viewport");
#endif
#ifndef NO_PRIVATE_HEADERS
    qWarning("  -script <path> ........................... set the script to use");
    qWarning("  -scriptopts <options>|help ............... set the script options to use");
#endif

    qWarning(" ");
    qWarning(" Press F1 for interactive help");

    exitApp(1);
}

void scriptOptsUsage()
{
    qWarning("Usage: qmlobserver -scriptopts <option>[,<option>...] ...");
    qWarning(" options:");
    qWarning("  record ................................... record a new script");
    qWarning("  play ..................................... playback an existing script");
    qWarning("  testimages ............................... record images or compare images on playback");
    qWarning("  testerror ................................ test 'error' property of root item on playback");
    qWarning("  testskip  ................................ test 'skip' property of root item on playback");
    qWarning("  snapshot ................................. file being recorded is static,");
    qWarning("                                             only one frame will be recorded or tested");
    qWarning("  exitoncomplete ........................... cleanly exit the viewer on script completion");
    qWarning("  exitonfailure ............................ immediately exit the viewer on script failure");
    qWarning("  saveonexit ............................... save recording on viewer exit");
    qWarning(" ");
    qWarning(" One of record, play or both must be specified.");

    exitApp(1);
}

enum WarningsConfig { ShowWarnings, HideWarnings, DefaultWarnings };

struct ViewerOptions
{
    ViewerOptions()
        : frameless(false),
          fps(0.0),
          autorecord_from(0),
          autorecord_to(0),
          dither("none"),
          runScript(false),
          devkeys(false),
          cache(0),
          useGL(false),
          fullScreen(false),
          stayOnTop(false),
          maximized(false),
          useNativeFileBrowser(true),
          experimentalGestures(false),
          warningsConfig(DefaultWarnings),
          sizeToView(true)
    {
#if defined(Q_OS_MAC)
        useGL = true;
#endif
    }

    bool frameless;
    double fps;
    int autorecord_from;
    int autorecord_to;
    QString dither;
    QString recordfile;
    QStringList recordargs;
    QStringList imports;
    QStringList plugins;
    QString script;
    QString scriptopts;
    bool runScript;
    bool devkeys;
    int cache;
    QString translationFile;
    bool useGL;
    bool fullScreen;
    bool stayOnTop;
    bool maximized;
    bool useNativeFileBrowser;
    bool experimentalGestures;

    WarningsConfig warningsConfig;
    bool sizeToView;

    QDeclarativeViewer::ScriptOptions scriptOptions;
};

static ViewerOptions opts;
static QStringList fileNames;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **&argv)
        : QApplication(argc, argv)
    {}

protected:
    bool event(QEvent *ev)
    {
        if (ev->type() != QEvent::FileOpen)
            return QApplication::event(ev);

        QFileOpenEvent *fev = static_cast<QFileOpenEvent *>(ev);

        globalViewer->open(fev->file());
        if (!globalViewer->isVisible())
            showViewer(globalViewer);

        return true;
    }

private Q_SLOTS:
    void showInitialViewer()
    {
        QApplication::processEvents();

        QDeclarativeViewer *viewer = globalViewer;
        if (!viewer)
            return;
        if (viewer->currentFile().isEmpty()) {
            if(opts.useNativeFileBrowser)
                viewer->open(initialFile);
            else
                viewer->openFile();
        }
        if (!viewer->isVisible())
            showViewer(viewer);
    }
};

static void parseScriptOptions()
{
    QStringList options =
        opts.scriptopts.split(QLatin1Char(','), QString::SkipEmptyParts);

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
        } else if (option == QLatin1String("testskip")) {
            scriptOptions |= QDeclarativeViewer::TestSkipProperty;
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

    opts.scriptOptions = scriptOptions;
}

static void parseCommandLineOptions(const QStringList &arguments)
{
    for (int i = 1; i < arguments.count(); ++i) {
        bool lastArg = (i == arguments.count() - 1);
        QString arg = arguments.at(i);
        if (arg == "-frameless") {
            opts.frameless = true;
        } else if (arg == "-maximized") {
            opts.maximized = true;
        } else if (arg == "-fullscreen") {
            opts.fullScreen = true;
        } else if (arg == "-stayontop") {
            opts.stayOnTop = true;
        } else if (arg == "-netcache") {
            if (lastArg) usage();
            opts.cache = arguments.at(++i).toInt();
        } else if (arg == "-recordrate") {
            if (lastArg) usage();
            opts.fps = arguments.at(++i).toDouble();
        } else if (arg == "-recordfile") {
            if (lastArg) usage();
            opts.recordfile = arguments.at(++i);
        } else if (arg == "-record") {
            if (lastArg) usage();
            opts.recordargs << arguments.at(++i);
        } else if (arg == "-recorddither") {
            if (lastArg) usage();
            opts.dither = arguments.at(++i);
        } else if (arg == "-autorecord") {
            if (lastArg) usage();
            QString range = arguments.at(++i);
            int dash = range.indexOf('-');
            if (dash > 0)
                opts.autorecord_from = range.left(dash).toInt();
            opts.autorecord_to = range.mid(dash+1).toInt();
        } else if (arg == "-devicekeys") {
            opts.devkeys = true;
        } else if (arg == "-dragthreshold") {
            if (lastArg) usage();
            qApp->setStartDragDistance(arguments.at(++i).toInt());
        } else if (arg == QLatin1String("-v") || arg == QLatin1String("-version")) {
            qWarning("Qt QML Viewer version %s", qVersion());
            exitApp(0);
        } else if (arg == "-translation") {
            if (lastArg) usage();
            opts.translationFile = arguments.at(++i);
#if defined(Q_OS_MAC)
        } else if (arg == "-no-opengl") {
            opts.useGL = false;
#else
        } else if (arg == "-opengl") {
            opts.useGL = true;
#endif
        } else if (arg == "-qmlbrowser") {
            opts.useNativeFileBrowser = false;
        } else if (arg == "-warnings") {
            if (lastArg) usage();
            QString warningsStr = arguments.at(++i);
            if (warningsStr == QLatin1String("show")) {
                opts.warningsConfig = ShowWarnings;
            } else if (warningsStr == QLatin1String("hide")) {
                opts.warningsConfig = HideWarnings;
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
                exitApp(0);
            }
            opts.imports << arguments.at(++i);
        } else if (arg == "-P") {
            if (lastArg) usage();
            opts.plugins << arguments.at(++i);
        } else if (arg == "-script") {
            if (lastArg) usage();
            opts.script = arguments.at(++i);
        } else if (arg == "-scriptopts") {
            if (lastArg) usage();
            opts.scriptopts = arguments.at(++i);
        } else if (arg == "-savescript") {
            if (lastArg) usage();
            opts.script = arguments.at(++i);
            opts.runScript = false;
        } else if (arg == "-playscript") {
            if (lastArg) usage();
            opts.script = arguments.at(++i);
            opts.runScript = true;
        } else if (arg == "-sizeviewtorootobject") {
            opts.sizeToView = false;
        } else if (arg == "-sizerootobjecttoview") {
            opts.sizeToView = true;
        } else if (arg == "-experimentalgestures") {
            opts.experimentalGestures = true;
        } else if (!arg.startsWith('-')) {
            fileNames.append(arg);
        } else if (true || arg == "-help") {
            usage();
        }
    }

    if (!opts.scriptopts.isEmpty()) {

        parseScriptOptions();

        if (opts.script.isEmpty())
            usage();

        if (!(opts.scriptOptions & QDeclarativeViewer::Record) && !(opts.scriptOptions & QDeclarativeViewer::Play))
            scriptOptsUsage();
    }  else if (!opts.script.isEmpty()) {
        usage();
    }

}

static QDeclarativeViewer *createViewer()
{
    Qt::WFlags wflags = (opts.frameless ? Qt::FramelessWindowHint : Qt::Widget);
    if (opts.stayOnTop)
        wflags |= Qt::WindowStaysOnTopHint;

    QDeclarativeViewer *viewer = new QDeclarativeViewer(0, wflags);
    viewer->setAttribute(Qt::WA_DeleteOnClose, true);
    viewer->setUseGL(opts.useGL);

    if (!opts.scriptopts.isEmpty()) {
        viewer->setScriptOptions(opts.scriptOptions);
        viewer->setScript(opts.script);
    }

    logger = viewer->warningsWidget();
    if (opts.warningsConfig == ShowWarnings) {
        logger.data()->setDefaultVisibility(LoggerWidget::ShowWarnings);
        logger.data()->show();
    } else if (opts.warningsConfig == HideWarnings){
        logger.data()->setDefaultVisibility(LoggerWidget::HideWarnings);
    }

    if (opts.experimentalGestures)
        viewer->enableExperimentalGestures();

    foreach (const QString &lib, opts.imports)
        viewer->addLibraryPath(lib);

    foreach (const QString &plugin, opts.plugins)
        viewer->addPluginPath(plugin);

    viewer->setNetworkCacheSize(opts.cache);
    viewer->setRecordFile(opts.recordfile);
    viewer->setSizeToView(opts.sizeToView);
    if (opts.fps > 0)
        viewer->setRecordRate(opts.fps);
    if (opts.autorecord_to)
        viewer->setAutoRecord(opts.autorecord_from, opts.autorecord_to);
    if (opts.devkeys)
        viewer->setDeviceKeys(true);
    viewer->setRecordDither(opts.dither);
    if (opts.recordargs.count())
        viewer->setRecordArgs(opts.recordargs);

    viewer->setUseNativeFileBrowser(opts.useNativeFileBrowser);

    return viewer;
}

void showViewer(QDeclarativeViewer *viewer)
{
    if (opts.fullScreen)
        viewer->showFullScreen();
    else if (opts.maximized)
        viewer->showMaximized();
    else
        viewer->show();
    viewer->raise();
}

QDeclarativeViewer *openFile(const QString &fileName)
{
    QDeclarativeViewer *viewer = globalViewer;

    viewer->open(fileName);
    showViewer(viewer);

    return viewer;
}

int main(int argc, char ** argv)
{
    systemMsgOutput = qInstallMsgHandler(myMessageOutput);

#if defined (Q_WS_X11) || defined (Q_OS_MAC)
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

    Application app(argc, argv);
    app.setApplicationName("QtQmlViewer");
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");

    QDeclarativeViewer::registerTypes();
    QDeclarativeTester::registerTypes();

    parseCommandLineOptions(app.arguments());

    QTranslator qmlTranslator;
    if (!opts.translationFile.isEmpty()) {
        qmlTranslator.load(opts.translationFile);
        app.installTranslator(&qmlTranslator);
    }

    if (opts.fullScreen && opts.maximized)
        qWarning() << "Both -fullscreen and -maximized specified. Using -fullscreen.";

    if (fileNames.isEmpty()) {
        QFile qmlapp(QLatin1String("qmlapp"));
        if (qmlapp.exists() && qmlapp.open(QFile::ReadOnly)) {
            QString content = QString::fromUtf8(qmlapp.readAll());
            qmlapp.close();

            int newline = content.indexOf(QLatin1Char('\n'));
            if (newline >= 0)
                fileNames += content.left(newline);
            else
                fileNames += content;
        }
    }

    //enable remote debugging
    QDeclarativeDebugHelper::enableDebugging();

    globalViewer = createViewer();

    if (fileNames.isEmpty()) {
        // show the initial viewer delayed.
        // This prevents an initial viewer popping up while there
        // are FileOpen events coming through the event queue
        QTimer::singleShot(1, &app, SLOT(showInitialViewer()));
    } else {
        foreach (const QString &fileName, fileNames)
            openFile(fileName);
    }

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    return app.exec();
}

#include "main.moc"
