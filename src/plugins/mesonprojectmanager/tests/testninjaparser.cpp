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

#include <project/outputparsers/ninjaparser.h>
#include <iostream>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;

struct TestData
{
    QString ninjaSTDO;
    std::vector<std::size_t> steps;
    std::size_t warnings;
    std::size_t errors;
};
Q_DECLARE_METATYPE(TestData);

static const TestData sample1{
    R"([1/2] Compiling C object 'SimpleCProject@exe/main.c.o'
../../qt-creator/src/plugins/mesonprojectmanager/tests/resources/SimpleCproject/main.c: In function ‘main’:
../../qt-creator/src/plugins/mesonprojectmanager/tests/resources/SimpleCproject/main.c:1:14: warning: unused parameter ‘argc’ [-Wunused-parameter]
    1 | int main(int argc, char** argv)
      |          ~~~~^~~~
../../qt-creator/src/plugins/mesonprojectmanager/tests/resources/SimpleCproject/main.c:1:27: warning: unused parameter ‘argv’ [-Wunused-parameter]
    1 | int main(int argc, char** argv)
      |                    ~~~~~~~^~~~
[2/2] Linking target SimpleCProject)",
    {50, 100},
    0Ul,
    0Ul};

static const TestData sample2{
    R"([1/10] should skip second this one ->[30/10]
 [2/10]   <- should skip this one
[11111]   <- should skip this one
[a/a]     <- should skip this one
[1]       <- should skip this one
[ 1 / 1 ] <- should skip this one
[10/10    <- should skip this one
10/10]    <- should skip this one
[-1/10]   <- should skip this one
[10/10]
)",
    {10, 100},
    0Ul,
    0Ul};

void feedParser(NinjaParser &parser, const TestData &data)
{
    auto lines = data.ninjaSTDO.split('\n');
    std::for_each(std::cbegin(lines), std::cend(lines), [&](const auto &line) {
        parser.handleLine(line, Utils::OutputFormat::StdOutFormat);
    });
}

class ANinjaParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void extractProgress_data()
    {
        QTest::addColumn<TestData>("testData");
        QTest::newRow("sample1") << sample1;
        QTest::newRow("sample2") << sample2;
    }
    void extractProgress()
    {
        QFETCH(TestData, testData);
        std::vector<std::size_t> steps;
        NinjaParser parser;
        connect(&parser, &NinjaParser::reportProgress, [&](int progress) {
            steps.push_back(progress);
        });
        feedParser(parser, testData);
        QVERIFY(steps == testData.steps);
    }
    void cleanupTestCase() {}
};

QTEST_MAIN(ANinjaParser)
#include "testninjaparser.moc"
