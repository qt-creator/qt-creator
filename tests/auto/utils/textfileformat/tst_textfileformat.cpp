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
};

QTEST_GUILESS_MAIN(TextFileFormatTest)
#include "tst_textfileformat.moc"
