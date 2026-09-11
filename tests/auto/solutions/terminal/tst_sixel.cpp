// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/sixel.h>

#include <QTest>

using namespace TerminalSolution;

class tst_Sixel : public QObject
{
    Q_OBJECT

private:
    static constexpr const char *red = "#0;2;100;0;0";
    static constexpr const char *green = "#1;2;0;100;0";

    SixelDecoder m_decoder;

    QImage decode(const QByteArray &data)
    {
        m_decoder.start();
        m_decoder.addData(data);
        return m_decoder.takeImage();
    }

private slots:
    void noDataGivesNoImage()
    {
        QVERIFY(decode("").isNull());
        QVERIFY(decode("#0;2;100;0;0").isNull());
    }

    void dataWithoutStartIsDropped()
    {
        SixelDecoder decoder;
        decoder.addData("~");

        QVERIFY(decoder.takeImage().isNull());
    }

    void oneSixelIsOneColumnOfSixPixels()
    {
        const QImage image = decode(QByteArray(red) + "~");

        QCOMPARE(image.size(), QSize(1, 6));
        for (int y = 0; y < 6; ++y)
            QCOMPARE(image.pixelColor(0, y), QColor(Qt::red));
    }

    void theBitsOfASixelAreTheRowsOfItsBand()
    {
        // 'A' is 0x41, two above the zero of 0x3f, so only the second row
        const QImage image = decode(QByteArray(red) + "A");

        QCOMPARE(image.size(), QSize(1, 2));
        QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(image.pixelColor(0, 1), QColor(Qt::red));
    }

    void pixelsThatAreNeverPaintedStayTransparent()
    {
        // '?' is the sixel with no bit set at all
        QVERIFY(decode(QByteArray(red) + "?").isNull());

        const QImage image = decode(QByteArray(red) + "?~");

        QCOMPARE(image.size(), QSize(2, 6));
        QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(image.pixelColor(1, 0), QColor(Qt::red));
    }

    void repeatRepeatsTheNextSixelOnly()
    {
        const QImage image = decode(QByteArray(red) + "!4~" + green + "~");

        QCOMPARE(image.size(), QSize(5, 6));
        for (int x = 0; x < 4; ++x)
            QCOMPARE(image.pixelColor(x, 0), QColor(Qt::red));
        QCOMPARE(image.pixelColor(4, 0), QColor(Qt::green));
    }

    void aCarriageReturnGoesBackToTheLeftEdge()
    {
        const QImage image = decode(QByteArray(red) + "!4~$" + green + "~");

        QCOMPARE(image.size(), QSize(4, 6));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::green));
        QCOMPARE(image.pixelColor(1, 0), QColor(Qt::red));
    }

    void aNewLineStartsTheNextBand()
    {
        const QImage image = decode(QByteArray(red) + "~-" + green + "~");

        QCOMPARE(image.size(), QSize(1, 12));
        QCOMPARE(image.pixelColor(0, 5), QColor(Qt::red));
        QCOMPARE(image.pixelColor(0, 6), QColor(Qt::green));
    }

    void colorsAreGivenInPercentages()
    {
        const QImage image = decode(QByteArray("#0;2;100;50;0~"));

        QCOMPARE(image.pixelColor(0, 0), QColor(255, 128, 0));
    }

    void hueIsCountedFromBlue()
    {
        // Hue 0, full saturation, half lightness: blue, where a hue counted
        // from red the way QColor does it would be red
        const QImage image = decode(QByteArray("#0;1;0;50;100~"));

        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::blue));
    }

    void aColorRegisterIsRememberedUntilItIsRedefined()
    {
        const QImage image = decode(QByteArray(red) + "~" + green + "~#0~");

        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(image.pixelColor(1, 0), QColor(Qt::green));
        QCOMPARE(image.pixelColor(2, 0), QColor(Qt::red));
    }

    void theDataDecidesTheSizeNotTheRasterAttributes()
    {
        const QImage image = decode(QByteArray("\"1;1;64;64") + red + "~");

        QCOMPARE(image.size(), QSize(1, 6));
    }

    void lineBreaksInTheDataAreIgnored()
    {
        // Some encoders wrap their output to keep the lines short
        const QImage image = decode(QByteArray(red) + "~\r\n~");

        QCOMPARE(image.size(), QSize(2, 6));
        QCOMPARE(image.pixelColor(1, 0), QColor(Qt::red));
    }

    void dataArrivingInFragmentsDecodesTheSame()
    {
        const QByteArray data = QByteArray(red) + "!3~-" + green + "@$#0;1;0;50;100~";
        const QImage whole = decode(data);
        QVERIFY(!whole.isNull());

        for (qsizetype split = 1; split < data.size(); ++split) {
            m_decoder.start();
            m_decoder.addData(data.first(split));
            m_decoder.addData(data.sliced(split));

            QVERIFY2(m_decoder.takeImage() == whole,
                     qPrintable(QString("split at %1").arg(split)));
        }
    }

    void anImageIsNeverBiggerThanTheLimit()
    {
        const QImage image = decode(QByteArray(red) + "!999999999~");

        QCOMPARE(image.width(), SixelDecoder::maxDimension);
        QCOMPARE(image.pixelColor(SixelDecoder::maxDimension - 1, 0), QColor(Qt::red));
    }

    void dataBeyondTheLimitIsDropped()
    {
        QByteArray data(red);
        for (int i = 0; i < SixelDecoder::maxDimension + 100; ++i)
            data += "-~";

        const QImage image = decode(data);

        QVERIFY(image.height() <= SixelDecoder::maxDimension);
        QVERIFY(qint64(image.width()) * image.height() <= SixelDecoder::maxPixels);
    }
};

QTEST_GUILESS_MAIN(tst_Sixel)

#include "tst_sixel.moc"
