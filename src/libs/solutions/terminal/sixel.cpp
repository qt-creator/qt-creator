// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "sixel.h"

#include <QColor>

#include <algorithm>
#include <cstring>

namespace TerminalSolution {

static constexpr int maxParameter = 1'000'000;
static constexpr int bandHeight = 6;

// The VT340 palette, in the percentages the format itself uses. Registers above
// it start out black, as on the terminals that only had these sixteen.
static constexpr int defaultPalette[16][3]
    = {{0, 0, 0},
       {20, 20, 80},
       {80, 13, 13},
       {20, 80, 20},
       {80, 20, 80},
       {20, 80, 80},
       {80, 80, 20},
       {53, 53, 53},
       {26, 26, 26},
       {33, 33, 60},
       {60, 20, 20},
       {33, 60, 33},
       {60, 33, 60},
       {33, 60, 60},
       {60, 60, 33},
       {80, 80, 80}};

static int percentToByte(int percent)
{
    return qRound(qBound(0, percent, 100) * 255 / 100.0);
}

static QRgb fromRgbPercent(int red, int green, int blue)
{
    return qRgb(percentToByte(red), percentToByte(green), percentToByte(blue));
}

static QRgb fromHls(int hue, int lightness, int saturation)
{
    // The format counts hue from blue, QColor from red
    const qreal h = ((qBound(0, hue, 360) + 240) % 360) / 360.0;
    const qreal s = qBound(0, saturation, 100) / 100.0;
    const qreal l = qBound(0, lightness, 100) / 100.0;
    return QColor::fromHslF(h, s, l).rgb();
}

// The introducer's parameters are not read: P1 is the aspect ratio, which is
// rendered 1:1 as on every terminal that does not have the VT340's pixels, and
// P2 only says what the pixels that are never written look like - they are left
// transparent, so the terminal background shows through either way.
void SixelDecoder::start()
{
    *this = SixelDecoder();

    m_palette.assign(colorRegisterCount, qRgb(0, 0, 0));
    for (int i = 0; i < 16; ++i)
        m_palette[i]
            = fromRgbPercent(defaultPalette[i][0], defaultPalette[i][1], defaultPalette[i][2]);
    m_color = m_palette[0];

    m_started = true;
}

void SixelDecoder::addData(QByteArrayView data)
{
    if (!m_started)
        return;

    for (const char c : data)
        handleByte(c);
}

QImage SixelDecoder::takeImage()
{
    QImage image;

    if (m_usedWidth > 0 && m_usedHeight > 0) {
        image = QImage(m_usedWidth, m_usedHeight, QImage::Format_ARGB32_Premultiplied);
        if (!image.isNull()) {
            for (int y = 0; y < m_usedHeight; ++y)
                std::memcpy(image.scanLine(y), scanLine(y), size_t(m_usedWidth) * sizeof(QRgb));
        }
    }

    *this = SixelDecoder();

    return image;
}

void SixelDecoder::handleByte(char c)
{
    if (c >= '0' && c <= '9') {
        if (!m_inParameter) {
            m_parameters.push_back(0);
            m_inParameter = true;
        }
        int &value = m_parameters.back();
        if (value < maxParameter)
            value = value * 10 + (c - '0');
        return;
    }

    if (c == ';') {
        if (!m_inParameter)
            m_parameters.push_back(0);
        m_inParameter = false;
        return;
    }

    endCommand();

    switch (c) {
    case '!': // repeat the next sixel
    case '"': // raster attributes
    case '#': // color register
        m_command = c;
        m_parameters.clear();
        m_inParameter = false;
        return;
    case '$': // back to the left edge, same band
        m_x = 0;
        return;
    case '-': // one band down, back to the left edge
        m_x = 0;
        m_bandTop = qMin(m_bandTop + bandHeight, maxDimension);
        return;
    default:
        break;
    }

    // Everything outside the data range, notably the line breaks some encoders
    // wrap their output at, means nothing here.
    if (c >= '?' && c <= '~')
        paint(c - '?', m_repeat);
}

void SixelDecoder::endCommand()
{
    const auto parameter = [this](size_t index, int fallback) {
        return index < m_parameters.size() ? m_parameters[index] : fallback;
    };

    switch (m_command) {
    case '!':
        m_repeat = qBound(1, parameter(0, 1), maxDimension);
        break;
    case '"':
        // The declared size is only a hint: the data decides how big the image
        // really is. Taking it saves growing the pixels over and over.
        grow(qMin(parameter(2, 0), maxDimension), qMin(parameter(3, 0), maxDimension));
        break;
    case '#': {
        const int index = qBound(0, parameter(0, 0), colorRegisterCount - 1);
        const int system = parameter(1, 0);
        if (m_parameters.size() >= 5) {
            const int first = parameter(2, 0);
            const int second = parameter(3, 0);
            const int third = parameter(4, 0);
            if (system == 1)
                m_palette[index] = fromHls(first, second, third);
            else if (system == 2)
                m_palette[index] = fromRgbPercent(first, second, third);
        }
        m_color = m_palette[index];
        break;
    }
    default:
        break;
    }

    m_command = 0;
    m_parameters.clear();
    m_inParameter = false;
}

void SixelDecoder::paint(int bits, int repeat)
{
    m_repeat = 1;

    if (m_x >= maxDimension || repeat < 1)
        return;

    const int right = qMin(m_x + repeat, maxDimension);

    if (bits == 0) {
        // Nothing to paint, but the position moves on
        m_x = right;
        return;
    }

    if (!grow(right, m_bandTop + bandHeight))
        return;

    for (int bit = 0; bit < bandHeight; ++bit) {
        if (!(bits & (1 << bit)))
            continue;

        const int y = m_bandTop + bit;
        QRgb *line = scanLine(y);
        std::fill(line + m_x, line + right, m_color);
        m_usedHeight = qMax(m_usedHeight, y + 1);
    }

    m_usedWidth = qMax(m_usedWidth, right);
    m_x = right;
}

bool SixelDecoder::grow(int width, int height)
{
    if (m_full)
        return false;

    if (width <= m_width && height <= m_height)
        return true;

    if (width > maxDimension || height > maxDimension) {
        m_full = true;
        return false;
    }

    int newWidth = qMax(m_width, width);
    int newHeight = qMax(m_height, height);

    // The final size is only known once the data ends, so grow in steps
    const auto step = [](int needed, int current) {
        return qMin(maxDimension, qMax(needed, qMax(64, current * 2)));
    };
    const int stepWidth = newWidth > m_width ? step(newWidth, m_width) : newWidth;
    const int stepHeight = newHeight > m_height ? step(newHeight, m_height) : newHeight;

    if (qint64(stepWidth) * stepHeight <= maxPixels) {
        newWidth = stepWidth;
        newHeight = stepHeight;
    } else if (qint64(newWidth) * newHeight > maxPixels) {
        m_full = true;
        return false;
    }

    std::vector<QRgb> pixels(size_t(newWidth) * size_t(newHeight), 0);
    for (int y = 0; y < m_height; ++y) {
        std::copy_n(m_pixels.begin() + size_t(y) * size_t(m_width),
                    m_width,
                    pixels.begin() + size_t(y) * size_t(newWidth));
    }

    m_pixels = std::move(pixels);
    m_width = newWidth;
    m_height = newHeight;

    return true;
}

} // namespace TerminalSolution
