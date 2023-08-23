// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

static const Store generateData()
{
    Store result;
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
    const Store originalData = generateData();
    QString error;
    bool success = writer.save(originalData, &error);
    QVERIFY2(success, error.toLocal8Bit());

    // verify written data
    PersistentSettingsReader reader;
    success = reader.load(filePath);
    QVERIFY(success);

    const Store restored = reader.restoreValues();
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

QTEST_GUILESS_MAIN(PersistentSettingsTest)

#include "tst_persistentsettings.moc"
