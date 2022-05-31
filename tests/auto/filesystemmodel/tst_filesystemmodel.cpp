
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
