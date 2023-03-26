// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>

#include <QFileInfo>
#include <QFileOpenEvent>
#include <QLibraryInfo>
#include <QSurfaceFormat>

#include <QCommandLineParser>

#include <QStandardPaths>
#include <QTranslator>

#include <QSharedPointer>

#include "appmetadata.h"
#include <iostream>

#include <QApplication>
class QmlBase : public QObject
{
    Q_OBJECT
public:
    struct AppArgs
    {
    public:
        int argc;
        char **argv;
    };

    QmlBase(int &argc, char **argv, QObject *parent = nullptr)
        : QObject{parent}
        , m_args({argc, argv})
    {
        m_argParser.setApplicationDescription("QML Runtime Provider for QDS");
        m_argParser.addOption({"qml-puppet", "Run QML Puppet (default)"});
#ifdef ENABLE_INTERNAL_QML_RUNTIME
        m_argParser.addOption({"qml-runtime", "Run QML Runtime"});
#endif
        m_argParser.addOption({"appinfo", "Print build information"});
        m_argParser.addOption({"test", "Run test mode"});
    }

    int run()
    {
        populateParser();
        initCoreApp();

        if (!m_coreApp) { //default to QGuiApplication
            createCoreApp<QGuiApplication>();
            qWarning() << "CoreApp is not initialized! Falling back to QGuiApplication!";
        }

        initParser();
        initQmlRunner();
        return m_coreApp->exec();
    }

    QSharedPointer<QCoreApplication> coreApp() const { return m_coreApp; }

protected:
    virtual void initCoreApp() = 0;
    virtual void populateParser() = 0;
    virtual void initQmlRunner() = 0;

    virtual int startTestMode()
    {
        qDebug() << "Test mode is not implemented for this type of runner";
        return 0;
    }

    template<typename T>
    void createCoreApp()
    {
        m_coreApp.reset(new T(m_args.argc, m_args.argv));
    }

    QSharedPointer<QCoreApplication> m_coreApp;
    QCommandLineParser m_argParser;
    QSharedPointer<QQmlApplicationEngine> m_qmlEngine;

    AppArgs m_args;

private:
    void initParser()
    {
        QCommandLineOption optHelp = m_argParser.addHelpOption();
        QCommandLineOption optVers = m_argParser.addVersionOption();

        if (!m_argParser.parse(m_coreApp->arguments())) {
            std::cout << "Error: " << m_argParser.errorText().toStdString() << std::endl;
            if (m_argParser.errorText().contains("qml-runtime")) {
                std::cout << "Note: --qml-runtime is only availabe when Qt is 6.4.x or higher"
                          << std::endl;
            }
            std::cout << std::endl;

            m_argParser.showHelp(1);
        } else if (m_argParser.isSet(optVers)) {
            m_argParser.showVersion();
        } else if (m_argParser.isSet(optHelp)) {
            m_argParser.showHelp(0);
        } else if (m_argParser.isSet("appinfo")) {
            QDSMeta::AppInfo::printAppInfo();
        } else if (m_argParser.isSet("test")) {
            exit(startTestMode());
        }
    }
};
