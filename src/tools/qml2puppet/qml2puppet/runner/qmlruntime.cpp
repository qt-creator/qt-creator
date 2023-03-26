// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QIcon>

#include "loadwatcher.h"
#include "qmlruntime.h"

#include <private/qqmlimport_p.h>
#include <private/qtqmlglobal_p.h>
#if QT_CONFIG(qml_animation)
#include <private/qabstractanimation_p.h>
#endif

#define FILE_OPEN_EVENT_WAIT_TIME 3000 // ms
#define QSL QStringLiteral

void QmlRuntime::populateParser()
{
    m_argParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    m_argParser.setOptionsAfterPositionalArgumentsMode(
        QCommandLineParser::ParseAsPositionalArguments);

    m_argParser.addOptions(
        {{QStringList() << QSL("a") << QSL("apptype"),
          QSL("Select which application class to use. Default is gui."),
          QSL("core|gui|widget")}, // just for translation

         {QSL("I"), QSL("Prepend the given path to the import paths."), QSL("path")},

         {QSL("f"), QSL("Load the given file as a QML file."), QSL("file")},

         {QStringList() << QSL("c") << QSL("config"),
          QSL("Load the given built-in configuration or configuration file."),
          QSL("file")},

         {QStringList() << QSL("list-conf"), QSL("List the built-in configurations.")},

         {QSL("translation"), QSL("Load the given file as the translations file."), QSL("file")},

#ifdef QT_GUI_LIB
         // OpenGL options
         {QSL("desktop"), QSL("Force use of desktop OpenGL (AA_UseDesktopOpenGL).")},

         {QSL("gles"), QSL("Force use of GLES (AA_UseOpenGLES).")},

         {QSL("software"), QSL("Force use of software rendering (AA_UseSoftwareOpenGL).")},

         {QSL("core-profile"), QSL("Force use of OpenGL Core Profile.")},

         {QSL("disable-context-sharing"),
          QSL("Disable the use of a shared GL context for QtQuick Windows")},
#endif // QT_GUI_LIB

         // Debugging and verbosity options
         {QSL("quiet"), QSL("Suppress all output.")},

         {QSL("verbose"),
          QSL("Print information about what qml is doing, like specific file URLs being loaded.")},

         {QSL("slow-animations"), QSL("Run all animations in slow motion.")},

         {QSL("fixed-animations"), QSL("Run animations off animation tick rather than wall time.")},

         {QStringList() << QSL("r") << QSL("rhi"),
          QSL("Set the backend for the Qt graphics abstraction (RHI). "
              "Backend is one of: default, vulkan, metal, d3d11, gl"),
          QSL("backend")},

         {QSL("S"), QSL("Add selector to the list of QQmlFileSelectors."), QSL("selector")}});

    // Positional arguments
    m_argParser.addPositionalArgument(
        "files",
        QSL("Any number of QML files can be loaded. They will share the same engine."),
        "[files...]");
    m_argParser.addPositionalArgument("args",
                                      QSL("Arguments after '--' are ignored, but passed through to "
                                          "the application.arguments variable in QML."),
                                      "[-- args...]");
}

void QmlRuntime::initCoreApp()
{
    bool glShareContexts = true;

    // these attributes must be set before the QCoreApp is initialized
    for (int i = 0; i < m_args.argc; i++) {
        if (!strcmp(m_args.argv[i], "-desktop") || !strcmp(m_args.argv[i], "--desktop")) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        } else if (!strcmp(m_args.argv[i], "-gles") || !strcmp(m_args.argv[i], "--gles")) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
        } else if (!strcmp(m_args.argv[i], "-software") || !strcmp(m_args.argv[i], "--software")) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        } else if (!strcmp(m_args.argv[i], "-disable-context-sharing")
                   || !strcmp(m_args.argv[i], "--disable-context-sharing")) {
            glShareContexts = false;
        }
    }

    if (glShareContexts)
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // since we handled all attributes above, now we can initialize the core app
    for (int i = 0; i < m_args.argc; i++) {
        if (!strcmp(m_args.argv[i], "--apptype") || !strcmp(m_args.argv[i], "-a")
            || !strcmp(m_args.argv[i], "-apptype")) {
            if (i + 1 < m_args.argc) {
                ++i;
                if (!strcmp(m_args.argv[i], "core")) {
                    createCoreApp<QCoreApplication>();
                }
                else if (!strcmp(m_args.argv[i], "gui")) {
                    createCoreApp<QGuiApplication>();
                }
#ifdef QT_WIDGETS_LIB
                else if (!strcmp(m_args.argv[i], "widget")) {
                    createCoreApp<QApplication>();
                    static_cast<QApplication *>(m_coreApp.get())
                        ->setWindowIcon(QIcon(m_iconResourcePath));
                }
#endif // QT_WIDGETS_LIB
            }
        }
    }
}

void QmlRuntime::initQmlRunner()
{
    m_qmlEngine.reset(new QQmlApplicationEngine());

    QStringList files;
    QString confFile;
    QString translationFile;

    if (!m_argParser.parse(QCoreApplication::arguments())) {
        qWarning() << m_argParser.errorText();
        exit(1);
    }

    // TODO: replace below logging modes with a proper logging category
    m_verboseMode = m_argParser.isSet("verbose");
    m_quietMode = (!m_verboseMode && m_argParser.isSet("quiet"));
    // FIXME: need to re-evaluate. we have our own message handler.
    //    if (quietMode) {
    //        qInstallMessageHandler(quietMessageHandler);
    //        QLoggingCategory::setFilterRules(QStringLiteral("*=false"));
    //    }

    if (m_argParser.isSet("list-conf")) {
        listConfFiles();
        exit(0);
    }

#if QT_CONFIG(qml_animation)
    if (m_argParser.isSet("slow-animations"))
        QUnifiedTimer::instance()->setSlowModeEnabled(true);
    if (m_argParser.isSet("fixed-animations"))
        QUnifiedTimer::instance()->setConsistentTiming(true);
#endif
    const auto valsImportPath = m_argParser.values("I");
    for (const QString &importPath : valsImportPath)
        m_qmlEngine->addImportPath(importPath);

    QStringList customSelectors;

    const auto valsSelectors = m_argParser.values("S");
    for (const QString &selector : valsSelectors)
        customSelectors.append(selector);

    if (!customSelectors.isEmpty())
        m_qmlEngine->setExtraFileSelectors(customSelectors);

    if (qEnvironmentVariableIsSet("QSG_CORE_PROFILE")
        || qEnvironmentVariableIsSet("QML_CORE_PROFILE") || m_argParser.isSet("core-profile")) {
        QSurfaceFormat surfaceFormat;
        surfaceFormat.setStencilBufferSize(8);
        surfaceFormat.setDepthBufferSize(24);
        surfaceFormat.setVersion(4, 1);
        surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(surfaceFormat);
    }

    if (m_argParser.isSet("config"))
        confFile = m_argParser.value("config");
    if (m_argParser.isSet("translation"))
        translationFile = m_argParser.value("translation");
    if (m_argParser.isSet("rhi")) {
        const QString rhiBackend = m_argParser.value("rhi");
        if (rhiBackend == QLatin1String("default"))
            qunsetenv("QSG_RHI_BACKEND");
        else
            qputenv("QSG_RHI_BACKEND", rhiBackend.toLatin1());
    }

    const auto valsPosArgs = m_argParser.positionalArguments();
    files << m_argParser.values("f");
    for (const QString &posArg : valsPosArgs) {
        if (posArg == QLatin1String("--"))
            break;
        else
            files << posArg;
    }

#if QT_CONFIG(translation)
    // Need to be installed before QQmlApplicationEngine's automatic translation loading
    // (qt_ translations are loaded there)
    if (!translationFile.isEmpty()) {
        QTranslator translator;

        if (translator.load(translationFile)) {
            m_coreApp->installTranslator(&translator);
            if (m_verboseMode)
                qInfo() << "qml: Loaded translation file %s\n",
                    qPrintable(QDir::toNativeSeparators(translationFile));
        } else {
            if (!m_quietMode)
                qInfo() << "qml: Could not load the translation file %s\n",
                    qPrintable(QDir::toNativeSeparators(translationFile));
        }
    }
#else
    if (!translationFile.isEmpty() && !quietMode)
        qInfo() << "qml: Translation file specified, but Qt built without translation support.\n");
#endif

    if (files.size() <= 0) {
#if defined(Q_OS_DARWIN)
        if (qobject_cast<QGuiApplication *>(m_coreApp.data())) {
            m_exitTimerId = static_cast<QGuiApplication *>(m_coreApp.get())
                                ->startTimer(FILE_OPEN_EVENT_WAIT_TIME);
        } else
#endif
        {
            if (!m_quietMode)
                qCritical() << "No files specified. Terminating.\n";
            exit(1);
        }
    }

    loadConf(confFile, !m_verboseMode);

    // Load files
    LoadWatcher *lw = new LoadWatcher(m_qmlEngine.data(), files.size(), m_conf.data());
    lw->setParent(this);

    for (const QString &path : std::as_const(files)) {
        QUrl url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (m_verboseMode)
            qInfo() << "qml: loading %s\n", qPrintable(url.toString());
        m_qmlEngine->load(url);
    }

    if (lw->earlyExit)
        exit(lw->returnCode);
}

void QmlRuntime::loadConf(const QString &override, bool quiet) // Terminates app on failure
{
    const QString defaultFileName = QLatin1String("default.qml");
    QUrl settingsUrl;
    bool builtIn = false; //just for keeping track of the warning
    if (override.isEmpty()) {
        QFileInfo fi;
        fi.setFile(QStandardPaths::locate(QStandardPaths::AppDataLocation, defaultFileName));
        if (fi.exists()) {
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
        } else {
            // If different built-in configs are needed per-platform, just apply QFileSelector to the qrc conf.qml path
            fi.setFile(m_confResourcePath + defaultFileName);
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            builtIn = true;
        }
    } else {
        QFileInfo fi;
        fi.setFile(m_confResourcePath + override + QLatin1String(".qml"));
        if (fi.exists()) {
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            builtIn = true;
        } else {
            fi.setFile(QDir(QStandardPaths::locate(QStandardPaths::AppConfigLocation,
                                                   override,
                                                   QStandardPaths::LocateDirectory)),
                       m_confResourcePath);
            if (fi.exists())
                settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            else
                fi.setFile(override);
            if (!fi.exists()) {
                qCritical() << "qml: Couldn't find required configuration file: %s\n",
                    qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath()));
                exit(1);
            }
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
        }
    }

    if (!quiet) {
        qInfo() << "qml: %s\n", QLibraryInfo::build();
        if (builtIn) {
            qInfo() << "qml: Using built-in configuration: %s\n",
                qPrintable(override.isEmpty() ? defaultFileName : override);
        } else {
            qInfo() << "qml: Using configuration: %s\n",
                qPrintable(settingsUrl.isLocalFile()
                               ? QDir::toNativeSeparators(settingsUrl.toLocalFile())
                               : settingsUrl.toString());
        }
    }

    // TODO: When we have better engine control, ban QtQuick* imports on this engine
    QQmlEngine e2;
    QQmlComponent c2(&e2, settingsUrl);
    m_conf.reset(qobject_cast<Config *>(c2.create()));

    if (!m_conf) {
        qCritical() << "qml: Error loading configuration file: %s\n", qPrintable(c2.errorString());
        exit(1);
    }
}

void QmlRuntime::listConfFiles()
{
    const QDir confResourceDir(m_confResourcePath);
    qInfo() << "%s\n", qPrintable(QCoreApplication::translate("main", "Built-in configurations:"));
    for (const QFileInfo &fi : confResourceDir.entryInfoList(QDir::Files))
        qInfo() << "  %s\n", qPrintable(fi.baseName());
    qInfo() << "%s\n", qPrintable(QCoreApplication::translate("main", "Other configurations:"));
    bool foundOther = false;
    const QStringList otherLocations = QStandardPaths::standardLocations(
        QStandardPaths::AppConfigLocation);
    for (const auto &confDirPath : otherLocations) {
        const QDir confDir(confDirPath);
        for (const QFileInfo &fi : confDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            foundOther = true;
            if (m_verboseMode)
                qInfo() << "  %s\n", qPrintable(fi.absoluteFilePath());
            else
                qInfo() << "  %s\n", qPrintable(fi.baseName());
        }
    }
    if (!foundOther)
        qInfo() << "  %s\n", qPrintable(QCoreApplication::translate("main", "none"));
    if (m_verboseMode) {
        qInfo() << "%s\n", qPrintable(QCoreApplication::translate("main", "Checked in:"));
        for (const auto &confDirPath : otherLocations)
            qInfo() << "  %s\n", qPrintable(confDirPath);
    }
}
