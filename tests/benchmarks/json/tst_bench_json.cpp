/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <qjsondocument.h>
#include <qjsonobject.h>

#include <json.h>

using namespace Json;

class BenchmarkJson: public QObject
{
    Q_OBJECT

public:
    BenchmarkJson() {}

private Q_SLOTS:
    void jsonObjectInsertQt();
    void jsonObjectInsertStd();

    void createBinaryMessageQt();
    void createBinaryMessageStd();

    void readBinaryMessageQt();
    void readBinaryMessageStd();

    void createTextMessageQt();
    void createTextMessageStd();

    void readTextMessageQt();
    void readTextMessageStd();

    void parseJsonQt();
    void parseJsonStd();

    void parseNumbersQt();
    void parseNumbersStd();
};

void BenchmarkJson::parseNumbersQt()
{
    QString testFile = QFINDTESTDATA("numbers.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file numbers.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QBENCHMARK {
        QJsonDocument doc = QJsonDocument::fromJson(testJson);
        doc.object();
    }
}

void BenchmarkJson::parseNumbersStd()
{
    QString testFile = QFINDTESTDATA("numbers.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file numbers.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    std::string testJson = file.readAll().toStdString();

    QBENCHMARK {
        JsonDocument doc = JsonDocument::fromJson(testJson);
        doc.object();
    }
}

void BenchmarkJson::parseJsonQt()
{
    QString testFile = QFINDTESTDATA("test.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file test.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QBENCHMARK {
        QJsonDocument doc = QJsonDocument::fromJson(testJson);
        doc.object();
    }
}

void BenchmarkJson::parseJsonStd()
{
    QString testFile = QFINDTESTDATA("test.json");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file test.json!");
    QFile file(testFile);
    file.open(QFile::ReadOnly);
    std::string testJson = file.readAll().toStdString();

    QBENCHMARK {
        JsonDocument doc = JsonDocument::fromJson(testJson);
        doc.object();
    }
}

void BenchmarkJson::createBinaryMessageQt()
{
    // Example: send information over a datastream to another process
    // Measure performance of creating and processing data into bytearray
    QBENCHMARK {
        QJsonObject ob;
        ob.insert(QStringLiteral("command"), 1);
        ob.insert(QStringLiteral("key"), "some information");
        ob.insert(QStringLiteral("env"), "some environment variables");
        QJsonDocument(ob).toBinaryData();
    }
}

void BenchmarkJson::createBinaryMessageStd()
{
    // Example: send information over a datastream to another process
    // Measure performance of creating and processing data into bytearray
    QBENCHMARK {
        JsonObject ob;
        ob.insert("command", 1);
        ob.insert("key", "some information");
        ob.insert("env", "some environment variables");
        JsonDocument(ob).toBinaryData();
    }
}

void BenchmarkJson::readBinaryMessageQt()
{
    // Example: receive information over a datastream from another process
    // Measure performance of converting content back to QVariantMap
    // We need to recreate the bytearray but here we only want to measure the latter
    QJsonObject ob;
    ob.insert(QStringLiteral("command"), 1);
    ob.insert(QStringLiteral("key"), "some information");
    ob.insert(QStringLiteral("env"), "some environment variables");
    QByteArray msg = QJsonDocument(ob).toBinaryData();

    QBENCHMARK {
        QJsonDocument::fromBinaryData(msg, QJsonDocument::Validate).object();
    }
}

void BenchmarkJson::readBinaryMessageStd()
{
    // Example: receive information over a datastream from another process
    // Measure performance of converting content back to QVariantMap
    // We need to recreate the bytearray but here we only want to measure the latter
    JsonObject ob;
    ob.insert("command", 1);
    ob.insert("key", "some information");
    ob.insert("env", "some environment variables");
    std::string msg = JsonDocument(ob).toBinaryData();

    QBENCHMARK {
        JsonDocument::fromBinaryData(msg, JsonDocument::Validate).object();
    }
}

void BenchmarkJson::createTextMessageQt()
{
    // Example: send information over a datastream to another process
    // Measure performance of creating and processing data into bytearray
    QBENCHMARK {
        QJsonObject ob;
        ob.insert(QStringLiteral("command"), 1);
        ob.insert(QStringLiteral("key"), "some information");
        ob.insert(QStringLiteral("env"), "some environment variables");
        QByteArray msg = QJsonDocument(ob).toJson();
    }
}

void BenchmarkJson::createTextMessageStd()
{
    // Example: send information over a datastream to another process
    // Measure performance of creating and processing data into bytearray
    QBENCHMARK {
        JsonObject ob;
        ob.insert("command", 1);
        ob.insert("key", "some information");
        ob.insert("env", "some environment variables");
        std::string msg = JsonDocument(ob).toJson();
    }
}

void BenchmarkJson::readTextMessageQt()
{
    // Example: receive information over a datastream from another process
    // Measure performance of converting content back to QVariantMap
    // We need to recreate the bytearray but here we only want to measure the latter
    QJsonObject ob;
    ob.insert(QStringLiteral("command"), 1);
    ob.insert(QStringLiteral("key"), "some information");
    ob.insert(QStringLiteral("env"), "some environment variables");
    QByteArray msg = QJsonDocument(ob).toJson();

    QBENCHMARK {
        QJsonDocument::fromJson(msg).object();
    }
}

void BenchmarkJson::readTextMessageStd()
{
    // Example: receive information over a datastream from another process
    // Measure performance of converting content back to QVariantMap
    // We need to recreate the bytearray but here we only want to measure the latter
    JsonObject ob;
    ob.insert("command", 1);
    ob.insert("key", "some information");
    ob.insert("env", "some environment variables");
    std::string msg = JsonDocument(ob).toJson();

    QBENCHMARK {
        JsonDocument::fromJson(msg).object();
    }
}

void BenchmarkJson::jsonObjectInsertQt()
{
    QJsonObject object;
    QJsonValue value(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + QString::number(i), value);
    }
}

void BenchmarkJson::jsonObjectInsertStd()
{
    JsonObject object;
    JsonValue value(1.5);

    QBENCHMARK {
        for (int i = 0; i < 1000; i++)
            object.insert("testkey_" + std::to_string(i), value);
    }
}

QTEST_MAIN(BenchmarkJson)

#include "tst_bench_json.moc"

