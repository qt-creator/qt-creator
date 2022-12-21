// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtestcase.h>
#include <QtTest>

#include <utils/expected.h>
#include <utils/filepath.h>

using namespace Utils;

class tst_expected : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void tryMonads()
    {
        FilePath p = "idontexists.ne";

        auto result = p.fileContents()
                          .and_then([](auto) { return expected_str<QByteArray>{}; })
                          .or_else([](auto error) {
                              return expected_str<QByteArray>(
                                  make_unexpected(QString("Error: " + error)));
                          })
                          .transform_or([](auto error) -> QString {
                              return QString(QString("More Info: ") + error);
                          });

        QVERIFY(!result);
    }
};
QTEST_GUILESS_MAIN(tst_expected)

#include "tst_expected.moc"
