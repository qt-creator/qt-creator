/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtTest>

#include <updateinfo/updateinfotools.h>

Q_LOGGING_CATEGORY(log, "qtc.updateinfo", QtWarningMsg)

class tst_UpdateInfo : public QObject
{
    Q_OBJECT

private slots:
    void updates_data();
    void updates();
};

void tst_UpdateInfo::updates_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QList<Update>>("xupdates");
    QTest::addColumn<QList<QtPackage>>("xpackages");

    QTest::newRow("updates and packages")
        << R"raw(<updates>
        <update name="Qt Design Studio 3.2.0" version="3.2.0-0-202203291247" size="3113234690" id="qt.tools.qtdesignstudio"/>
        </updates>
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
    QFETCH(QString, xml);
    QFETCH(QList<Update>, xupdates);
    QFETCH(QList<QtPackage>, xpackages);

    std::unique_ptr<QDomDocument> doc = documentForResponse(xml);
    const QList<Update> updates = availableUpdates(*doc);
    QCOMPARE(updates, xupdates);
    const QList<QtPackage> packages = availableQtPackages(*doc);
    QCOMPARE(packages, xpackages);
}

QTEST_GUILESS_MAIN(tst_UpdateInfo)

#include "tst_updateinfo.moc"
