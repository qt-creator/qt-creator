// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <languageserverprotocol/basemessage.h>
#include <languageserverprotocol/jsonobject.h>
#include <languageserverprotocol/jsonrpcmessages.h>

#include <utils/hostosinfo.h>

#include <QTextCodec>
#include <QtTest>

using namespace LanguageServerProtocol;

Q_DECLARE_METATYPE(QTextCodec *)
Q_DECLARE_METATYPE(BaseMessage)
Q_DECLARE_METATYPE(DocumentUri)
Q_DECLARE_METATYPE(Range)

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

    void documentUri_data();
    void documentUri();

    void range_data();
    void range();

private:
    QByteArray defaultMimeType;
    QTextCodec *defaultCodec = nullptr;
};

void tst_LanguageServerProtocol::initTestCase()
{
    defaultMimeType = JsonRpcMessage::jsonRpcMimeType();
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
            << false // errorMessage
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
            << false // errorMessage
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
            << false // errorMessage
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
        qWarning() << qPrintable(parseError);
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

    QCOMPARE(message.header() + message.content, data);
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

    BaseMessage baseMessage(JsonRpcMessage::jsonRpcMimeType(), content, content.length(), codec);
    JsonRpcMessage jsonRpcMessage(baseMessage);

    if (!error && !jsonRpcMessage.parseError().isEmpty())
        QFAIL(jsonRpcMessage.parseError().toLocal8Bit().data());
    QCOMPARE(jsonRpcMessage.toJsonObject(), expected);
    QCOMPARE(!jsonRpcMessage.parseError().isEmpty(), error);
}

void tst_LanguageServerProtocol::jsonMessageToBaseMessage_data()
{
    QTest::addColumn<JsonRpcMessage>("jsonMessage");
    QTest::addColumn<BaseMessage>("baseMessage");

    QTest::newRow("empty object") << JsonRpcMessage(QJsonObject())
                                  << BaseMessage(JsonRpcMessage::jsonRpcMimeType(),
                                                 "{}");

    QTest::newRow("key value pair") << JsonRpcMessage(QJsonObject{{"key", "value"}})
                                    << BaseMessage(JsonRpcMessage::jsonRpcMimeType(),
                                                   R"({"key":"value"})");
}

void tst_LanguageServerProtocol::jsonMessageToBaseMessage()
{
    QFETCH(JsonRpcMessage, jsonMessage);
    QFETCH(BaseMessage, baseMessage);

    QCOMPARE(jsonMessage.toBaseMessage(), baseMessage);
}

void tst_LanguageServerProtocol::documentUri_data()
{
    QTest::addColumn<DocumentUri>("uri");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<Utils::FilePath>("fileName");
    QTest::addColumn<QString>("string");

    // '/' (fs root) is part of the file path
    const QString filePrefix = Utils::HostOsInfo::isWindowsHost() ? QString("file:///")
                                                                  : QString("file://");

    QTest::newRow("empty uri")
            << DocumentUri()
            << false
            << Utils::FilePath()
            << QString();


    QTest::newRow("home dir")
            << DocumentUri::fromFilePath(Utils::FilePath::fromString(QDir::homePath()), [](auto in){ return in;})
            << true
            << Utils::FilePath::fromUserInput(QDir::homePath())
            << QString(filePrefix + QDir::homePath());

    const QString argv0 = QFileInfo(qApp->arguments().first()).absoluteFilePath();
    const auto argv0FileName = Utils::FilePath::fromUserInput(argv0);
    QTest::newRow("argv0 file name")
            << DocumentUri::fromFilePath(argv0FileName, [](auto in){ return in;})
            << true
            << argv0FileName
            << QString(filePrefix + QDir::fromNativeSeparators(argv0));

    QTest::newRow("http")
            << DocumentUri::fromProtocol("https://www.qt.io/")
            << true
            << Utils::FilePath()
            << "https://www.qt.io/";

    // depending on the OS the resulting path is different (made suitable for the file system)
    const QString winUserPercent("file:///C%3A/Users/");
    const QString winUser = Utils::HostOsInfo::isWindowsHost() ? QString("C:\\Users\\")
                                                               : QString("/C:/Users/");
    QTest::newRow("percent encoding")
            << DocumentUri::fromProtocol(winUserPercent)
            << true
            << Utils::FilePath::fromUserInput(winUser)
            << QString(filePrefix + QDir::fromNativeSeparators(winUser));
}

void tst_LanguageServerProtocol::documentUri()
{
    QFETCH(DocumentUri, uri);
    QFETCH(bool, isValid);
    QFETCH(Utils::FilePath, fileName);
    QFETCH(QString, string);

    QCOMPARE(uri.isValid(), isValid);
    QCOMPARE(uri.toFilePath([](auto in){ return in;}), fileName);
    QCOMPARE(uri.toString(), string);
}

void tst_LanguageServerProtocol::range_data()
{
    QTest::addColumn<Range>("r1");
    QTest::addColumn<Range>("r2");
    QTest::addColumn<bool>("overlaps");
    QTest::addColumn<bool>("r1Containsr2");
    QTest::addColumn<bool>("r2Containsr1");

    auto pos = [](int pos) { return Position(0, pos); };

    QTest::newRow("both ranges empty")
        << Range(pos(0), pos(0)) << Range(pos(0), pos(0)) << true << true << true;
    QTest::newRow("equal ranges")
        << Range(pos(0), pos(1)) << Range(pos(0), pos(1)) << true << true << true;
    QTest::newRow("r1 before r2")
        << Range(pos(0), pos(1)) << Range(pos(2), pos(3)) << false << false << false;
    QTest::newRow("r1 adjacent before r2")
        << Range(pos(0), pos(1)) << Range(pos(1), pos(2)) << false << false << false;
    QTest::newRow("r1 starts before r2 overlapping")
        << Range(pos(0), pos(2)) << Range(pos(1), pos(3)) << true << false << false;
    QTest::newRow("empty r1 on r2 start")
        << Range(pos(0), pos(0)) << Range(pos(0), pos(1)) << true << false << true;
    QTest::newRow("r1 inside r2 equal start")
        << Range(pos(0), pos(1)) << Range(pos(0), pos(2)) << true << false << true;
    QTest::newRow("r1 inside r2 equal start")
        << Range(pos(0), pos(1)) << Range(pos(0), pos(2)) << true << false << true;
    QTest::newRow("empty r1 inside r2")
        << Range(pos(1), pos(1)) << Range(pos(0), pos(2)) << true << false << true;
    QTest::newRow("r1 completely inside r2")
        << Range(pos(1), pos(2)) << Range(pos(0), pos(3)) << true << false << true;
    QTest::newRow("r1 inside r2 equal end")
        << Range(pos(1), pos(2)) << Range(pos(0), pos(2)) << true << false << true;
    QTest::newRow("r1 ends after r2 overlapping")
        << Range(pos(1), pos(3)) << Range(pos(0), pos(2)) << true << false << false;
    QTest::newRow("empty r1 on r2 end")
        << Range(pos(1), pos(1)) << Range(pos(0), pos(1)) << true << false << true;
    QTest::newRow("r1 adjacent after r2")
        << Range(pos(1), pos(2)) << Range(pos(0), pos(1)) << false << false << false;
    QTest::newRow("r1 behind r2")
        << Range(pos(2), pos(3)) << Range(pos(0), pos(1)) << false << false << false;
}

void tst_LanguageServerProtocol::range()
{
    QFETCH(Range, r1);
    QFETCH(Range, r2);
    QFETCH(bool, overlaps);
    QFETCH(bool, r1Containsr2);
    QFETCH(bool, r2Containsr1);

    QVERIFY(r1.overlaps(r2) == r2.overlaps(r1));
    QCOMPARE(r1.overlaps(r2), overlaps);
    QCOMPARE(r1.contains(r2), r1Containsr2);
    QCOMPARE(r2.contains(r1), r2Containsr1);
}

QTEST_GUILESS_MAIN(tst_LanguageServerProtocol)

#include "tst_languageserverprotocol.moc"
