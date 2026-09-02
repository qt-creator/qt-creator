// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "tst_inlib.h"

#include <QObject>
#include <QTest>

class InLibTest : public QObject
{
    Q_OBJECT

private slots:
    void tst_first() {}
    void tst_second() {}
};

int runInLibTests(int argc, char *argv[])
{
    return QTest::qExec(new InLibTest, argc, argv);
}

#include "tst_inlib.moc"
