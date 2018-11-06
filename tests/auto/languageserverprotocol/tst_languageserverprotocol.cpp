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

#include <languageserverprotocol/basemessage.h>
#include <languageserverprotocol/jsonobject.h>
#include <languageserverprotocol/jsonrpcmessages.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QtTest>

using namespace LanguageServerProtocol;

Q_DECLARE_METATYPE(QTextCodec *)
Q_DECLARE_METATYPE(BaseMessage)
Q_DECLARE_METATYPE(JsonRpcMessage)
Q_DECLARE_METATYPE(DocumentUri)

class tst_LanguageServerProtocol : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void baseMessageParse_data();
    void baseMessageParse();

    void baseMessageToData_data();
    void baseMessageToData();

    void fromJsonValue();

    void toJsonObject_data();
    void toJsonObject();

    void jsonMessageToBaseMessage_data();
    void jsonMessageToBaseMessage();

    void jsonObject();

    void documentUri_data();
    void documentUri();

private:
    QByteArray defaultMimeType;
    QTextCodec *defaultCodec = nullptr;
};

void tst_LanguageServerProtocol::initTestCase()
{
    defaultMimeType = JsonRpcMessageHandler::jsonRpcMimeType();
    defaultCodec = QTextCodec::codecForName("utf-8");
}

void tst_LanguageServerProtocol::baseMessageParse_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("mimeType");
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<bool>("complete");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("error");
    QTest::addColumn<QTextCodec*>("codec");
    QTest::addColumn<BaseMessage>("partial");

    QTest::newRow("empty content")
            << QByteArray("")
            << defaultMimeType
            << QByteArray()
            << false // complete
            << false // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("garbage")
            << QByteArray("garbage\r\n")
            << defaultMimeType
            << QByteArray()
            << false // complete
            << false // valid
            << true  // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("minimum message")
            << QByteArray("Content-Length: 0\r\n"
                          "\r\n")
            << defaultMimeType
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("minimum message with content")
            << QByteArray("Content-Length: 3\r\n"
                          "\r\n"
                          "foo")
            << defaultMimeType
            << QByteArray("foo")
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("minimum message with incomplete content")
            << QByteArray("Content-Length: 6\r\n"
                          "\r\n"
                          "foo")
            << defaultMimeType
            << QByteArray("foo")
            << false // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("default mime type")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: application/vscode-jsonrpc\r\n"
                          "\r\n")
            << defaultMimeType
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("default mime type and charset")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: application/vscode-jsonrpc; charset=utf-8\r\n"
                          "\r\n")
            << defaultMimeType
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    // For backwards compatibility it is highly recommended that a client and a server
    // treats the string utf8 as utf-8. (lsp documentation)
    QTest::newRow("default mime type and old charset")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: application/vscode-jsonrpc; charset=utf8\r\n"
                          "\r\n")
            << defaultMimeType
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("non default mime type with default charset")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: text/x-python\r\n"
                          "\r\n")
            << QByteArray("text/x-python")
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("non default mime type and charset")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: text/x-python; charset=iso-8859-1\r\n"
                          "\r\n")
            << QByteArray("text/x-python")
            << QByteArray()
            << true  // complete
            << true  // valid
            << false // errorMessage
            << QTextCodec::codecForName("iso-8859-1")
            << BaseMessage();

    QTest::newRow("data after message")
            << QByteArray("Content-Length: 3\r\n"
                          "\r\n"
                          "foobar")
            << defaultMimeType
            << QByteArray("foo")
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("Unexpected header field")
            << QByteArray("Content-Length: 6\r\n"
                          "Foo: bar\r\n"
                          "\r\n"
                          "foobar")
            << defaultMimeType
            << QByteArray("foobar")
            << true // complete
            << true // valid
            << true // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("Unexpected header line")
            << QByteArray("Content-Length: 6\r\n"
                          "Foobar\r\n"
                          "\r\n"
                          "foobar")
            << defaultMimeType
            << QByteArray("foobar")
            << true // complete
            << true // valid
            << true // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("Unknown mimeType")
            << QByteArray("Content-Length: 6\r\n"
                          "Content-Type: foobar\r\n"
                          "\r\n"
                          "foobar")
            << QByteArray("foobar")
            << QByteArray("foobar")
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("Unknown charset")
            << QByteArray("Content-Length: 6\r\n"
                          "Content-Type: application/vscode-jsonrpc; charset=foobar\r\n"
                          "\r\n"
                          "foobar")
            << defaultMimeType
            << QByteArray("foobar")
            << true // complete
            << true // valid
            << true // errorMessage
            << defaultCodec
            << BaseMessage();

    QTest::newRow("completing content")
            << QByteArray("bar")
            << defaultMimeType
            << QByteArray("foobar")
            << true  // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage(defaultMimeType, "foo", 6, defaultCodec);

    QTest::newRow("still incomplet content")
            << QByteArray("bar")
            << defaultMimeType
            << QByteArray("foobar")
            << false // complete
            << true  // valid
            << false // errorMessage
            << defaultCodec
            << BaseMessage(defaultMimeType, "foo", 7, defaultCodec);
}

void tst_LanguageServerProtocol::baseMessageParse()
{
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, mimeType);
    QFETCH(QByteArray, content);
    QFETCH(bool, complete);
    QFETCH(bool, valid);
    QFETCH(QTextCodec *, codec);
    QFETCH(bool, error);
    QFETCH(BaseMessage, partial);

    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadWrite);
    QString parseError;
    BaseMessage::parse(&buffer, parseError, partial);

    if (!parseError.isEmpty() && !error) // show message if there is an error message we do not expect
        QWARN(parseError.toLatin1());
    QCOMPARE(!parseError.isEmpty(), error);
    QCOMPARE(partial.content, content);
    QCOMPARE(partial.isValid(), valid);
    QCOMPARE(partial.isComplete(), complete);
    QCOMPARE(partial.mimeType, mimeType);
    QVERIFY(partial.codec != nullptr);
    QVERIFY(codec != nullptr);
    QCOMPARE(partial.codec->mibEnum(), codec->mibEnum());
}

void tst_LanguageServerProtocol::baseMessageToData_data()
{
    QTest::addColumn<BaseMessage>("message");
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("empty")
            << BaseMessage(defaultMimeType, "")
            << QByteArray("Content-Length: 0\r\n"
                          "\r\n");

    QTest::newRow("content")
            << BaseMessage(defaultMimeType, "foo")
            << QByteArray("Content-Length: 3\r\n"
                          "\r\n"
                          "foo");

    QTest::newRow("custom mime type")
            << BaseMessage("text/x-python", "")
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: text/x-python; charset=UTF-8\r\n"
                          "\r\n");

    QTextCodec *codec = QTextCodec::codecForName("iso-8859-1");
    QTest::newRow("custom mime type and codec")
            << BaseMessage("text/x-python", "", 0, codec)
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: text/x-python; charset=ISO-8859-1\r\n"
                          "\r\n");

    QTest::newRow("custom codec")
            << BaseMessage(defaultMimeType, "", 0, codec)
            << QByteArray("Content-Length: 0\r\n"
                          "Content-Type: application/vscode-jsonrpc; charset=ISO-8859-1\r\n"
                          "\r\n");
}

void tst_LanguageServerProtocol::baseMessageToData()
{
    QFETCH(BaseMessage, message);
    QFETCH(QByteArray, data);

    QCOMPARE(message.toData(), data);
}

void tst_LanguageServerProtocol::fromJsonValue()
{
    const QString strVal("foobar");
    QCOMPARE(LanguageServerProtocol::fromJsonValue<QString>(QJsonValue(strVal)), strVal);
    const int intVal = 42;
    QCOMPARE(LanguageServerProtocol::fromJsonValue<int>(QJsonValue(intVal)), intVal);
    const double doubleVal = 4.2;
    QCOMPARE(LanguageServerProtocol::fromJsonValue<double>(QJsonValue(doubleVal)), doubleVal);
    const bool boolVal = false;
    QCOMPARE(LanguageServerProtocol::fromJsonValue<bool>(QJsonValue(boolVal)), boolVal);
    const QJsonArray array = QJsonArray::fromStringList({"foo", "bar"});
    QCOMPARE(LanguageServerProtocol::fromJsonValue<QJsonArray>(array), array);
    QJsonObject object;
    object.insert("asd", "foo");
    QCOMPARE(LanguageServerProtocol::fromJsonValue<QJsonObject>(object), object);
}

void tst_LanguageServerProtocol::toJsonObject_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QTextCodec *>("codec");
    QTest::addColumn<bool>("error");
    QTest::addColumn<QJsonObject>("expected");

    QJsonObject tstObject;
    tstObject.insert("jsonrpc", "2.0");

    QTest::newRow("empty")
            << QByteArray("")
            << defaultCodec
            << false
            << QJsonObject();

    QTest::newRow("garbage")
            << QByteArray("foobar")
            << defaultCodec
            << true
            << QJsonObject();

    QTest::newRow("empty object")
            << QByteArray("{}")
            << defaultCodec
            << false
            << QJsonObject();

    QTest::newRow("object")
            << QByteArray(R"({"jsonrpc": "2.0"})")
            << defaultCodec
            << false
            << tstObject;

    QTextCodec *codec = QTextCodec::codecForName("iso-8859-1");
    QJsonObject tstCodecObject;
    tstCodecObject.insert("foo", QString::fromLatin1("b\xe4r"));
    QTest::newRow("object88591")
            << QByteArray("{\"foo\": \"b\xe4r\"}")
            << codec
            << false
            << tstCodecObject;

    QTest::newRow("object and garbage")
            << QByteArray(R"({"jsonrpc": "2.0"}  foobar)")
            << defaultCodec
            << true
            << QJsonObject(); // TODO can be improved

    QTest::newRow("empty array")
            << QByteArray("[]")
            << defaultCodec
            << true
            << QJsonObject();

    QTest::newRow("null")
            << QByteArray("null")
            << defaultCodec
            << true
            << QJsonObject();
}

void tst_LanguageServerProtocol::toJsonObject()
{
    QFETCH(QByteArray, content);
    QFETCH(QTextCodec *, codec);
    QFETCH(bool, error);
    QFETCH(QJsonObject, expected);

    QString parseError;
    const QJsonObject object = JsonRpcMessageHandler::toJsonObject(content, codec, parseError);

    if (!error && !parseError.isEmpty())
        QFAIL(parseError.toLocal8Bit().data());
    QCOMPARE(object, expected);
    QCOMPARE(!parseError.isEmpty(), error);
}

void tst_LanguageServerProtocol::jsonMessageToBaseMessage_data()
{
    QTest::addColumn<JsonRpcMessage>("jsonMessage");
    QTest::addColumn<BaseMessage>("baseMessage");

    QTest::newRow("empty object") << JsonRpcMessage(QJsonObject())
                                  << BaseMessage(JsonRpcMessageHandler::jsonRpcMimeType(),
                                                 "{}");

    QTest::newRow("key value pair") << JsonRpcMessage({{"key", "value"}})
                                    << BaseMessage(JsonRpcMessageHandler::jsonRpcMimeType(),
                                                   R"({"key":"value"})");
}

void tst_LanguageServerProtocol::jsonMessageToBaseMessage()
{
    QFETCH(JsonRpcMessage, jsonMessage);
    QFETCH(BaseMessage, baseMessage);

    QCOMPARE(jsonMessage.toBaseMessage(), baseMessage);
}

class JsonTestObject : public JsonObject
{
public:
    using JsonObject::JsonObject;
    using JsonObject::insert;
    using JsonObject::value;
    using JsonObject::contains;
    using JsonObject::find;
    using JsonObject::end;
    using JsonObject::remove;
    using JsonObject::keys;
    using JsonObject::typedValue;
    using JsonObject::optionalValue;
    using JsonObject::clientValue;
    using JsonObject::optionalClientValue;
    using JsonObject::array;
    using JsonObject::optionalArray;
    using JsonObject::clientArray;
    using JsonObject::optionalClientArray;
    using JsonObject::insertArray;
    using JsonObject::checkKey;
    using JsonObject::valueTypeString;
    using JsonObject::check;
    using JsonObject::checkType;
    using JsonObject::checkVal;
    using JsonObject::checkArray;
    using JsonObject::checkOptional;
    using JsonObject::checkOptionalArray;
    using JsonObject::errorString;
    using JsonObject::operator==;
};

void tst_LanguageServerProtocol::jsonObject()
{
    JsonTestObject obj;

    obj.insert("integer", 42);
    obj.insert("double", 42.42);
    obj.insert("bool", false);
    obj.insert("null", QJsonValue::Null);
    obj.insert("string", "foobar");
    obj.insertArray("strings", QStringList{"foo", "bar"});
    const JsonTestObject innerObj(obj);
    obj.insert("object", innerObj);

    QCOMPARE(obj.value("integer"), QJsonValue(42));
    QCOMPARE(obj.value("double"), QJsonValue(42.42));
    QCOMPARE(obj.value("bool"), QJsonValue(false));
    QCOMPARE(obj.value("null"), QJsonValue(QJsonValue::Null));
    QCOMPARE(obj.value("string"), QJsonValue("foobar"));
    QCOMPARE(obj.value("strings"), QJsonValue(QJsonArray({"foo", "bar"})));
    QCOMPARE(obj.value("object"), QJsonValue(QJsonObject(innerObj)));

    QCOMPARE(obj.typedValue<int>("integer"), 42);
    QCOMPARE(obj.typedValue<double>("double"), 42.42);
    QCOMPARE(obj.typedValue<bool>("bool"), false);
    QCOMPARE(obj.typedValue<QString>("string"), QString("foobar"));
    QCOMPARE(obj.typedValue<JsonTestObject>("object"), innerObj);

    QVERIFY(!obj.optionalValue<int>("doesNotExist").has_value());
    QVERIFY(obj.optionalValue<int>("integer").has_value());
    QCOMPARE(obj.optionalValue<int>("integer").value_or(0), 42);

    QVERIFY(obj.clientValue<int>("null").isNull());
    QVERIFY(!obj.clientValue<int>("integer").isNull());
    QCOMPARE(obj.clientValue<int>("integer").value(), 42);

    QVERIFY(!obj.optionalClientValue<int>("doesNotExist").has_value());
    QVERIFY(obj.optionalClientValue<int>("null").has_value());
    QVERIFY(obj.optionalClientValue<int>("null").value().isNull());
    QVERIFY(obj.optionalClientValue<int>("integer").has_value());
    QVERIFY(!obj.optionalClientValue<int>("integer").value().isNull());
    QCOMPARE(obj.optionalClientValue<int>("integer").value().value(0), 42);

    QCOMPARE(obj.array<QString>("strings"), QList<QString>({"foo", "bar"}));

    QVERIFY(!obj.optionalArray<QString>("doesNotExist").has_value());
    QVERIFY(obj.optionalArray<QString>("strings").has_value());
    QCOMPARE(obj.optionalArray<QString>("strings").value_or(QList<QString>()),
             QList<QString>({"foo", "bar"}));

    QVERIFY(obj.clientArray<QString>("null").isNull());
    QVERIFY(!obj.clientArray<QString>("strings").isNull());
    QCOMPARE(obj.clientArray<QString>("strings").toList(), QList<QString>({"foo", "bar"}));

    QVERIFY(!obj.optionalClientArray<QString>("doesNotExist").has_value());
    QVERIFY(obj.optionalClientArray<QString>("null").has_value());
    QVERIFY(obj.optionalClientArray<QString>("null").value().isNull());
    QVERIFY(obj.optionalClientArray<QString>("strings").has_value());
    QVERIFY(!obj.optionalClientArray<QString>("strings").value().isNull());
    QCOMPARE(obj.optionalClientArray<QString>("strings").value().toList(),
             QList<QString>({"foo", "bar"}));

    QStringList errorHierarchy;
    QVERIFY(!obj.check<int>(&errorHierarchy, "doesNotExist"));
    QCOMPARE(errorHierarchy, QStringList({obj.errorString(QJsonValue::Double, QJsonValue::Undefined), "doesNotExist"}));
    errorHierarchy.clear();

    QVERIFY(!obj.check<int>(&errorHierarchy, "bool"));
    QCOMPARE(errorHierarchy, QStringList({obj.errorString(QJsonValue::Double, QJsonValue::Bool), "bool"}));
    errorHierarchy.clear();

    QVERIFY(obj.check<int>(&errorHierarchy, "integer"));
    QVERIFY(errorHierarchy.isEmpty());
    QVERIFY(obj.check<double>(&errorHierarchy, "double"));
    QVERIFY(errorHierarchy.isEmpty());
    QVERIFY(obj.check<bool>(&errorHierarchy, "bool"));
    QVERIFY(errorHierarchy.isEmpty());
    QVERIFY(obj.check<std::nullptr_t>(&errorHierarchy, "null"));
    QVERIFY(errorHierarchy.isEmpty());
    QVERIFY(obj.check<QString>(&errorHierarchy, "string"));
    QVERIFY(errorHierarchy.isEmpty());
}

void tst_LanguageServerProtocol::documentUri_data()
{
    QTest::addColumn<DocumentUri>("uri");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<Utils::FileName>("fileName");
    QTest::addColumn<QString>("string");

    // '/' (fs root) is part of the file path
    const QString filePrefix = Utils::HostOsInfo::isWindowsHost() ? QString("file:///")
                                                                  : QString("file://");

    QTest::newRow("empty uri")
            << DocumentUri()
            << false
            << Utils::FileName()
            << QString();


    QTest::newRow("home dir")
            << DocumentUri::fromFileName(Utils::FileName::fromString(QDir::homePath()))
            << true
            << Utils::FileName::fromUserInput(QDir::homePath())
            << QString(filePrefix + QDir::homePath());

    const QString argv0 = QFileInfo(qApp->arguments().first()).absoluteFilePath();
    const auto argv0FileName = Utils::FileName::fromUserInput(argv0);
    QTest::newRow("argv0 file name")
            << DocumentUri::fromFileName(argv0FileName)
            << true
            << argv0FileName
            << QString(filePrefix + QDir::fromNativeSeparators(argv0));

    QTest::newRow("http")
            << DocumentUri::fromProtocol("https://www.qt.io/")
            << true
            << Utils::FileName()
            << "https://www.qt.io/";

    // depending on the OS the resulting path is different (made suitable for the file system)
    const QString winUserPercent("file:///C%3A/Users/");
    const QString winUser = Utils::HostOsInfo::isWindowsHost() ? QString("C:\\Users\\")
                                                               : QString("/C:/Users/");
    QTest::newRow("percent encoding")
            << DocumentUri::fromProtocol(winUserPercent)
            << true
            << Utils::FileName::fromUserInput(winUser)
            << QString(filePrefix + QDir::fromNativeSeparators(winUser));
}

void tst_LanguageServerProtocol::documentUri()
{
    QFETCH(DocumentUri, uri);
    QFETCH(bool, isValid);
    QFETCH(Utils::FileName, fileName);
    QFETCH(QString, string);

    QCOMPARE(uri.isValid(), isValid);
    QCOMPARE(uri.toFileName(), fileName);
    QCOMPARE(uri.toString(), string);
}

QTEST_MAIN(tst_LanguageServerProtocol)

#include "tst_languageserverprotocol.moc"
