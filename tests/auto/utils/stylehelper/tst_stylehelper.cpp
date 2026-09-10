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

QTEST_GUILESS_MAIN(tst_StyleHelper)

#include "tst_stylehelper.moc"
