// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpuppet.h"
#include "configcrashpad.h"

#ifdef MULTILANGUAGE_TRANSLATIONPROVIDER
#include <sqlitelibraryinitializer.h>
#endif

#include <app/app_version.h>
#include <qml2puppet/import3d/import3d.h>

#include <qt5nodeinstanceclientproxy.h>

#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSettings>

#if defined(Q_OS_WIN) && defined(QT_NO_DEBUG)
    #include <Windows.h>
#endif

void QmlPuppet::initCoreApp()
{
    // Since we always render text into an FBO, we need to globally disable
    // subpixel antialiasing and instead use gray.
    qputenv("QSG_DISTANCEFIELD_ANTIALIASING", "gray");
#ifdef Q_OS_MACOS
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
#endif
#ifdef MULTILANGUAGE_TRANSLATIONPROVIDER
    Sqlite::LibraryInitializer::initialize();
#endif

    //If a style different from Desktop is set we have to use QGuiApplication
    bool useGuiApplication = (!qEnvironmentVariableIsSet("QMLDESIGNER_FORCE_QAPPLICATION")
                              || qgetenv("QMLDESIGNER_FORCE_QAPPLICATION") != "true")
                             && qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")
                             && qgetenv("QT_QUICK_CONTROLS_STYLE") != "Desktop";
#ifndef QT_GUI_LIB
    createCoreApp<QCoreApplication>();
#else
#if defined QT_WIDGETS_LIB
    if (!useGuiApplication)
        createCoreApp<QApplication>();
    else
#endif //QT_WIDGETS_LIB
        createCoreApp<QGuiApplication>();
#endif //QT_GUI_LIB
}

int QmlPuppet::startTestMode()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem {\n}\n", QUrl::fromLocalFile("test.qml"));

    if (!QSharedPointer<QObject>(component.create())) {
        qDebug() << "Basic QtQuick 2.0 not working...";
        qDebug() << component.errorString();
        return -1;
    }

    qDebug() << "Basic QtQuick 2.0 working...";
    return 0;
}

void QmlPuppet::populateParser()
{
    // we're not using the commandline parser but just populating the help text
    m_argParser.addOptions(
        {{"readcapturedstream", "Read captured stream.", "inputStream, [outputStream]"},
         {"import3dAsset", "Import 3d asset.", "sourceAsset, outDir, importOptJson"}});
}

// should be in sync with coreplugin/icore.cpp -> FilePath ICore::crashReportsPath()
// and src\app\main.cpp
QString crashReportsPath()
{
    QSettings settings(
        QSettings::IniFormat,
        QSettings::UserScope,
        QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
        QLatin1String(Core::Constants::IDE_CASED_ID));

#if defined(Q_OS_MACOS)
        return QFileInfo(settings.fileName()).path() + "/crashpad_reports";
#else
        return QCoreApplication::applicationDirPath()
                + '/' + RELATIVE_LIBEXEC_PATH + "crashpad_reports";
#endif
}

void QmlPuppet::initQmlRunner()
{
    if (m_coreApp->arguments().count() < 2
        || (m_argParser.isSet("readcapturedstream") && m_coreApp->arguments().count() < 3)
        || (m_argParser.isSet("import3dAsset") && m_coreApp->arguments().count() < 6)
        || (!m_argParser.isSet("readcapturedstream") && m_coreApp->arguments().count() < 4)) {
        qDebug() << "Wrong argument count: " << m_coreApp->arguments().count();
        m_argParser.showHelp(1);
    }

    if (m_argParser.isSet("readcapturedstream") && m_coreApp->arguments().count() > 2) {
        QString fileName = m_argParser.value("readcapturedstream");
        if (!QFileInfo::exists(fileName)) {
            qDebug() << "Input stream does not exist:" << fileName;
            exit(-1);
        }

        if (m_coreApp->arguments().count() > 3) {
            fileName = m_coreApp->arguments().at(3);
            if (!QFileInfo::exists(fileName)) {
                qDebug() << "Output stream does not exist:" << fileName;
                exit(-1);
            }
        }
    }

    if (m_argParser.isSet("import3dAsset")) {
        QString sourceAsset = m_coreApp->arguments().at(2);
        QString outDir = m_coreApp->arguments().at(3);
        QString options = m_coreApp->arguments().at(4);

        Import3D::import3D(sourceAsset, outDir, options);
    }

    startCrashpad(QCoreApplication::applicationDirPath()
                  + '/' + RELATIVE_LIBEXEC_PATH, crashReportsPath());

    new QmlDesigner::Qt5NodeInstanceClientProxy(m_coreApp.get());

#if defined(Q_OS_WIN) && defined(QT_NO_DEBUG)
    SetErrorMode(SEM_NOGPFAULTERRORBOX); //We do not want to see any message boxes
#endif

    if (m_argParser.isSet("readcapturedstream"))
        exit(0);
}
