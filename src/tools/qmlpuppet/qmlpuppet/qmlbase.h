// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFont>
#include <QQmlApplicationEngine>

class QmlBase : public QObject
{
    Q_OBJECT
public:
    // These constants should be kept in sync with their counterparts in qmlprojectconstants.h
    static constexpr char QMLPUPPET_ENV_PROJECT_ROOT[] = "QMLPUPPET_PROJECT_ROOT";

    static void addCommandLineOptions(QCommandLineParser &parser);
    static void registerFonts(const QDir &dir);

    struct AppArgs
    {
    public:
        int argc;
        char **argv;
    };

    struct McuOptions
    {
        std::optional<QString> fontEngine = std::nullopt;
        std::optional<QString> fontFile = std::nullopt;

        std::optional<QString> defaultFont = std::nullopt;
        std::optional<QString> fontDirectory = std::nullopt;

        bool isSpark() const;
        void parse(const QCommandLineParser &parser);
    };

    QmlBase(int &argc, char **argv, QObject *parent = nullptr);

    int run();

    QSharedPointer<QCoreApplication> coreApp() const { return m_coreApp; }

protected:
    virtual void initCoreApp() = 0;
    virtual void populateParser() = 0;
    virtual void initQmlRunner();
    virtual int startTestMode();

    template<typename T>
    void createCoreApp()
    {
        m_coreApp.reset(new T(m_args.argc, m_args.argv));
    }

    QSharedPointer<QCoreApplication> m_coreApp;
    QCommandLineParser m_argParser;
    QSharedPointer<QQmlApplicationEngine> m_qmlEngine;

    AppArgs m_args;
    McuOptions m_mcuOptions;

private:
    void initParser();
    void setFontFileExclusive(const QString &fontFile);
};
