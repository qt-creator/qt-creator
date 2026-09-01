// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTemporaryDir>
#include <QTest>

#include <utils/filepath.h>
#include <utils/textcodec.h>
#include <utils/textfileformat.h>

using namespace Utils;

class TextFileFormatTest : public QObject
{
    Q_OBJECT

private slots:
    void testDecodingErrorPosition_data();
    void testDecodingErrorPosition();
    void testDecodingErrorOffsets();
    void testDecodingErrorOffsetsBeyondLimits();

    void fallbackDecode()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        const QByteArray data("caf\xE9" "X", 5);
        QVERIFY(path.writeFileContents(data));

        TextFileFormat format;
        const TextEncoding utf8(QStringEncoder::Encoding::Utf8);
        format.setEncoding(utf8);
        const auto result = format.readFile(path, utf8, TextEncoding("ISO-8859-1"));
        QCOMPARE(result.code, TextFileFormat::ReadSuccess);
        QVERIFY(result.usedDecodingFallback);
        QCOMPARE(format.encoding(), TextEncoding("ISO-8859-1"));
        QCOMPARE(result.content, QString::fromLatin1(data));
    }

    void noFallbackKeepsPrimaryEncoding()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        QVERIFY(path.writeFileContents(QByteArray("caf\xE9" "X", 5)));

        TextFileFormat format;
        const TextEncoding utf8(QStringEncoder::Encoding::Utf8);
        format.setEncoding(utf8);
        const auto result = format.readFile(path, utf8);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QVERIFY(!result.usedDecodingFallback);
        QVERIFY(format.encoding().isUtf8());
        QVERIFY(result.decodingErrorOffsets.isEmpty());
        QVERIFY(result.decodingErrorData.isEmpty());
        QCOMPARE(result.decodingErrorOffset, qsizetype(-1));
    }

    void sameEncodingIsNotRetried()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        QVERIFY(path.writeFileContents(QByteArray("caf\xE9" "X", 5)));

        TextFileFormat format;
        const TextEncoding utf8(QStringEncoder::Encoding::Utf8);
        format.setEncoding(utf8);
        const auto result = format.readFile(path, utf8, utf8);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QVERIFY(!result.usedDecodingFallback);
        QVERIFY(format.encoding().isUtf8());
    }

    void failedFallbackRestoresPrimaryEncoding()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        const QByteArray data = QByteArray::fromHex("ffffffff");
        QVERIFY(path.writeFileContents(data));
        const TextEncoding utf8(TextEncoding::Utf8);
        TextFileFormat primaryFormat;
        const auto primary = primaryFormat.readFile(path, utf8);
        QCOMPARE(primary.code, TextFileFormat::ReadEncodingError);

        if (!QStringDecoder("US-ASCII").isValid())
            QSKIP("US-ASCII requires a Qt build with ICU.");
        const TextEncoding fallback("US-ASCII");
        QVERIFY(fallback.isValid());
        QVERIFY(fallback != utf8);
        TextFileFormat fallbackFormat;
        QCOMPARE(fallbackFormat.readFile(path, fallback).code, TextFileFormat::ReadEncodingError);
        TextFileFormat format;
        const auto result = format.readFile(path, utf8, fallback);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QVERIFY(!result.usedDecodingFallback);
        QCOMPARE(format.encoding(), utf8);
        QCOMPARE(result.content, primary.content);
        QCOMPARE(result.decodingErrorSample, data);
    }

    void bomTakesPrecedenceOverFallback()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        QVERIFY(path.writeFileContents(QByteArray::fromHex("efbbbf") + QByteArray("\xE9" "X", 2)));

        TextFileFormat format;
        format.setEncoding(TextEncoding(QStringEncoder::Encoding::Utf8));
        const auto result = format.readFile(path, TextEncoding(), TextEncoding("ISO-8859-1"));
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QVERIFY(!result.usedDecodingFallback);
        QVERIFY(format.encoding().isUtf8());
    }

    void utf8ErrorOffsetPointsToPendingLeadByte()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        QVERIFY(path.writeFileContents(QByteArray("A\r\n\xE9" "X", 5)));

        TextFileFormat format;
        const TextEncoding utf8(QStringEncoder::Encoding::Utf8);
        format.setEncoding(utf8);
        const auto result = format.readFile(
            path, utf8, {}, TextFileFormat::DecodingErrorDetails::Collect);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QCOMPARE(result.decodingErrorOffset, qsizetype(3));
    }

    void decodingErrorOffsetsAcrossChunks_data()
    {
        QTest::addColumn<QByteArray>("encodingName");
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<QList<qsizetype>>("expectedOffsets");
        QTest::newRow("ASCII late errors")
            << QByteArray("US-ASCII")
            << QByteArray(1024 * 1024, 'a') + QByteArray::fromHex("ff58ff")
            << QList<qsizetype>{1024 * 1024, 1024 * 1024 + 2};
        QTest::newRow("Shift-JIS pending lead at boundary")
            << QByteArray("Shift_JIS")
            << QByteArray(4095, 'a') + QByteArray::fromHex("82208220")
            << QList<qsizetype>{4095, 4097};
        QTest::newRow("Shift-JIS valid boundary then error")
            << QByteArray("Shift_JIS")
            << QByteArray(4095, 'a') + QByteArray::fromHex("82a0")
                   + QByteArray(8192, 'b') + QByteArray::fromHex("8220")
            << QList<qsizetype>{12289};
        QList<qsizetype> boundedOffsets;
        for (qsizetype offset = 0; offset <= TextFileFormat::MaximumDecodingErrorOffsets; ++offset)
            boundedOffsets.append(offset);
        QTest::newRow("ASCII bounded errors")
            << QByteArray("US-ASCII") << QByteArray(10001, char(0xff)) << boundedOffsets;
    }

    void decodingErrorOffsetsAcrossChunks()
    {
        QFETCH(QByteArray, encodingName);
        QFETCH(QByteArray, data);
        QFETCH(QList<qsizetype>, expectedOffsets);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        QVERIFY(path.writeFileContents(data));
        if (!QStringDecoder(encodingName).isValid())
            QSKIP("The test encoding requires a Qt build with ICU.");
        const TextEncoding encoding(encodingName);
        QVERIFY(encoding.isValid());
        TextFileFormat format;
        format.setEncoding(encoding);
        const auto result = format.readFile(
            path, encoding, {}, TextFileFormat::DecodingErrorDetails::Collect);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QCOMPARE(result.decodingErrorOffsets, expectedOffsets);
    }

    void decodingErrorOffsets()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath path = FilePath::fromString(dir.path() + "/input.txt");
        const QByteArray data("A\r\n\x82" "B", 5);
        QVERIFY(path.writeFileContents(data));

        TextFileFormat format;
        if (!QStringDecoder("Shift_JIS").isValid())
            QSKIP("Shift-JIS requires a Qt build with ICU.");
        const TextEncoding encoding("Shift_JIS");
        format.setEncoding(encoding);
        const auto result = format.readFile(
            path, encoding, {}, TextFileFormat::DecodingErrorDetails::Collect);
        QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
        QCOMPARE(result.decodingErrorOffsets, QList<qsizetype>{3});
        QVERIFY(!result.content.contains(QLatin1Char('\r')));
        QVERIFY(result.content.contains(QLatin1Char('\n')));

        QCOMPARE(TextFileFormat::decodedPositionsForByteOffsets(
                     data, encoding, {3}, TextFileFormat::CRLFLineTerminator),
                 QList<qsizetype>{2});
        const QByteArray mixedLineEndings("A\nB\r\ncaf\xE9" "X", 10);
        const QList<qsizetype> positions = TextFileFormat::decodedPositionsForByteOffsets(
            mixedLineEndings, TextEncoding(TextEncoding::Latin1), {8},
            TextFileFormat::LFLineTerminator);
        QCOMPARE(positions, QList<qsizetype>{7});
    }

};

void TextFileFormatTest::testDecodingErrorPosition_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<qsizetype>("errorOffset");
    QTest::addColumn<qsizetype>("errorPosition");

    QTest::newRow("ASCII with LF") << (QByteArray("line one\n") + QByteArray(1, char(0xff))
                                      + QByteArray("after"))
                                   << qsizetype(9)
                                   << qsizetype(9);
    QTest::newRow("UTF-8 with LF") << QByteArray::fromHex("c3a40aff") << qsizetype(3)
                                   << qsizetype(2);
    QTest::newRow("UTF-8 supplementary character") << QByteArray::fromHex("f09f9880ff")
                                                    << qsizetype(4) << qsizetype(2);
    QTest::newRow("ASCII with CRLF") << (QByteArray("line\r\n") + QByteArray(1, char(0xff))
                                         + QByteArray("after"))
                                     << qsizetype(6)
                                     << qsizetype(5);
    QTest::newRow("lone CR before invalid byte")
        << QByteArray::fromHex("610dff620a") << qsizetype(2) << qsizetype(2);
    QTest::newRow("ASCII after a complete chunk")
        << QByteArray(4096, 'a') + QByteArray(1, char(0xff)) << qsizetype(4096)
        << qsizetype(4096);
    QTest::newRow("invalid sequence after ASCII prefix")
        << QByteArray(4095, 'a') + QByteArray::fromHex("e958") << qsizetype(4095)
        << qsizetype(4095);
    QTest::newRow("ASCII after maximum scan size")
        << QByteArray(1 << 20, 'a') + QByteArray(1, char(0xff)) << qsizetype(1 << 20)
        << qsizetype(1 << 20);
}

void TextFileFormatTest::testDecodingErrorPosition()
{
    QFETCH(QByteArray, data);
    QFETCH(qsizetype, errorOffset);
    QFETCH(qsizetype, errorPosition);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath("input");
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), qint64(data.size()));
    file.close();

    TextFileFormat format;
    const TextFileFormat::ReadResult result = format.readFile(
        FilePath::fromString(fileName), TextEncoding(TextEncoding::Utf8),
        {}, TextFileFormat::DecodingErrorDetails::Collect);

    QVERIFY2(result.code == TextFileFormat::ReadEncodingError,
             qPrintable(result.error));
    QCOMPARE(result.decodingErrorOffset, errorOffset);
    QCOMPARE(result.decodingErrorPosition, errorPosition);
}

void TextFileFormatTest::testDecodingErrorOffsets()
{
    QByteArray data("a\n");
    data += char(0xfc);
    data += char(0xdf);
    data += QByteArray(" x ");
    data += char(0xf6);
    data += QByteArray(" ");
    data += QByteArray::fromHex("c4d6dc");

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath("input");
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), qint64(data.size()));
    file.close();

    TextFileFormat format;
    const TextFileFormat::ReadResult result = format.readFile(
        FilePath::fromString(fileName), TextEncoding(TextEncoding::Utf8),
        {}, TextFileFormat::DecodingErrorDetails::Collect);

    QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
    const QList<qsizetype> expectedOffsets{2, 3, 7, 9, 10, 11};
    QCOMPARE(result.decodingErrorOffsets, expectedOffsets);
    const TextEncoding latin1(TextEncoding::Latin1);
    const QList<qsizetype> decodedPositions = TextFileFormat::decodedPositionsForByteOffsets(
        data, latin1, expectedOffsets, TextFileFormat::LFLineTerminator);
    QCOMPARE(decodedPositions, expectedOffsets);
    const QString decodedText = latin1.decode(data);
    QCOMPARE(decodedText.at(decodedPositions.at(0)), QChar(0x00fc));
    QCOMPARE(decodedText.at(decodedPositions.at(1)), QChar(0x00df));
}

void TextFileFormatTest::testDecodingErrorOffsetsBeyondLimits()
{
    constexpr qsizetype errorCount = 10001;
    QByteArray data(1 << 20, 'a');
    data += QByteArray(errorCount, char(0xff));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString fileName = directory.filePath("input");
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), qint64(data.size()));
    file.close();

    TextFileFormat format;
    const TextFileFormat::ReadResult result = format.readFile(
        FilePath::fromString(fileName), TextEncoding(TextEncoding::Utf8),
        {}, TextFileFormat::DecodingErrorDetails::Collect);

    QCOMPARE(result.code, TextFileFormat::ReadEncodingError);
    constexpr qsizetype retainedErrorCount
        = TextFileFormat::MaximumDecodingErrorOffsets + 1;
    QCOMPARE(result.decodingErrorOffsets.size(), retainedErrorCount);
    QCOMPARE(result.decodingErrorOffsets.first(), qsizetype(1 << 20));
    QCOMPARE(result.decodingErrorOffsets.last(), qsizetype((1 << 20) + retainedErrorCount - 1));
    QCOMPARE(result.decodingErrorOffset, qsizetype(1 << 20));
    QCOMPARE(result.decodingErrorPosition, qsizetype(1 << 20));
}

QTEST_GUILESS_MAIN(TextFileFormatTest)
#include "tst_textfileformat.moc"
