
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../../src/shared/modeltest/modeltest.h"

#include <utils/filesystemmodel/filesystemmodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QtTest>

#ifndef WITH_TESTS
#define WITH_TESTS
#endif

class FileSystemModelTest : public ModelTest
{
    Q_OBJECT

public:
    FileSystemModelTest()
        : ModelTest(new Utils::FileSystemModel())
    {
        static_cast<Utils::FileSystemModel*>(this->model())->setRootPath("/");
    }

private slots:
    void qTestWithTestClass() {
        Utils::FileSystemModel modelToBeTested;
        modelToBeTested.setRootPath("/");
        auto tester = new QAbstractItemModelTester(&modelToBeTested);
    }

    void testWithSortFilterProxyModel() {
        Utils::FileSystemModel modelToBeTested;
        QSortFilterProxyModel proxyModel;

        proxyModel.setSourceModel(&modelToBeTested);
        proxyModel.setSortRole(Qt::UserRole + 50);
        proxyModel.sort(0);

        modelToBeTested.setRootPath("/");
        auto tester = new QAbstractItemModelTester(&proxyModel);
    }
};

QTEST_MAIN(FileSystemModelTest)

#include "tst_filesystemmodel.moc"
