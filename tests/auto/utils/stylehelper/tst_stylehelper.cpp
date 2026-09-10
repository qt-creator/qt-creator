// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/stylehelper.h>

#include <QTest>

#include <cmath>

using namespace Utils;

class tst_StyleHelper : public QObject
{
    Q_OBJECT

private slots:
    void luminanceIsThatOfThePaintedPixel();
    void ensureReadableOn_data();
    void ensureReadableOn();
    void luminanceRisesWithTheHsvValue_data();
    void luminanceRisesWithTheHsvValue();
    void everyColorBecomesReadable_data();
    void everyColorBecomesReadable();
};

// A color is only ever seen as the pixel it paints, so its luminance has to be
// that pixel's. An HSV color carries more precision than the pixel keeps, and
// luminance() caches by the pixel, so reading the finer value stores a
// luminance no color on screen has.
void tst_StyleHelper::luminanceIsThatOfThePaintedPixel()
{
    // https://www.w3.org/TR/2008/REC-WCAG20-20081211/#relativeluminancedef
    const auto channel = [](int eightBit) {
        const double c = eightBit / 255.;
        return c < 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };

    for (int hue = 0; hue < 360; hue += 3) {
        for (int saturation : {1, 64, 128, 192, 254, 255}) {
            for (int value : {1, 2, 3, 7, 31, 77, 128, 200, 254, 255}) {
                const QColor color = QColor::fromHsv(hue, saturation, value);
                const QRgb pixel = color.rgb();
                const double expected = 0.2126 * channel(qRed(pixel))
                                        + 0.7152 * channel(qGreen(pixel))
                                        + 0.0722 * channel(qBlue(pixel));
                QVERIFY2(
                    qFuzzyCompare(StyleHelper::luminance(color), expected),
                    qPrintable(QString("%1 paints %2, whose luminance is %3, but reads %4")
                                   .arg(color.name(QColor::HexArgb))
                                   .arg(color.name())
                                   .arg(expected)
                                   .arg(StyleHelper::luminance(color))));
            }
        }
    }
}

void tst_StyleHelper::ensureReadableOn_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<QColor>("desired");
    QTest::addColumn<QColor>("expected");

    QTest::newRow("already readable")
        << QColor("#262626") << QColor("#c06060") << QColor("#c06060");
    QTest::newRow("only lighter reaches 3:1")
        << QColor("#262626") << QColor("#800000") << QColor("#e00000");
    QTest::newRow("only darker reaches 3:1")
        << QColor("#ffffff") << QColor("#eeeeee") << QColor("#949494");
    QTest::newRow("only darker, saturated")
        << QColor("#ffffff") << QColor("#ffff00") << QColor("#9a9a00");
    QTest::newRow("both directions, nearer one wins")
        << QColor("#5a5a5a") << QColor("#808080") << QColor("#ababab");
    // A hue whose luminance ceiling is below 3:1 however bright it is made:
    // the value cannot do it, so the saturation gives way and the hue stays.
    QTest::newRow("only giving up saturation reaches 3:1")
        << QColor("#262626") << QColor("#0000ff") << QColor("#5656ff");
    // getHsv() and fromHsv() quantize, so the color at the desired value is a
    // step away from the desired one - and where that step is enough to read,
    // it is the nearest there is. Without it the bisection starts from a
    // readable end and overshoots, to #003bfc.
    QTest::newRow("the value quantizes into readability")
        << QColor("#000000") << QColor("#0037fb") << QColor("#003bfb");
    // The alpha of the desired color is the alpha of the answer, and it is no
    // part of the contrast it was chosen against: isReadableOn() reads rgb().
    QTest::newRow("alpha is carried over")
        << QColor("#262626") << QColor("#80800000") << QColor("#80e00000");
}

void tst_StyleHelper::ensureReadableOn()
{
    QFETCH(QColor, background);
    QFETCH(QColor, desired);
    QFETCH(QColor, expected);

    const QColor actual = StyleHelper::ensureReadableOn(background, desired);
    QCOMPARE(actual.name(QColor::HexArgb), expected.name(QColor::HexArgb));
    QVERIFY(StyleHelper::isReadableOn(background, actual));
    // The hue is what a formatter asked for; the value and, where the value
    // alone cannot reach 3:1, the saturation are what this is free to move.
    QCOMPARE(actual.hue(), desired.hue());
    QVERIFY(actual.saturation() <= desired.saturation());
}

// ensureReadableOn() bisects the HSV value, which only finds the nearest
// readable color if readability cannot come back once it is lost while walking
// away from the background. That holds because HSV to RGB scales every channel
// by the value, so the luminance never falls as the value rises.
void tst_StyleHelper::luminanceRisesWithTheHsvValue_data()
{
    QTest::addColumn<int>("hue");
    QTest::addColumn<int>("saturation");

    for (int hue = 0; hue < 360; hue += 15) {
        for (int saturation : {0, 1, 64, 128, 192, 254, 255}) {
            QTest::addRow("hue %d, saturation %d", hue, saturation) << hue << saturation;
        }
    }
}

void tst_StyleHelper::luminanceRisesWithTheHsvValue()
{
    QFETCH(int, hue);
    QFETCH(int, saturation);

    for (int value = 0; value < 255; ++value) {
        const QColor lower = QColor::fromHsv(hue, saturation, value);
        const QColor higher = QColor::fromHsv(hue, saturation, value + 1);
        QVERIFY2(
            StyleHelper::luminance(lower) <= StyleHelper::luminance(higher),
            qPrintable(QString("%1 is brighter than %2").arg(lower.name(), higher.name())));
    }
}

// Walking the value reaches 3:1 for most colors and not for a saturated hue on
// a dark background, where the luminance ceiling of the hue is below it. What
// the answer must always be is readable, whichever lever it took to get there.
void tst_StyleHelper::everyColorBecomesReadable_data()
{
    QTest::addColumn<QColor>("background");

    for (const char *background :
         {"#000000", "#1e1e1e", "#262626", "#808080", "#c0c0c0", "#ffffff"})
        QTest::newRow(background) << QColor(QLatin1String(background));
}

void tst_StyleHelper::everyColorBecomesReadable()
{
    QFETCH(QColor, background);

    for (int hue = 0; hue < 360; hue += 15) {
        for (int saturation = 0; saturation <= 255; saturation += 15) {
            for (int value = 0; value <= 255; value += 15) {
                const QColor desired = QColor::fromHsv(hue, saturation, value);
                const QColor actual = StyleHelper::ensureReadableOn(background, desired);
                QVERIFY2(StyleHelper::isReadableOn(background, actual),
                         qPrintable(QString("%1 on %2 stayed unreadable as %3")
                                        .arg(desired.name(), background.name(), actual.name())));
            }
        }
    }
}

QTEST_GUILESS_MAIN(tst_StyleHelper)

#include "tst_stylehelper.moc"
