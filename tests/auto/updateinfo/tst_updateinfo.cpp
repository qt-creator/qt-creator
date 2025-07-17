// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <updateinfo/updateinfotools.h>

Q_LOGGING_CATEGORY(updateLog, "qtc.updateinfo", QtWarningMsg)

using namespace UpdateInfo;

class tst_UpdateInfo : public QObject
{
    Q_OBJECT

private slots:
    void updates_data();
    void updates();

    void available();
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

void tst_UpdateInfo::available()
{
    const QString input = R"raw(<?xml version="1.0"?>
<availablepackages>
    <package name="qt.tools.qtcreator_gui" displayname="Qt Creator 16.0.0" version="16.0.0-0-202503111337" installedVersion="16.0.0-0-202503111337"/>
    <package name="qt.tools.qtcreatordev" displayname="Plugin Development" version="16.0.0-0-202503111337"/>
    <package name="qt.tools.qtdesignstudio" displayname="Qt Design Studio 4.7.1" version="4.7.1-0-202503281750" installedVersion="4.7.0-0-202502170946"/>
    <package name="qt.tools.qtcreatordbg" displayname="Debug Symbols" version="16.0.0-0-202503111337"/>
</availablepackages>
)raw";
    const QList<Package> xpackages(
        {{"qt.tools.qtcreator_gui",
          "Qt Creator 16.0.0",
          QVersionNumber::fromString("16.0.0-0-202503111337"),
          QVersionNumber::fromString("16.0.0-0-202503111337")},
         {"qt.tools.qtcreatordev",
          "Plugin Development",
          QVersionNumber::fromString("16.0.0-0-202503111337"),
          {}},
         {"qt.tools.qtdesignstudio",
          "Qt Design Studio 4.7.1",
          QVersionNumber::fromString("4.7.1-0-202503281750"),
          QVersionNumber::fromString("4.7.0-0-202502170946")},
         {"qt.tools.qtcreatordbg",
          "Debug Symbols",
          QVersionNumber::fromString("16.0.0-0-202503111337"),
          {}}});
    const QList<Package> packages = availablePackages(input);
    QCOMPARE(packages, xpackages);
}

QTEST_GUILESS_MAIN(tst_UpdateInfo)

#include "tst_updateinfo.moc"
