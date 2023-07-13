// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassembly_test.h"

#include "webassemblyemsdk.h"
#include "webassemblyrunconfiguration.h"

#include <utils/environment.h>

#include <QTest>

using namespace Utils;

namespace WebAssembly::Internal {

void WebAssemblyTest::testEmSdkEnvParsing()
{
    QFETCH(QString, emSdkEnvOutput);
    QFETCH(int, osType);
    QFETCH(int, pathCount);
    QFETCH(QString, emsdk);
    QFETCH(QString, em_config);

    Environment env{OsType(osType)};
    WebAssemblyEmSdk::parseEmSdkEnvOutputAndAddToEnv(emSdkEnvOutput, env);

    QVERIFY(env.path().count() == pathCount);
    QCOMPARE(env.value("EMSDK"), emsdk);
    QCOMPARE(env.value("EM_CONFIG"), em_config);
}

void WebAssemblyTest::testEmSdkEnvParsing_data()
{
    // Output of "emsdk_env"
    QTest::addColumn<QString>("emSdkEnvOutput");
    QTest::addColumn<int>("osType");
    QTest::addColumn<int>("pathCount");
    QTest::addColumn<QString>("emsdk");
    QTest::addColumn<QString>("em_config");

    QTest::newRow("windows") << R"(
Adding directories to PATH:
PATH += C:\Users\user\dev\emsdk
PATH += C:\Users\user\dev\emsdk\upstream\emscripten
PATH += C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin
PATH += C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit
PATH += C:\Users\user\dev\emsdk\java\8.152_64bit\bin

Setting environment variables:
PATH = C:\Users\user\dev\emsdk;C:\Users\user\dev\emsdk\upstream\emscripten;C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin;C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit;C:\Users\user\dev\emsdk\java\8.152_64bit\bin;...other_stuff...
EMSDK = C:/Users/user/dev/emsdk
EM_CONFIG = C:\Users\user\dev\emsdk\.emscripten
EM_CACHE = C:/Users/user/dev/emsdk/upstream/emscripten\cache
EMSDK_NODE = C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin\node.exe
EMSDK_PYTHON = C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit\python.exe
JAVA_HOME = C:\Users\user\dev\emsdk\java\8.152_64bit
      )" << int(OsTypeWindows) << 6 << "C:/Users/user/dev/emsdk" << "C:\\Users\\user\\dev\\emsdk\\.emscripten";

    QTest::newRow("linux") << R"(
Adding directories to PATH:
PATH += /home/user/dev/emsdk
PATH += /home/user/dev/emsdk/upstream/emscripten
PATH += /home/user/dev/emsdk/node/12.18.1_64bit/bin

Setting environment variables:
PATH = /home/user/dev/emsdk:/home/user/dev/emsdk/upstream/emscripten:/home/user/dev/emsdk/node/12.18.1_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
EMSDK = /home/user/dev/emsdk
EM_CONFIG = /home/user/dev/emsdk/.emscripten
EM_CACHE = /home/user/dev/emsdk/upstream/emscripten/cache
EMSDK_NODE = /home/user/dev/emsdk/node/12.18.1_64bit/bin/node
      )" << int(OsTypeLinux) << 3 << "/home/user/dev/emsdk" << "/home/user/dev/emsdk/.emscripten";
}

void WebAssemblyTest::testEmrunBrowserListParsing()
{
    QFETCH(QByteArray, emrunOutput);
    QFETCH(WebBrowserEntries, expectedBrowsers);

    QCOMPARE(parseEmrunOutput(emrunOutput), expectedBrowsers);
}

void WebAssemblyTest::testEmrunBrowserListParsing_data()
{
    QTest::addColumn<QByteArray>("emrunOutput");
    QTest::addColumn<WebBrowserEntries>("expectedBrowsers");

    QTest::newRow("emsdk 1.39.8")
// Output of "emrun --list_browsers"
            << QByteArray(
R"(emrun has automatically found the following browsers in the default install locations on the system:

  - firefox: Mozilla Firefox
  - chrome: Google Chrome

You can pass the --browser <id> option to launch with the given browser above.
Even if your browser was not detected, you can use --browser /path/to/browser/executable to launch with that browser.

)")
            << WebBrowserEntries({
                {QLatin1String("firefox"), QLatin1String("Mozilla Firefox")},
                {QLatin1String("chrome"), QLatin1String("Google Chrome")}});

    QTest::newRow("emsdk 2.0.14")
            << QByteArray(
R"(emrun has automatically found the following browsers in the default install locations on the system:

  - firefox: Mozilla Firefox 96.0.0.8041
  - chrome: Google Chrome 97.0.4692.71

You can pass the --browser <id> option to launch with the given browser above.
Even if your browser was not detected, you can use --browser /path/to/browser/executable to launch with that browser.

)")
            << WebBrowserEntries({
                {QLatin1String("firefox"), QLatin1String("Mozilla Firefox 96.0.0.8041")},
                {QLatin1String("chrome"), QLatin1String("Google Chrome 97.0.4692.71")}});
}

} // namespace WebAssembly::Internal
