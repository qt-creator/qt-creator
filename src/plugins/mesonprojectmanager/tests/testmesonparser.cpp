/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <project/outputparsers/mesonoutputparser.h>
#include <iostream>
#include <projectexplorer/taskhub.h>
#include <utils/fileinprojectfinder.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;

struct TestData
{
    QString stdo;
    QStringList linesToExtract;
};
Q_DECLARE_METATYPE(TestData);

static const TestData sample1{
    R"(ERROR: Value "C++11" for combo option is not one of the choices. Possible choices are: "none", "c++98", "c++03", "c++11", "c++14", "c++17", "c++1z", "c++2a", "gnu++03", "gnu++11", "gnu++14", "gnu++17", "gnu++1z", "gnu++2a".
ERROR: Value "true" for combo option is not one of the choices. Possible choices are: "shared", "static", "both".
ERROR: Value shared is not boolean (true or false).
)",
    {
        R"(ERROR: Value "C++11" for combo option is not one of the choices. Possible choices are: "none", "c++98", "c++03", "c++11", "c++14", "c++17", "c++1z", "c++2a", "gnu++03", "gnu++11", "gnu++14", "gnu++17", "gnu++1z", "gnu++2a".)",
        R"(ERROR: Value "true" for combo option is not one of the choices. Possible choices are: "shared", "static", "both".)",
        R"(ERROR: Value shared is not boolean (true or false).)",
    }};

static const TestData sample2{
    R"(../core/meson.build:163:0: ERROR: could not get https://gitlab.com/mdds/mdds/-/archive/1.6.0/mdds-1.6.0.tar.gz is the internet available?)",
    {R"(../core/meson.build:163:0: ERROR: could not get https://gitlab.com/mdds/mdds/-/archive/1.6.0/mdds-1.6.0.tar.gz is the internet available?)"}};

static const TestData sample3{
    R"!(
The Meson build system
Version: 0.54.0
Source dir: /home/jeandet/Documents/prog/SciQLop
Build dir: /home/jeandet/Documents/prog/build-SciQLop-Desktop-debug
Build type: native build
WARNING: Unknown options: "unknown"
The value of new options can be set with:
meson setup <builddir> --reconfigure -Dnew_option=new_value ...
Project name: SciQLOP
Project version: 1.1.0
C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
Host machine cpu family: x86_64
Host machine cpu: x86_64
WARNING: rcc dependencies will not work reliably until this upstream issue is fixed: https://bugreports.qt.io/browse/QTBUG-45460
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Dependency qt5 found: YES 5.13.2 (cached)
Found pkg-config: /usr/bin/pkg-config (1.6.3)
Found CMake: /usr/bin/cmake (3.17.1)
Run-time dependency cpp_utils found: NO (tried pkgconfig and cmake)
Looking for a fallback subproject for the dependency cpp_utils

|Executing subproject cpp_utils method meson
|
|Project name: cpp_utils
|Project version: undefined
|C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
|C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
|Program cppcheck found: YES (/usr/bin/cppcheck)
|Run-time dependency catch2 found: NO (tried pkgconfig and cmake)
|Looking for a fallback subproject for the dependency catch2
|
||Executing subproject catch2 method meson
||
||Project name: catch2
||Project version: 2.9.0
||C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
||C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
||Build targets in project: 1
||Subproject catch2 finished.
|
|Dependency catch2 from subproject subprojects/catch2 found: YES 2.9.0
|Build targets in project: 9
|Subproject cpp_utils finished.

Dependency cpp_utils from subproject subprojects/cpp_utils found: YES undefined
Program moc-qt5 found: YES (/usr/bin/moc-qt5)
Program rcc-qt5 found: YES (/usr/bin/rcc-qt5)
Run-time dependency catalogicpp found: NO (tried pkgconfig and cmake)
Looking for a fallback subproject for the dependency catalogicpp

|Executing subproject catalogicpp method meson
|
|Project name: catalogicpp
|Project version: 1.0.0
|C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
|C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
|Dependency nlohmann_json found: YES 3.7.3 (cached)
|Run-time dependency stduuid found: NO (tried pkgconfig and cmake)
|Looking for a fallback subproject for the dependency stduuid
|
||Executing subproject stduuid method meson
||
||Project name: stduuid
||Project version: 1.0.0
||C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
||C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
||Build targets in project: 9
||Subproject stduuid finished.
|
|Dependency stduuid from subproject subprojects/stduuid found: YES 1.0.0
|Run-time dependency GTest found: NO (tried pkgconfig and system)
|Looking for a fallback subproject for the dependency gtest
|
||Executing subproject gtest method meson
||
||Project name: gtest
||Project version: 1.8.1
||C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
||C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
||Dependency threads found: YES unknown (cached)
||Dependency threads found: YES unknown (cached)
||Dependency threads found: YES unknown (cached)
||Dependency threads found: YES unknown (cached)
||Build targets in project: 9
||Subproject gtest finished.
|
|Dependency gtest from subproject subprojects/gtest found: YES 1.8.1
|Build targets in project: 13
|Subproject catalogicpp finished.

Dependency catalogicpp from subproject subprojects/catalogicpp found: YES 1.0.0
Dependency pybind11 found: YES 2.5.0 (cached)
Run-time dependency timeseries found: NO (tried pkgconfig and cmake)
Looking for a fallback subproject for the dependency TimeSeries

|Executing subproject TimeSeries method meson
|
|Project name: TimeSeries
|Project version: 0.1
|C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
|C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
|Dependency gtest from subproject subprojects/gtest found: YES 1.8.1
|Run-time dependency benchmark found: NO (tried pkgconfig and cmake)
|Looking for a fallback subproject for the dependency benchmark
|
||Executing subproject google-benchmark method meson
||
||Project name: benchmark
||Project version: 1.4.1
||C++ compiler for the host machine: /usr/lib64/ccache/g++ (gcc 10.0.1 "g++ (GCC) 10.0.1 20200328 (Red Hat 10.0.1-0.11)")
||C++ linker for the host machine: /usr/lib64/ccache/g++ ld.bfd 2.34-2
||Dependency threads found: YES unknown (cached)
||Build targets in project: 15
||Subproject google-benchmark finished.
|
|Dependency benchmark from subproject subprojects/google-benchmark found: YES 1.4.1
|Build targets in project: 22
|Subproject TimeSeries finished.

Dependency TimeSeries from subproject subprojects/TimeSeries found: YES 0.1
Dependency cpp_utils found: YES undefined (cached)
Detecting Qt5 tools
 moc: YES (/usr/lib64/qt5/bin/moc, 5.13.2)
 uic: YES (/usr/lib64/qt5/bin/uic, 5.13.2)
 rcc: YES (/usr/lib64/qt5/bin/rcc, 5.13.2)
 lrelease: YES (/usr/lib64/qt5/bin/lrelease, 5.13.2)
Program python3 found: YES (/usr/bin/python3)
Program shiboken2 found: YES (/usr/bin/shiboken2)
Program python4Ul3 (PySide2, shiboken2, shiboken2_generator, numpy) found: YES (/usr/bin/python3) modules: PySide2, shiboken2, shiboken2_generator, numpy
Dependency python found: YES (pkgconfig)
WARNING: Project targeting '>=0.51.0' but tried to use feature introduced in '0.53.0': embed arg in python_installation.dependency
Dependency python found: YES (pkgconfig)
Program cppcheck found: YES (/usr/bin/cppcheck)
Build targets in project: 45
WARNING: Project specifies a minimum meson_version '>=0.51.0' but uses features which were added in newer versions:
 * 0.53.0: {'embed arg in python_installation.dependency'}
WARNING: Deprecated features used:
 * 0.53.0: {'embed arg in python_installation.dependency'}

SciQLOP 1.1.0

  Subprojects
          TimeSeries: YES
         catalogicpp: YES
              catch2: YES
           cpp_utils: YES
    google-benchmark: YES
               gtest: YES
             stduuid: YES

Found ninja-1.10.0 at /usr/bin/ninja
)!",
    {
        R"(WARNING: Unknown options: "unknown")",
        R"(The value of new options can be set with:)",
        R"(meson setup <builddir> --reconfigure -Dnew_option=new_value ...)",
        R"(WARNING: rcc dependencies will not work reliably until this upstream issue is fixed: https://bugreports.qt.io/browse/QTBUG-45460)",
        R"(WARNING: Project targeting '>=0.51.0' but tried to use feature introduced in '0.53.0': embed arg in python_installation.dependency)",
        R"(WARNING: Project specifies a minimum meson_version '>=0.51.0' but uses features which were added in newer versions:)",
        R"( * 0.53.0: {'embed arg in python_installation.dependency'})",
        R"(WARNING: Deprecated features used:)",
        R"( * 0.53.0: {'embed arg in python_installation.dependency'})",
    }};

static const TestData sample4{
    R"([0/1] Regenerating build files.
The Meson build system
Version: 0.54.0
Source dir: /home/jeandet/Documents/prog/SciQLop
Build dir: /home/jeandet/Documents/prog/build-SciQLop-Desktop-debug
Build type: native build

../SciQLop/meson.build:21:0: ERROR: Expecting eof got rparen.
)
^

A full log can be found at /home/jeandet/Documents/prog/build-SciQLop-Desktop-debug/meson-logs/meson-log.txt
FAILED: build.ninja
/usr/bin/meson --internal regenerate /home/jeandet/Documents/prog/SciQLop /home/jeandet/Documents/prog/build-SciQLop-Desktop-debug --backend ninja
ninja: error: rebuilding 'build.ninja': subcommand failed
)",
    {R"(../SciQLop/meson.build:21:0: ERROR: Expecting eof got rparen.)"}};

QStringList feedParser(MesonOutputParser &parser, const TestData &data)
{
    QStringList extractedLines;
    auto lines = data.stdo.split('\n');
    std::for_each(std::cbegin(lines), std::cend(lines), [&](const auto &line) {
        Utils::OutputLineParser::Result result
            = parser.handleLine(line, Utils::OutputFormat::StdOutFormat);
        if (result.status == Utils::OutputLineParser::Status::Done)
            extractedLines << line;
    });
    return extractedLines;
}

class AMesonOutputParser : public QObject
{
    Q_OBJECT

    std::size_t task_count;
    std::size_t error_count;

private slots:
    void initTestCase() {}
    void extractProgress_data()
    {
        QTest::addColumn<TestData>("testData");
        QTest::newRow("sample1") << sample1;
        QTest::newRow("sample2") << sample2;
        QTest::newRow("sample3") << sample3;
        QTest::newRow("sample4") << sample4;
    }
    void extractProgress()
    {
        QFETCH(TestData, testData);
        error_count = 0;
        task_count = 0;
        MesonOutputParser parser;
        auto extracetdLines = feedParser(parser, testData);
        QCOMPARE(extracetdLines, testData.linesToExtract);
    }
    void cleanupTestCase() {}
};

QTEST_MAIN(AMesonOutputParser)

#include "testmesonparser.moc"
