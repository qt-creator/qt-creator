/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <sqliteglobal.h>

#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QLoggingCategory>

#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

#ifdef WITH_BENCHMARKS
#include <benchmark/benchmark.h>
#endif

int main(int argc, char *argv[])
{
    const QString temporayDirectoryPath = QDir::tempPath() +"/QtCreator-UnitTests-XXXXXX";
    Utils::TemporaryDirectory::setMasterTemporaryDirectory(temporayDirectoryPath);
    qputenv("TMPDIR", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());
    qputenv("TEMP", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());

    QCoreApplication application(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("*.info=false\n*.debug=false\n*.warning=true"));

    testing::InitGoogleTest(&argc, argv);
#ifdef WITH_BENCHMARKS
    benchmark::Initialize(&argc, argv);
#endif

    int testsHaveErrors = RUN_ALL_TESTS();

#ifdef WITH_BENCHMARKS
    if (testsHaveErrors == 0  && application.arguments().contains(QStringLiteral("--with-benchmarks")))
        benchmark::RunSpecifiedBenchmarks();
#endif

    return testsHaveErrors;
}
