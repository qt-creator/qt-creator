// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlbase.h"
#include "qmlprivategate/qmlprivategate.h"

#include <QCommandLineOption>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QObject>
#include <QUrl>
#include <QtEnvironmentVariables>
#include <QtLogging>
#include "private/qfontdatabase_p.h"

#include <cstdlib>
#include <iostream>

QmlBase::QmlBase(int &argc, char **argv, QObject *parent)
    : QObject{parent}
    , m_args({.argc = argc, .argv = argv})
{
    m_argParser.setApplicationDescription("QML Runtime Provider for QDS");
    addCommandLineOptions(m_argParser);
}

int QmlBase::startTestMode()
{
    qDebug() << "Test mode is not implemented for this type of runner";
    return 0;
}

void QmlBase::initQmlRunner()
{
    QmlDesigner::Internal::QmlPrivateGate::registerFixResourcePathsForObjectCallBack();

    if (const auto& defaultFontFamily = m_mcuOptions.defaultFont) {
        if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()) != nullptr)
            QGuiApplication::setFont(QFont{*defaultFontFamily});
    }

    if (const auto& fontsFolder = m_mcuOptions.fontDirectory)
        registerFonts(*fontsFolder);
}

int QmlBase::run()
{
    populateParser();
    initCoreApp();

    if (!m_coreApp) { //default to QGuiApplication
        createCoreApp<QGuiApplication>();
        qWarning() << "CoreApp is not initialized! Falling back to QGuiApplication!";
    }

    initParser();
    initQmlRunner();

    if (m_mcuOptions.isSpark()) {
        if (m_mcuOptions.fontFile.has_value())
            setFontFileExclusive(*m_mcuOptions.fontFile);
    }

    return QCoreApplication::exec();
}

void QmlBase::initParser()
{
    const QCommandLineOption optHelp = m_argParser.addHelpOption();

    if (!m_argParser.parse(QCoreApplication::arguments())) {
        std::cout << "Error: " << m_argParser.errorText().toStdString() << "\n";
        if (m_argParser.errorText().contains("qml-runtime")) {
            std::cout << "Note: --qml-runtime is only available when Qt is 6.4.x or higher\n";
        }
        std::cout << "\n";

        m_argParser.showHelp(1);
    } else if (m_argParser.isSet(optHelp)) {
        m_argParser.showHelp(0);
    } else if (m_argParser.isSet("test")) {
        exit(startTestMode());
    }

    m_mcuOptions.parse(m_argParser);
}

void QmlBase::setFontFileExclusive(const QString &fontFile)
{
    QString fontFamily;
    if (const int fontId = QFontDatabase::addApplicationFont(fontFile); fontId >= 0 ) {
        const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        QFontDatabase::removeApplicationFont(fontId);
        fontFamily = fontFamilies.first();
    }

    auto *db = QFontDatabasePrivate::instance();
    db->fallbacksCache.clear();

    for (auto fam : QFontDatabase::families()) {
        if (fam == fontFamily)
            continue;
        QFont::insertSubstitution(fam, fontFamily);
    }

    db->clearFamilies();
    db->populated = true;

    qGuiApp->setFont(QFont(fontFamily));
    QFontDatabase::addApplicationFont(fontFile);
}

void QmlBase::addCommandLineOptions(QCommandLineParser &parser)
{
    parser.addOption({"qml-puppet", "Run QML Puppet (default)"});
    parser.addOption({"qml-renderer", "Run QML Renderer"});
#ifdef ENABLE_INTERNAL_QML_RUNTIME
    parser.addOption({"qml-runtime", "Run QML Runtime"});
#endif
    parser.addOption({"test", "Run test mode"});

    parser.addOption({"mcu-font-file", "MCU font file when spark engine is used", "String"});
    parser.addOption({"mcu-font-engine", "MCU font engine beeing used", "String"});
    parser.addOption({"mcu-font-dir", "MCU mode enabled", "String"});
    parser.addOption({"mcu-default-font", "Default Font Family for mcu projects", "String"});
}

void QmlBase::registerFonts(const QDir &dir)
{
    // Autoregister all fonts found inside the dir
    QDirIterator
        itr{dir.absolutePath(), {"*.ttf", "*.otf"}, QDir::Files, QDirIterator::Subdirectories};
    while (itr.hasNext()) {
        QFontDatabase::addApplicationFont(itr.next());
    }
}

bool QmlBase::McuOptions::isSpark() const
{
    return fontEngine.value_or("Static") == QStringLiteral("Spark");
}

void QmlBase::McuOptions::parse(const QCommandLineParser &parser)
{
    if (parser.isSet("mcu-font-engine"))
        fontEngine = parser.value("mcu-font-engine");
    if (parser.isSet("mcu-font-file"))
        fontFile = parser.value("mcu-font-file");
    if (parser.isSet("mcu-default-font"))
        defaultFont = parser.value("mcu-default-font");
    if (parser.isSet("mcu-font-dir"))
        fontDirectory = parser.value("mcu-font-dir");
}
