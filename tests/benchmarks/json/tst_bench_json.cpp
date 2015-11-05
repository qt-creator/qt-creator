/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

