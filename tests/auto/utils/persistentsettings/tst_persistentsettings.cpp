/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <utils/persistentsettings.h>

#include <QTemporaryDir>

#include <QtTest>

using namespace Utils;

class PersistentSettingsTest : public QObject
{
    Q_OBJECT
public:
    PersistentSettingsTest() = default;

private slots:
    void tst_readwrite();
};

static const QVariantMap generateData()
{
    QVariantMap result;
    QByteArray barr("I am a byte array.");
    QString str("I am a string.");
    QColor color("#8b00d1");
    QRect rect(0, 0, 400, 600);
    QRect rect2(10, 10, 40, 40);
    QVariantList varList{barr, color, rect2};
    result.insert("barr", barr);
    result.insert("str", str);
    result.insert("color", color);
    result.insert("rect", rect);
    result.insert("varList", varList);
    return result;
}

void PersistentSettingsTest::tst_readwrite()
{
    QTemporaryDir tmpDir("qtc_test_persistentXXXXXX");
    tmpDir.setAutoRemove(false);
    const QFileInfo fi(QDir(tmpDir.path()), "settings.xml");
    qDebug() << "using" << fi.absoluteFilePath();
    const FilePath filePath = FilePath::fromFileInfo(fi);
    PersistentSettingsWriter writer(filePath, "Narf");
    const QVariantMap originalData = generateData();
    QString error;
    bool success = writer.save(originalData, &error);
    QVERIFY2(success, error.toLocal8Bit());

    // verify written data
    PersistentSettingsReader reader;
    success = reader.load(filePath);
    QVERIFY(success);

    const QVariantMap restored = reader.restoreValues();
    QCOMPARE(restored.size(), originalData.size());
    auto restoredEnd = restored.end();
    for (auto it = originalData.cbegin(), end = originalData.cend(); it != end; ++it) {
        auto found = restored.find(it.key());
        QVERIFY(found != restoredEnd);
        QVERIFY(found.value().isValid());
        if (it.value().type() == QVariant::List) {
            const QVariantList origList = it.value().toList();
            const QVariantList foundList = found.value().toList();

            QCOMPARE(foundList.size(), origList.size());
            for (int i = 0, vEnd = foundList.size(); i < vEnd; ++i) {
                if (foundList.at(i).type() == QVariant::Rect)
                    qDebug() << foundList.at(i).toRect() << origList.at(i).toRect();
                QCOMPARE(foundList.at(i), origList.at(i));
            }
        }
        if (it.value().type() == QVariant::Rect)
            qDebug() << found.value().toRect() << "vs" << it.value().toRect();
        QCOMPARE(found.value(), it.value());
    }
    tmpDir.setAutoRemove(!QTest::currentTestFailed());
}

QTEST_MAIN(PersistentSettingsTest)

#include "tst_persistentsettings.moc"
