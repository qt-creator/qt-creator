// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <updateinfo/updateinfotools.h>

Q_LOGGING_CATEGORY(updateLog, "qtc.updateinfo", QtWarningMsg)

class tst_UpdateInfo : public QObject
{
    Q_OBJECT

private slots:
    void updates_data();
    void updates();
};

void tst_UpdateInfo::updates_data()
{
    QTest::addColumn<QString>("updateXml");
    QTest::addColumn<QString>("packageXml");
    QTest::addColumn<QList<Update>>("xupdates");
    QTest::addColumn<QList<QtPackage>>("xpackages");

    QTest::newRow("updates and packages")
        << R"raw(<?xml version="1.0"?>
        <updates>
        <update name="Qt Design Studio 3.2.0" version="3.2.0-0-202203291247" size="3113234690" id="qt.tools.qtdesignstudio"/>
        </updates>)raw"
        << R"raw(<?xml version="1.0"?>
        <availablepackages>
        <package name="qt.qt6.621" displayname="Qt 6.2.1" version="6.2.1-0-202110220854"/>
        <package name="qt.qt5.5152" displayname="Qt 5.15.2" version="5.15.2-0-202011130607" installedVersion="5.15.2-0-202011130607"/>
        <package name="qt.qt6.610" displayname="Qt 6.1.0-beta1" version="6.1.0-0-202105040922"/>
        </availablepackages>)raw"
        << QList<Update>({{"Qt Design Studio 3.2.0", "3.2.0-0-202203291247"}})
        << QList<QtPackage>(
               {{"Qt 6.2.1", QVersionNumber::fromString("6.2.1-0-202110220854"), false, false},
                {"Qt 6.1.0-beta1", QVersionNumber::fromString("6.1.0-0-202105040922"), false, true},
                {"Qt 5.15.2", QVersionNumber::fromString("5.15.2-0-202011130607"), true, false}});
}

void tst_UpdateInfo::updates()
{
    QFETCH(QString, updateXml);
    QFETCH(QString, packageXml);
    QFETCH(QList<Update>, xupdates);
    QFETCH(QList<QtPackage>, xpackages);

    const QList<Update> updates = availableUpdates(updateXml);
    QCOMPARE(updates, xupdates);
    const QList<QtPackage> packages = availableQtPackages(packageXml);
    QCOMPARE(packages, xpackages);
}

QTEST_GUILESS_MAIN(tst_UpdateInfo)

#include "tst_updateinfo.moc"
