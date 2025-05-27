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

#include <cstdlib>
#include <iostream>

QmlBase::QmlBase(int &argc, char **argv, QObject *parent)
    : QObject{parent}
    , m_args({.argc = argc, .argv = argv})
{
    m_argParser.setApplicationDescription("QML Runtime Provider for QDS");
    m_argParser.addOption({"qml-puppet", "Run QML Puppet (default)"});
    m_argParser.addOption({"qml-renderer", "Run QML Renderer"});
#ifdef ENABLE_INTERNAL_QML_RUNTIME
    m_argParser.addOption({"qml-runtime", "Run QML Runtime"});
#endif
    m_argParser.addOption({"test", "Run test mode"});
}

int QmlBase::startTestMode()
{
    qDebug() << "Test mode is not implemented for this type of runner";
    return 0;
}

void QmlBase::initQmlRunner()
{
    QmlDesigner::Internal::QmlPrivateGate::registerFixResourcePathsForObjectCallBack();

    if (const QString defaultFontFamily = qEnvironmentVariable(QMLPUPPET_ENV_DEFAULT_FONT_FAMILY);
        !defaultFontFamily.isEmpty()) {
        if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()) != nullptr) {
            QGuiApplication::setFont(QFont{defaultFontFamily});
        }
    }

    if (const QString mcuFontsFolder = qEnvironmentVariable(QMLPUPPET_ENV_MCU_FONTS_DIR);
        !mcuFontsFolder.isEmpty()) {
        registerFonts(mcuFontsFolder);
    }
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
